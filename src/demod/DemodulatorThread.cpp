#include "DemodulatorThread.h"
#include "CubicSDRDefs.h"
#include <vector>

DemodulatorThread::DemodulatorThread(DemodulatorThreadInputQueue* pQueue) :
        inputQueue(pQueue), visOutQueue(NULL), terminated(false), initialized(false), audio_resampler(NULL), resample_ratio(1), audio_resample_ratio(
                1), resampler(NULL), commandQueue(NULL), fir_filter(NULL), audioInputQueue(NULL) {

    float kf = 0.5;         // modulation factor
    fdem = freqdem_create(kf);
//    freqdem_print(fdem);

    nco_shift = nco_crcf_create(LIQUID_VCO);
    shift_freq = 0;

    workerQueue = new DemodulatorThreadWorkerCommandQueue;
    workerResults = new DemodulatorThreadWorkerResultQueue;
    workerThread = new DemodulatorWorkerThread(workerQueue, workerResults);

    t_Worker = new std::thread(&DemodulatorWorkerThread::threadMain, workerThread);
}

void DemodulatorThread::initialize() {
    initialized = false;

    resample_ratio = (float) (params.bandwidth) / (float) params.inputRate;
    audio_resample_ratio = (float) (params.audioSampleRate) / (float) params.bandwidth;

    float fc = 0.5 * ((double) params.bandwidth / (double) params.inputRate);         // filter cutoff frequency

    if (fc <= 0) {
        fc = 0;
    }

    if (fc >= 0.5) {
        fc = 0.5;
    }

    float ft = 0.05f;         // filter transition
    float As = 60.0f;         // stop-band attenuation [dB]
    float mu = 0.0f;         // fractional timing offset

    // estimate required filter length and generate filter
    unsigned int h_len = estimate_req_filter_len(ft, As);
    float h[h_len];
    liquid_firdes_kaiser(h_len, fc, As, mu, h);

    if (fir_filter) {
        firfilt_crcf_recreate(fir_filter, h, h_len);
    } else {
        fir_filter = firfilt_crcf_create(h, h_len);
    }

    // create multi-stage arbitrary resampler object
    if (resampler) {
        msresamp_crcf_destroy(resampler);
    }
    resampler = msresamp_crcf_create(resample_ratio, As);
//    msresamp_crcf_print(resampler);

    if (audio_resampler) {
        msresamp_crcf_destroy(audio_resampler);
    }
    audio_resampler = msresamp_crcf_create(audio_resample_ratio, As);
//    msresamp_crcf_print(audio_resampler);

    initialized = true;
//    std::cout << "inputResampleRate " << params.bandwidth << std::endl;

    last_params = params;
}

DemodulatorThread::~DemodulatorThread() {
    delete workerThread;
    delete workerQueue;
    delete workerResults;
}

void DemodulatorThread::threadMain() {

    if (!initialized) {
        initialize();
    }

    std::cout << "Demodulator thread started.." << std::endl;
    while (!terminated) {
        DemodulatorThreadIQData inp;
        inputQueue->pop(inp);

        bool bandwidthChanged = false;
        DemodulatorThreadParameters bandwidthParams = params;

        if (!commandQueue->empty()) {
            bool paramsChanged = false;
            while (!commandQueue->empty()) {
                DemodulatorThreadCommand command;
                commandQueue->pop(command);
                switch (command.cmd) {
                case DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_BANDWIDTH:
                    if (command.int_value < 3000) {
                        command.int_value = 3000;
                    }
                    if (command.int_value > SRATE) {
                        command.int_value = SRATE;
                    }
                    bandwidthParams.bandwidth = command.int_value;
                    bandwidthChanged = true;
                    break;
                case DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY:
                    params.frequency = command.int_value;
                    break;
                }
            }

            if (bandwidthChanged) {
                DemodulatorWorkerThreadCommand command(DemodulatorWorkerThreadCommand::DEMOD_WORKER_THREAD_CMD_BUILD_FILTERS);
                command.audioSampleRate = bandwidthParams.audioSampleRate;
                command.bandwidth = bandwidthParams.bandwidth;
                command.frequency = bandwidthParams.frequency;
                command.inputRate = bandwidthParams.inputRate;

                workerQueue->push(command);
            }
        }

        if (!initialized) {
            continue;
        }

        // Requested frequency is not center, shift it into the center!
        if (inp.frequency != params.frequency) {
            if ((params.frequency - inp.frequency) != shift_freq) {
                shift_freq = params.frequency - inp.frequency;
                if (abs(shift_freq) <= (int) ((float) (SRATE / 2) * 1.5)) {
                    nco_crcf_set_frequency(nco_shift, (2.0 * M_PI) * (((float) abs(shift_freq)) / ((float) SRATE)));
                }
            }
        }

        if (abs(shift_freq) > (int) ((float) (SRATE / 2) * 1.5)) {
            continue;
        }

        std::vector<signed char> *data = &inp.data;
        if (data->size()) {
            liquid_float_complex filtered_input[BUF_SIZE / 2];

            liquid_float_complex x, y, z;

            for (int i = 0; i < BUF_SIZE / 2; i++) {
                if (shift_freq != 0) {
                    nco_crcf_step(nco_shift);

                    z.real = (float) (*data)[i * 2] / 127.0f;
                    z.imag = (float) (*data)[i * 2 + 1] / 127.0f;

                    if (shift_freq < 0) {
                        nco_crcf_mix_up(nco_shift, z, &x);
                    } else {
                        nco_crcf_mix_down(nco_shift, z, &x);
                    }
                } else {
                    x.real = (float) (*data)[i * 2] / 127.0f;
                    x.imag = (float) (*data)[i * 2 + 1] / 127.0f;
                }

                firfilt_crcf_push(fir_filter, x);      // push input sample
                firfilt_crcf_execute(fir_filter, &y);   // compute output

                filtered_input[i] = y;
            }

            int out_size = ceil((float) (BUF_SIZE / 2) * resample_ratio);

            liquid_float_complex resampled_output[out_size];

            unsigned int num_written;       // number of values written to buffer
            msresamp_crcf_execute(resampler, filtered_input, (BUF_SIZE / 2), resampled_output, &num_written);

            float pcm = 0;

            for (int i = 0; i < num_written; i++) {
                freqdem_demodulate(fdem, resampled_output[i], &pcm);
                resampled_output[i].real = (float) pcm;
                resampled_output[i].imag = 0;
            }

            int audio_out_size = ceil((float) (num_written) * audio_resample_ratio);
            liquid_float_complex resampled_audio_output[audio_out_size];

            unsigned int num_audio_written;
            msresamp_crcf_execute(audio_resampler, resampled_output, num_written, resampled_audio_output, &num_audio_written);

            std::vector<float> newBuffer;
            newBuffer.resize(num_audio_written * 2);
            for (int i = 0; i < num_audio_written; i++) {
                liquid_float_complex y = resampled_audio_output[i];

                newBuffer[i * 2] = y.real;
                newBuffer[i * 2 + 1] = y.real;
            }

            AudioThreadInput ati;
            ati.data = newBuffer;

            if (audioInputQueue != NULL) {
                audioInputQueue->push(ati);
            }

            if (visOutQueue != NULL) {
                visOutQueue->push(ati);
            }
        }

        if (!workerResults->empty()) {
            while (!workerResults->empty()) {
                DemodulatorWorkerThreadResult result;
                workerResults->pop(result);

                switch (result.cmd) {
                case DemodulatorWorkerThreadResult::DEMOD_WORKER_THREAD_RESULT_FILTERS:
                    firfilt_crcf_destroy(fir_filter);
                    msresamp_crcf_destroy(resampler);
                    msresamp_crcf_destroy(audio_resampler);

                    fir_filter = result.fir_filter;
                    resampler = result.resampler;
                    audio_resampler = result.audio_resampler;

                    resample_ratio = result.resample_ratio;
                    audio_resample_ratio = result.audio_resample_ratio;

                    params.audioSampleRate = result.audioSampleRate;
                    params.bandwidth = result.bandwidth;
                    params.inputRate = result.inputRate;

                    break;
                }
            }
        }
    }

    std::cout << "Demodulator thread done." << std::endl;
}

void DemodulatorThread::terminate() {
    terminated = true;
    DemodulatorThreadIQData inp;    // push dummy to nudge queue
    inputQueue->push(inp);

    workerThread->terminate();
    t_Worker->join();
}

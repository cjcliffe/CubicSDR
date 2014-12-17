
#include "CubicSDRDefs.h"
#include <vector>

#ifdef __APPLE__
#include <pthread.h>
#endif

#include "DemodulatorPreThread.h"

DemodulatorPreThread::DemodulatorPreThread(DemodulatorThreadInputQueue* pQueueIn, DemodulatorThreadPostInputQueue* pQueueOut, DemodulatorThreadCommandQueue* threadQueueNotify) :
        inputQueue(pQueueIn), postInputQueue(pQueueOut), terminated(false), initialized(false), audio_resampler(NULL), resample_ratio(1), audio_resample_ratio(
                1), resampler(NULL), commandQueue(NULL), fir_filter(NULL), audioInputQueue(NULL), threadQueueNotify(threadQueueNotify) {

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

void DemodulatorPreThread::initialize() {
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

DemodulatorPreThread::~DemodulatorPreThread() {
    delete workerThread;
    delete workerQueue;
    delete workerResults;
}

#ifdef __APPLE__
void *DemodulatorPreThread::threadMain() {
#else
void DemodulatorPreThread::threadMain() {
#endif
#ifdef __APPLE__
	    pthread_t tID = pthread_self();	 // ID of this thread
	    int priority = sched_get_priority_min( SCHED_RR );
	    sched_param prio = { priority }; // scheduling priority of thread
	    pthread_setschedparam( tID, SCHED_RR, &prio );
#endif

    if (!initialized) {
        initialize();
    }


    std::cout << "Demodulator preprocessor thread started.." << std::endl;
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
        	int bufSize = data->size() / 2;

            liquid_float_complex in_buf_data[bufSize];
            liquid_float_complex out_buf_data[bufSize];

            liquid_float_complex *in_buf = in_buf_data;
            liquid_float_complex *out_buf = out_buf_data;
            liquid_float_complex *temp_buf = NULL;

            for (int i = 0; i < bufSize; i++) {
                in_buf[i].real = (float) (*data)[i * 2] / 127.0f;
                in_buf[i].imag = (float) (*data)[i * 2 + 1] / 127.0f;
            }

            if (shift_freq != 0) {
                if (shift_freq < 0) {
                    nco_crcf_mix_block_up(nco_shift, in_buf, out_buf, bufSize);
                } else {
                    nco_crcf_mix_block_down(nco_shift, in_buf, out_buf, bufSize);
                }
                temp_buf = in_buf;
                in_buf = out_buf;
                out_buf = temp_buf;
            }

            DemodulatorThreadPostIQData resamp;
            resamp.data.resize(bufSize);

            firfilt_crcf_execute_block(fir_filter, in_buf, bufSize, &resamp.data[0]);

            resamp.audio_resample_ratio = audio_resample_ratio;
            resamp.audio_resampler = audio_resampler;
            resamp.resample_ratio = resample_ratio;
            resamp.resampler = resampler;

            postInputQueue->push(resamp);
        }

        if (!workerResults->empty()) {
            while (!workerResults->empty()) {
                DemodulatorWorkerThreadResult result;
                workerResults->pop(result);

                switch (result.cmd) {
                case DemodulatorWorkerThreadResult::DEMOD_WORKER_THREAD_RESULT_FILTERS:
                    firfilt_crcf_destroy(fir_filter);
//                    msresamp_crcf_destroy(resampler);
//                    msresamp_crcf_destroy(audio_resampler);

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

    std::cout << "Demodulator preprocessor thread done." << std::endl;
    DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_PREPROCESS_TERMINATED);
    tCmd.context = this;
    threadQueueNotify->push(tCmd);
}

void DemodulatorPreThread::terminate() {
    terminated = true;
    DemodulatorThreadIQData inp;    // push dummy to nudge queue
    inputQueue->push(inp);
    workerThread->terminate();
}

#include "DemodulatorThread.h"
#include "CubicSDRDefs.h"
#include <vector>

DemodulatorThread::DemodulatorThread(DemodulatorThreadInputQueue* pQueue, DemodulatorThreadParameters *params_in) :
        inputQueue(pQueue), visOutQueue(NULL), terminated(false) {

    DemodulatorThreadParameters defaultParams;
    if (!params_in) {
        params = defaultParams;
    } else {
        params = *params_in;
    }

    resample_ratio = (float) (params.inputResampleRate) / (float) params.inputRate;
    second_resampler_ratio = (float) (params.demodResampleRate) / (float) params.inputResampleRate;
    audio_resample_ratio = (float) (params.audioSampleRate) / (float) params.demodResampleRate;

    float fc = 0.5f * ((float) params.inputResampleRate / (float) params.inputRate) * 0.75;         // filter cutoff frequency
    float ft = 0.05f;         // filter transition
    float As = 60.0f;         // stop-band attenuation [dB]
    float mu = 0.0f;         // fractional timing offset

    // estimate required filter length and generate filter
    unsigned int h_len = estimate_req_filter_len(ft, As);
    float h[h_len];
    liquid_firdes_kaiser(h_len, fc, As, mu, h);

    fir_filter = firfilt_crcf_create(h, h_len);

    h_len = estimate_req_filter_len(ft, As);
    liquid_firdes_kaiser(h_len, (float) params.filterFrequency / (float) params.demodResampleRate, As, mu, h);

    fir_audio_filter = firfilt_crcf_create(h, h_len);

    // create multi-stage arbitrary resampler object
    resampler = msresamp_crcf_create(resample_ratio, As);
    msresamp_crcf_print(resampler);

    second_resampler = msresamp_crcf_create(second_resampler_ratio, As);
    msresamp_crcf_print(second_resampler);

    audio_resampler = msresamp_crcf_create(audio_resample_ratio, As);
    msresamp_crcf_print(audio_resampler);

    float kf = 0.75;         // modulation factor

    fdem = freqdem_create(kf);
    freqdem_print(fdem);
}

DemodulatorThread::~DemodulatorThread() {
    std::cout << std::endl << "Demodulator Thread Done." << std::endl << std::endl;
}

void DemodulatorThread::threadMain() {

    while (!terminated) {
        DemodulatorThreadIQData inp;
        inputQueue->pop(inp);

        std::vector<signed char> *data = &inp.data;
        if (data->size()) {
            liquid_float_complex filtered_input[BUF_SIZE / 2];

            for (int i = 0; i < BUF_SIZE / 2; i++) {

                liquid_float_complex x;
                liquid_float_complex y;

                x.real = (float) (*data)[i * 2] / 127.0f;
                x.imag = (float) (*data)[i * 2 + 1] / 127.0f;

                firfilt_crcf_push(fir_filter, x);      // push input sample
                firfilt_crcf_execute(fir_filter, &y);   // compute output

                filtered_input[i] = y;
            }

            int out_size = ceil((float) (BUF_SIZE / 2) * resample_ratio);

            liquid_float_complex resampled_output[out_size];

            unsigned int num_written;       // number of values written to buffer
            msresamp_crcf_execute(resampler, filtered_input, (BUF_SIZE / 2), resampled_output, &num_written);

            float waveform_ceil = 0, waveform_floor = 0;

            float pcm = 0;

            for (int i = 0; i < num_written; i++) {
                freqdem_demodulate(fdem, resampled_output[i], &pcm);

                resampled_output[i].real = (float) pcm;
                resampled_output[i].imag = 0;

                if (waveform_ceil < resampled_output[i].real) {
                    waveform_ceil = resampled_output[i].real;
                }

                if (waveform_floor > resampled_output[i].real) {
                    waveform_floor = resampled_output[i].real;
                }
            }

            int wbfm_out_size = ceil((float) (num_written) * second_resampler_ratio);
            liquid_float_complex resampled_wbfm_output[wbfm_out_size];

            unsigned int num_wbfm_written;
            msresamp_crcf_execute(second_resampler, resampled_output, num_written, resampled_wbfm_output, &num_wbfm_written);

            for (int i = 0; i < num_wbfm_written; i++) {
                firfilt_crcf_push(fir_audio_filter, resampled_wbfm_output[i]);
                firfilt_crcf_execute(fir_audio_filter, &resampled_wbfm_output[i]);
            }

            int audio_out_size = ceil((float) (num_wbfm_written) * audio_resample_ratio);
            liquid_float_complex resampled_audio_output[audio_out_size];

            unsigned int num_audio_written;
            msresamp_crcf_execute(audio_resampler, resampled_wbfm_output, num_wbfm_written, resampled_audio_output, &num_audio_written);

            std::vector<float> newBuffer;
            newBuffer.resize(num_audio_written * 2);
            for (int i = 0; i < num_audio_written; i++) {
                liquid_float_complex y = resampled_audio_output[i];

                newBuffer[i * 2] = y.real;
                newBuffer[i * 2 + 1] = y.real;
            }

            AudioThreadInput ati;
            ati.data = newBuffer;

            if (params.audioInputQueue != NULL) {
                params.audioInputQueue->push(ati);
            }

            if (visOutQueue != NULL) {
                visOutQueue->push(ati);
            }
        }
    }
}

void DemodulatorThread::terminate() {
    std::cout << "Terminating demodulator thread.." << std::endl;
    terminated = true;
    DemodulatorThreadIQData inp;    // push dummy to nudge queue
    inputQueue->push(inp);
}

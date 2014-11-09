#include "Demodulator.h"

static int patestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData) {

    Demodulator *src = (Demodulator *) userData;

    float *out = (float*) outputBuffer;

    if (!src->audio_queue.size()) {
        for (int i = 0; i < framesPerBuffer * 2; i++) {
            out[i] = 0;
        }
        return paContinue;
    }

    std::vector<float> *nextBuffer = src->audio_queue.front();

    for (int i = 0; i < framesPerBuffer * 2; i++) {
        out[i] = (*nextBuffer)[src->audio_queue_ptr];

        src->audio_queue_ptr++;

        if (src->audio_queue_ptr == nextBuffer->size()) {
            src->audio_queue.pop();
            delete nextBuffer;
            src->audio_queue_ptr = 0;
            if (!src->audio_queue.size()) {
                break;
            }
            nextBuffer = src->audio_queue.front();
        }
    }

    return paContinue;
}

Demodulator::Demodulator() {

    bandwidth = 800000;
    resample_ratio = (float) (bandwidth) / (float) SRATE;
    audio_frequency = 44100;
    audio_resample_ratio = (float) (audio_frequency) / (float) bandwidth;

    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cout << "Error starting :(\n";
    }

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
        std::cout << "Error: No default output device.\n";
    }

    outputParameters.channelCount = 2; /* Stereo output, most likely supported. */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output. */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    stream = NULL;

    err = Pa_OpenStream(&stream, NULL, &outputParameters, 44100, 256, paClipOff, &patestCallback, this);

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cout << "Error starting stream: " << Pa_GetErrorText(err) << std::endl;
        std::cout << "\tPortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    float fc = 0.5f * (bandwidth / SRATE);         // filter cutoff frequency
    float ft = 0.05f;// filter transition
    float As = 60.0f;// stop-band attenuation [dB]
    float mu = 0.0f;// fractional timing offset

    // estimate required filter length and generate filter
    unsigned int h_len = estimate_req_filter_len(ft, As);
    float h[h_len];
    liquid_firdes_kaiser(h_len, fc, As, mu, h);

    fir_filter = firfilt_crcf_create(h, h_len);

    // create multi-stage arbitrary resampler object
    resampler = msresamp_crcf_create(resample_ratio, As);
    msresamp_crcf_print(resampler);

    audio_resampler = msresamp_crcf_create(audio_resample_ratio, As);
    msresamp_crcf_print(audio_resampler);

    float kf = 0.1f;// modulation factor

    fdem = freqdem_create(kf);
    freqdem_print(fdem);
}

Demodulator::~Demodulator() {
    PaError err;
    err = Pa_StopStream(stream);
    err = Pa_CloseStream(stream);
    Pa_Terminate();
}

void Demodulator::writeBuffer(std::vector<signed char> *data) {
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

    int audio_out_size = ceil((float) (num_written) * audio_resample_ratio);
    liquid_float_complex resampled_audio_output[audio_out_size];

    unsigned int num_audio_written;       // number of values written to buffer
    msresamp_crcf_execute(audio_resampler, resampled_output, num_written, resampled_audio_output, &num_audio_written);

    std::vector<float> *newBuffer = new std::vector<float>;
    newBuffer->resize(num_audio_written * 2);
    for (int i = 0; i < num_audio_written; i++) {
        (*newBuffer)[i * 2] = resampled_audio_output[i].real;
        (*newBuffer)[i * 2 + 1] = resampled_audio_output[i].real;
    }

    audio_queue.push(newBuffer);

    if (waveform_points.size() != num_audio_written * 2) {
        waveform_points.resize(num_audio_written * 2);
    }

    for (int i = 0, iMax = waveform_points.size() / 2; i < iMax; i++) {
        waveform_points[i * 2 + 1] = resampled_audio_output[i].real * 0.5f;
        waveform_points[i * 2] = ((double) i / (double) iMax);
    }
}

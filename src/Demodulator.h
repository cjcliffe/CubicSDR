#pragma once

#include <vector>
#include <queue>
#include <cstring>
#include <iostream>
#include <math.h>


#include "CubicSDRDefs.h"
#include "liquid/liquid.h"

#include "portaudio.h"
#ifdef WIN32
#include "pa_stream.h"
#include "pa_debugprint.h"
#endif

static int patestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData);

class Demodulator {
public:
    std::queue<std::vector<float> *> audio_queue;
    unsigned int audio_queue_ptr;
    std::vector<float> waveform_points;

    Demodulator();
    ~Demodulator();

    void writeBuffer(std::vector<signed char> *data);

private:
    firfilt_crcf fir_filter;

    msresamp_crcf resampler;
    msresamp_crcf audio_resampler;
    float resample_ratio;
    unsigned int bandwidth;
    unsigned int audio_frequency;
    float audio_resample_ratio;

    PaStreamParameters outputParameters;
    PaStream *stream;
    freqdem fdem;
};

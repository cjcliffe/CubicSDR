#pragma once

#include <vector>
#include <queue>
#include <cstring>
#include <iostream>
#include <math.h>

#include "CubicSDRDefs.h"
#include "liquid/liquid.h"



class Demodulator {
public:
    std::vector<float> waveform_points;

    Demodulator();
    ~Demodulator();

    void writeBuffer(std::vector<signed char> *data);

private:
    firfilt_crcf fir_filter;
    firfilt_crcf fir_audio_filter;

    unsigned int bandwidth;

    msresamp_crcf resampler;
    float resample_ratio;

    msresamp_crcf wbfm_resampler;
    float wbfm_resample_ratio;
    unsigned int wbfm_frequency;

    msresamp_crcf audio_resampler;
    float audio_resample_ratio;

    unsigned int audio_frequency;

    freqdem fdem;
};

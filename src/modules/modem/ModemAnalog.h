// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "Modem.h"

class ModemKitAnalog : public ModemKit {
public:
    ModemKitAnalog() : ModemKit(), audioResampler(nullptr), audioResampleRatio(0) {
        
    };
    
    msresamp_rrrf audioResampler;
    double audioResampleRatio;
};


class ModemAnalog : public Modem {
public:
    ModemAnalog();
    std::string getType();
    virtual int checkSampleRate(long long sampleRate, int audioSampleRate);
    virtual ModemKit *buildKit(long long sampleRate, int audioSampleRate);
    virtual void disposeKit(ModemKit *kit);
    virtual void initOutputBuffers(ModemKitAnalog *akit, ModemIQData *input);
    virtual void buildAudioOutput(ModemKitAnalog *akit, AudioThreadInput *audioOut, bool autoGain);
    virtual std::vector<float> *getDemodOutputData();
    virtual std::vector<float> *getResampledOutputData();
protected:
    size_t bufSize;
    std::vector<float> demodOutputData;
    std::vector<float> resampledOutputData;

    float aOutputCeil;
    float aOutputCeilMA;
    float aOutputCeilMAA;
};

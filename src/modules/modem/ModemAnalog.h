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
    ModemKit *buildKit(long long sampleRate, int audioSampleRate);
    void disposeKit(ModemKit *kit);
    void initOutputBuffers(ModemKitAnalog *akit, ModemIQData *input);
    void buildAudioOutput(ModemKitAnalog *akit, AudioThreadInput *audioOut, bool autoGain);
    std::vector<float> *getDemodOutputData();
    std::vector<float> *getResampledOutputData();
protected:
    int bufSize;
    std::vector<float> demodOutputData;
    std::vector<float> resampledOutputData;

    float aOutputCeil;
    float aOutputCeilMA;
    float aOutputCeilMAA;
};
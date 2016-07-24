#pragma once
#include "Modem.h"

class ModemIQ : public Modem {
public:
    ModemIQ();

    std::string getType();
    std::string getName();
    
    static ModemBase *factory();

    int checkSampleRate(long long sampleRate, int audioSampleRate);
    int getDefaultSampleRate();

    ModemKit *buildKit(long long sampleRate, int audioSampleRate);

    void disposeKit(ModemKit *kit);
    
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    
};
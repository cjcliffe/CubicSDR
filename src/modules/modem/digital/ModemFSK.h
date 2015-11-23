#pragma once
#include "ModemDigital.h"

class ModemKitFSK : public ModemKitDigital {
public:
    float sps, spacing, bandwidth;
    unsigned int m, k;
    
    fskdem demodFSK;
    std::vector<liquid_float_complex> inputBuffer;
};


class ModemFSK : public ModemDigital {
public:
    ModemFSK();
    ~ModemFSK();
    std::string getName();
    Modem *factory();
    ModemKit *buildKit(long long sampleRate, int audioSampleRate);
    void disposeKit(ModemKit *kit);
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);


private:
    fskdem dem;
};


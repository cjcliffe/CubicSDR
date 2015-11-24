#pragma once
#include "Modem.h"

class ModemKitDigital : public ModemKit {
public:
    ModemKitDigital() : ModemKit() {
        
    };
};


class ModemDigital : public Modem {
public:
    ModemDigital();
    
    std::string getType();
    
    virtual int checkSampleRate(long long sampleRate, int audioSampleRate);
    
    virtual ModemKit *buildKit(long long sampleRate, int audioSampleRate);
    virtual void disposeKit(ModemKit *kit);
    
    virtual void digitalStart(ModemKitDigital *kit, modem mod, ModemIQData *input);
    virtual void digitalFinish(ModemKitDigital *kit, modem mod);

    virtual void setDemodulatorLock(bool demod_lock_in);
    virtual int getDemodulatorLock();
    
    virtual void updateDemodulatorLock(modem mod, float sensitivity);

protected:
    std::vector<unsigned int> demodOutputDataDigital;
    std::atomic_bool currentDemodLock;
    
//    std::vector<unsigned int> demodOutputDataDigitalTest;    
//    std::vector<unsigned char> demodOutputSoftbits;
//    std::vector<unsigned char> demodOutputSoftbitsTest;
};
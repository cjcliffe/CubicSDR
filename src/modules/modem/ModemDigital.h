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
    ModemKit *buildKit(long long sampleRate, int audioSampleRate);
    void disposeKit(ModemKit *kit);
    void digitalStart(ModemKitDigital *kit, modem mod, ModemIQData *input);
    void digitalFinish(ModemKitDigital *kit, modem mod);

    virtual void setDemodulatorLock(bool demod_lock_in);
    virtual int getDemodulatorLock();
    
    virtual void setDemodulatorCons(int demod_cons_in);
    virtual int getDemodulatorCons();

    virtual void updateDemodulatorCons(int cons);
    virtual void updateDemodulatorLock(modem mod, float sensitivity);

protected:
    std::vector<unsigned int> demodOutputDataDigital;
    std::atomic_int demodulatorCons;
    std::atomic_bool currentDemodLock;
    std::atomic_int currentDemodCons;
        
//    std::vector<unsigned int> demodOutputDataDigitalTest;    
//    std::vector<unsigned char> demodOutputSoftbits;
//    std::vector<unsigned char> demodOutputSoftbitsTest;
};
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
    ModemKit *buildKit(long long sampleRate, int audioSampleRate);
    void disposeKit(ModemKit *kit);

    void setDemodulatorLock(bool demod_lock_in);
    int getDemodulatorLock();
    
    void setDemodulatorCons(int demod_cons_in);
    int getDemodulatorCons();

    void updateDemodulatorCons(int Cons);
    void updateDemodulatorLock(modem demod, float sensitivity);

protected:
    std::vector<unsigned int> demodOutputDataDigital;
    std::atomic_int demodulatorCons;
    bool currentDemodLock;
    int currentDemodCons;
    
    int bufSize;
    
//    std::vector<unsigned int> demodOutputDataDigitalTest;    
//    std::vector<unsigned char> demodOutputSoftbits;
//    std::vector<unsigned char> demodOutputSoftbitsTest;
};
#pragma once
#include "ModemAnalog.h"

class ModemUSB : public ModemAnalog {
public:
    ModemUSB();
    ~ModemUSB();
    
    std::string getName();
    
    Modem *factory();
    
    int checkSampleRate(long long sampleRate, int audioSampleRate);
    int getDefaultSampleRate();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
#ifdef WIN32
	firfilt_crcf ssbFilt;
#else
	iirfilt_crcf ssbFilt;
#endif
	firhilbf c2rFilt;
    nco_crcf ssbShift;
//    ampmodem demodAM_USB;
};
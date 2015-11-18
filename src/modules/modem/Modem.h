#pragma once

#include "liquid/liquid.h"
#include "IOThread.h"
#include "AudioThread.h"

class ModemKit {
public:
    ModemKit() : sampleRate(0), audioSampleRate(0) {
        
    }
    
    long long sampleRate;
    int audioSampleRate;
};


class ModemIQData: public ReferenceCounter {
public:
    std::vector<liquid_float_complex> data;
    long long sampleRate;
    
    ModemIQData() : sampleRate(0) {
        
    }
    
    ~ModemIQData() {
        std::lock_guard < std::mutex > lock(m_mutex);
    }
};

class Modem;
typedef std::map<std::string,Modem *> ModemFactoryList;

class Modem  {
public:
    static void addModemFactory(std::string modemName, Modem *factorySingle);
    static ModemFactoryList getFactories();
    static Modem *makeModem(std::string modemType);
    
    virtual Modem *factory() = 0;

    virtual ModemKit *buildKit(long long sampleRate, int audioSampleRate) = 0;
    virtual void disposeKit(ModemKit *kit) = 0;
    virtual void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) = 0;
private:
    static ModemFactoryList modemFactories;
};
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
typedef Modem *(Modem::*ModemFactoryFunc)();
typedef std::map<std::string,ModemFactoryFunc *> ModemFactoryList;

class Modem  {
public:
    static void addModemFactory(std::string modemName, ModemFactoryFunc *factoryFunc);
    static ModemFactoryList getFactories();
    
    virtual Modem *factory();

    virtual ModemKit *buildKit(long long sampleRate, int audioSampleRate);
    virtual void disposeKit(ModemKit *kit);
    virtual void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
private:
    static ModemFactoryList modemFactories;
};
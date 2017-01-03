// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "Modem.h"
#include <map>
#include <vector>
#include <sstream>
#include <ostream>
#include <mutex>

class ModemKitDigital : public ModemKit {
public:
    ModemKitDigital() : ModemKit() {
        
    };
};

class ModemDigitalOutput {
public:
    ModemDigitalOutput();
    virtual ~ModemDigitalOutput();
    
    virtual void write(std::string outp) = 0;
    virtual void write(char outc) = 0;
    
    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual void Close() = 0;
    
private:
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

#if ENABLE_DIGITAL_LAB
    void setOutput(ModemDigitalOutput *digitalOutput);
#endif
    
protected:
    std::vector<unsigned int> demodOutputDataDigital;
    std::atomic_bool currentDemodLock;
#if ENABLE_DIGITAL_LAB
    ModemDigitalOutput *digitalOut;
    std::stringstream outStream;
#endif
};
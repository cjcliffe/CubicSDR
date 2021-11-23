// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "Modem.h"

class ModemIQ : public Modem {
public:
    ModemIQ();

    std::string getType() override;
    std::string getName() override;
    
    static ModemBase *factory();

    int checkSampleRate(long long sampleRate, int audioSampleRate) override;
    int getDefaultSampleRate() override;

    ModemKit *buildKit(long long sampleRate, int audioSampleRate) override;

    void disposeKit(ModemKit *kit) override;
    
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;
    
private:
    
};
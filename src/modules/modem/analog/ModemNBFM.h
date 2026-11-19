// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "Modem.h"
#include "ModemAnalog.h"
#include "SquelchState.h"

class ModemNBFM : public ModemAnalog {
public:
    ModemNBFM();
    ~ModemNBFM() override;
    
    std::string getName() override;
    
    static ModemBase *factory();

    int getDefaultSampleRate() override;

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;

    SquelchScheme getSquelchScheme() const override;
    float getSquelchMetric() const override;

private:
    freqdem demodFM;
    FmNoiseMeter noiseMeter;
    float squelchMetricDb = 0.0f;
};
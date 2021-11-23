// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"

class ModemQPSK : public ModemDigital {
public:
    ModemQPSK();
    ~ModemQPSK() override;
    
    std::string getName() override;
    
    static ModemBase *factory();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;
    
private:
    modemcf demodQPSK;
};

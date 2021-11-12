// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"

class ModemBPSK : public ModemDigital {
public:
    ModemBPSK();
    ~ModemBPSK() override;
    
    std::string getName() override;
    
    static ModemBase *factory();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;
    
private:
    modemcf demodBPSK;
};

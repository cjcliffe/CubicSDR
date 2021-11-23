// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemSQAM.h"

ModemSQAM::ModemSQAM() : ModemDigital()  {
    demodSQAM32 = modemcf_create(LIQUID_MODEM_SQAM32);
    demodSQAM128 = modemcf_create(LIQUID_MODEM_SQAM128);
    demodSQAM = demodSQAM32;
    cons = 32;
}

ModemBase *ModemSQAM::factory() {
    return new ModemSQAM;
}

ModemSQAM::~ModemSQAM() {
    modemcf_destroy(demodSQAM32);
    modemcf_destroy(demodSQAM128);
}

std::string ModemSQAM::getName() {
    return "SQAM";
}

ModemArgInfoList ModemSQAM::getSettings() {
    ModemArgInfoList args;
    
    ModemArgInfo consArg;
    consArg.key = "cons";
    consArg.name = "Constellation";
    consArg.description = "Modem Constellation Pattern";
    consArg.value = std::to_string(cons);
    consArg.type = ModemArgInfo::Type::STRING;
    std::vector<std::string> consOpts;
    consOpts.push_back("32");
    consOpts.push_back("128");
    consArg.options = consOpts;
    args.push_back(consArg);
    
    return args;
}

void ModemSQAM::writeSetting(std::string setting, std::string value) {
    if (setting == "cons") {
        int newCons = std::stoi(value);
        updateDemodulatorCons(newCons);
    }
}

std::string ModemSQAM::readSetting(std::string setting) {
    if (setting == "cons") {
        return std::to_string(cons);
    }
    return "";
}

void ModemSQAM::updateDemodulatorCons(int cons_in) {
    cons = cons_in;
    switch (cons_in) {
        case 32:
            demodSQAM = demodSQAM32;
            break;
        case 128:
            demodSQAM = demodSQAM128;
            break;
    }
}

void ModemSQAM::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput * /* audioOut */) {
    auto *dkit = (ModemKitDigital *)kit;

    digitalStart(dkit, demodSQAM, input);
    
    for (size_t i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modemcf_demodulate(demodSQAM, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodSQAM, 0.005f);
    
    digitalFinish(dkit, demodSQAM);
}

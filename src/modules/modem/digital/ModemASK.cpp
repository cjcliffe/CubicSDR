// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemASK.h"

ModemASK::ModemASK() : ModemDigital()  {
    demodASK2 = modem_create(LIQUID_MODEM_ASK2);
    demodASK4 = modem_create(LIQUID_MODEM_ASK4);
    demodASK8 = modem_create(LIQUID_MODEM_ASK8);
    demodASK16 = modem_create(LIQUID_MODEM_ASK16);
    demodASK32 = modem_create(LIQUID_MODEM_ASK32);
    demodASK64 = modem_create(LIQUID_MODEM_ASK64);
    demodASK128 = modem_create(LIQUID_MODEM_ASK128);
    demodASK256 = modem_create(LIQUID_MODEM_ASK256);
    demodASK = demodASK2;
    cons = 2;
}

ModemBase *ModemASK::factory() {
    return new ModemASK;
}

ModemASK::~ModemASK() {
    modem_destroy(demodASK4);
    modem_destroy(demodASK8);
    modem_destroy(demodASK16);
    modem_destroy(demodASK32);
    modem_destroy(demodASK64);
    modem_destroy(demodASK128);
    modem_destroy(demodASK256);
}

std::string ModemASK::getName() {
    return "ASK";
}

ModemArgInfoList ModemASK::getSettings() {
    ModemArgInfoList args;
    
    ModemArgInfo consArg;
    consArg.key = "cons";
    consArg.name = "Constellation";
    consArg.description = "Modem Constellation Pattern";
    consArg.value = std::to_string(cons);
    consArg.type = ModemArgInfo::STRING;
    std::vector<std::string> consOpts;
    consOpts.push_back("2");
    consOpts.push_back("4");
    consOpts.push_back("8");
    consOpts.push_back("16");
    consOpts.push_back("32");
    consOpts.push_back("64");
    consOpts.push_back("128");
    consOpts.push_back("256");
    consArg.options = consOpts;
    args.push_back(consArg);
    
    return args;
}

void ModemASK::writeSetting(std::string setting, std::string value) {
    if (setting == "cons") {
        int newCons = std::stoi(value);
        updateDemodulatorCons(newCons);
    }
}

std::string ModemASK::readSetting(std::string setting) {
    if (setting == "cons") {
        return std::to_string(cons);
    }
    return "";
}

void ModemASK::updateDemodulatorCons(int cons) {
    this->cons = cons;
    switch (cons) {
        case 2:
            demodASK = demodASK2;
            break;
        case 4:
            demodASK = demodASK4;
            break;
        case 8:
            demodASK = demodASK8;
            break;
        case 16:
            demodASK = demodASK16;
            break;
        case 32:
            demodASK = demodASK32;
            break;
        case 64:
            demodASK = demodASK64;
            break;
        case 128:
            demodASK = demodASK128;
            break;
        case 256:
            demodASK = demodASK256;
            break;
    }
}

void ModemASK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
    
    digitalStart(dkit, demodASK, input);

    for (size_t i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodASK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodASK, 0.005f);
    
    digitalFinish(dkit, demodASK);
}

// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemAPSK.h"

ModemAPSK::ModemAPSK() : ModemDigital() {
    demodAPSK4 = modem_create(LIQUID_MODEM_APSK4);
    demodAPSK8 = modem_create(LIQUID_MODEM_APSK8);
    demodAPSK16 = modem_create(LIQUID_MODEM_APSK16);
    demodAPSK32 = modem_create(LIQUID_MODEM_APSK32);
    demodAPSK64 = modem_create(LIQUID_MODEM_APSK64);
    demodAPSK128 = modem_create(LIQUID_MODEM_APSK128);
    demodAPSK256 = modem_create(LIQUID_MODEM_APSK256);
    demodAPSK = demodAPSK4;
    cons = 4;
}

ModemBase *ModemAPSK::factory() {
    return new ModemAPSK;
}

ModemAPSK::~ModemAPSK() {
    modem_destroy(demodAPSK4);
    modem_destroy(demodAPSK8);
    modem_destroy(demodAPSK16);
    modem_destroy(demodAPSK32);
    modem_destroy(demodAPSK64);
    modem_destroy(demodAPSK128);
    modem_destroy(demodAPSK256);
}

std::string ModemAPSK::getName() {
    return "APSK";
}

ModemArgInfoList ModemAPSK::getSettings() {
    ModemArgInfoList args;
    
    ModemArgInfo consArg;
    consArg.key = "cons";
    consArg.name = "Constellation";
    consArg.description = "Modem Constellation Pattern";
    consArg.value = std::to_string(cons);
    consArg.type = ModemArgInfo::STRING;
    std::vector<std::string> consOpts;
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

void ModemAPSK::writeSetting(std::string setting, std::string value) {
    if (setting == "cons") {
        int newCons = std::stoi(value);
        updateDemodulatorCons(newCons);
    }
}

std::string ModemAPSK::readSetting(std::string setting) {
    if (setting == "cons") {
        return std::to_string(cons);
    }
    return "";
}

void ModemAPSK::updateDemodulatorCons(int cons) {
    this->cons = cons;
    switch (cons) {
        case 4:
            demodAPSK = demodAPSK4;
            break;
        case 8:
            demodAPSK = demodAPSK8;
            break;
        case 16:
            demodAPSK = demodAPSK16;
            break;
        case 32:
            demodAPSK = demodAPSK32;
            break;
        case 64:
            demodAPSK = demodAPSK64;
            break;
        case 128:
            demodAPSK = demodAPSK128;
            break;
        case 256:
            demodAPSK = demodAPSK256;
            break;
    }
}

void ModemAPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
    
    digitalStart(dkit, demodAPSK, input);
    
    for (size_t i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodAPSK, input->data[i], &demodOutputDataDigital[i]);
    }
    
    updateDemodulatorLock(demodAPSK, 0.005f);
    
    digitalFinish(dkit, demodAPSK);
}

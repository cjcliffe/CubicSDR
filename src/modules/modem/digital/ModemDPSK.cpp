// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemDPSK.h"

ModemDPSK::ModemDPSK() : ModemDigital() {
    demodDPSK2 = modemcf_create(LIQUID_MODEM_DPSK2);
    demodDPSK4 = modemcf_create(LIQUID_MODEM_DPSK4);
    demodDPSK8 = modemcf_create(LIQUID_MODEM_DPSK8);
    demodDPSK16 = modemcf_create(LIQUID_MODEM_DPSK16);
    demodDPSK32 = modemcf_create(LIQUID_MODEM_DPSK32);
    demodDPSK64 = modemcf_create(LIQUID_MODEM_DPSK64);
    demodDPSK128 = modemcf_create(LIQUID_MODEM_DPSK128);
    demodDPSK256 = modemcf_create(LIQUID_MODEM_DPSK256);
    demodDPSK = demodDPSK2;
    cons = 2;
}

ModemBase *ModemDPSK::factory() {
    return new ModemDPSK;
}

std::string ModemDPSK::getName() {
    return "DPSK";
}

ModemDPSK::~ModemDPSK() {
    modemcf_destroy(demodDPSK2);
    modemcf_destroy(demodDPSK4);
    modemcf_destroy(demodDPSK8);
    modemcf_destroy(demodDPSK16);
    modemcf_destroy(demodDPSK32);
    modemcf_destroy(demodDPSK64);
    modemcf_destroy(demodDPSK128);
    modemcf_destroy(demodDPSK256);
}

ModemArgInfoList ModemDPSK::getSettings() {
    ModemArgInfoList args;
    
    ModemArgInfo consArg;
    consArg.key = "cons";
    consArg.name = "Constellation";
    consArg.description = "Modem Constellation Pattern";
    consArg.value = std::to_string(cons);
    consArg.type = ModemArgInfo::Type::STRING;
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

void ModemDPSK::writeSetting(std::string setting, std::string value) {
    if (setting == "cons") {
        int newCons = std::stoi(value);
        updateDemodulatorCons(newCons);
    }
}

std::string ModemDPSK::readSetting(std::string setting) {
    if (setting == "cons") {
        return std::to_string(cons);
    }
    return "";
}

void ModemDPSK::updateDemodulatorCons(int cons_in) {
    cons = cons_in;
    switch (cons_in) {
        case 2:
            demodDPSK = demodDPSK2;
            break;
        case 4:
            demodDPSK = demodDPSK4;
            break;
        case 8:
            demodDPSK = demodDPSK8;
            break;
        case 16:
            demodDPSK = demodDPSK16;
            break;
        case 32:
            demodDPSK = demodDPSK32;
            break;
        case 64:
            demodDPSK = demodDPSK64;
            break;
        case 128:
            demodDPSK = demodDPSK128;
            break;
        case 256:
            demodDPSK = demodDPSK256;
            break;
    }
}

void ModemDPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput * /* audioOut */) {
    auto *dkit = (ModemKitDigital *)kit;
   
    digitalStart(dkit, demodDPSK, input);
 
    for (size_t i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modemcf_demodulate(demodDPSK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodDPSK, 0.005f);
    
    digitalFinish(dkit, demodDPSK);
}

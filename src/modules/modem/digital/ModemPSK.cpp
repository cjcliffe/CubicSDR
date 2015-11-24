#include "ModemPSK.h"

ModemPSK::ModemPSK() {
    demodPSK2 = modem_create(LIQUID_MODEM_PSK2);
    demodPSK4 = modem_create(LIQUID_MODEM_PSK4);
    demodPSK8 = modem_create(LIQUID_MODEM_PSK8);
    demodPSK16 = modem_create(LIQUID_MODEM_PSK16);
    demodPSK32 = modem_create(LIQUID_MODEM_PSK32);
    demodPSK64 = modem_create(LIQUID_MODEM_PSK64);
    demodPSK128 = modem_create(LIQUID_MODEM_PSK128);
    demodPSK256 = modem_create(LIQUID_MODEM_PSK256);
    demodPSK = demodPSK2;
    cons = 2;
}

Modem *ModemPSK::factory() {
    return new ModemPSK;
}

std::string ModemPSK::getName() {
    return "PSK";
}

ModemPSK::~ModemPSK() {
    modem_destroy(demodPSK2);
    modem_destroy(demodPSK4);
    modem_destroy(demodPSK8);
    modem_destroy(demodPSK16);
    modem_destroy(demodPSK32);
    modem_destroy(demodPSK64);
    modem_destroy(demodPSK128);
    modem_destroy(demodPSK256);
}


ModemArgInfoList ModemPSK::getSettings() {
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

void ModemPSK::writeSetting(std::string setting, std::string value) {
    if (setting == "cons") {
        int newCons = std::stoi(value);
        updateDemodulatorCons(newCons);
    }
}

std::string ModemPSK::readSetting(std::string setting) {
    if (setting == "cons") {
        return std::to_string(cons);
    }
    return "";
}

void ModemPSK::updateDemodulatorCons(int cons) {
    this->cons = cons;
    switch (cons) {
        case 2:
            demodPSK = demodPSK2;
            break;
        case 4:
            demodPSK = demodPSK4;
            break;
        case 8:
            demodPSK = demodPSK8;
            break;
        case 16:
            demodPSK = demodPSK16;
            break;
        case 32:
            demodPSK = demodPSK32;
            break;
        case 64:
            demodPSK = demodPSK64;
            break;
        case 128:
            demodPSK = demodPSK128;
            break;
        case 256:
            demodPSK = demodPSK256;
            break;
    }
}

void ModemPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;

    digitalStart(dkit, demodPSK, input);
    
    for (int i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodPSK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodPSK, 0.005f);
    
    digitalFinish(dkit, demodPSK);
}
#include "ModemSQAM.h"

ModemSQAM::ModemSQAM() : ModemDigital()  {
    demodSQAM32 = modem_create(LIQUID_MODEM_SQAM32);
    demodSQAM128 = modem_create(LIQUID_MODEM_SQAM128);
    demodSQAM = demodSQAM32;
    cons = 32;
}

Modem *ModemSQAM::factory() {
    return new ModemSQAM;
}

ModemSQAM::~ModemSQAM() {
    modem_destroy(demodSQAM32);
    modem_destroy(demodSQAM128);
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
    consArg.type = ModemArgInfo::STRING;
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

void ModemSQAM::updateDemodulatorCons(int cons) {
    this->cons = cons;
    switch (cons) {
        case 32:
            demodSQAM = demodSQAM32;
            break;
        case 128:
            demodSQAM = demodSQAM128;
            break;
    }
}

void ModemSQAM::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;

    digitalStart(dkit, demodSQAM, input);
    
    for (int i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodSQAM, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodSQAM, 0.005f);
    
    digitalFinish(dkit, demodSQAM);
}
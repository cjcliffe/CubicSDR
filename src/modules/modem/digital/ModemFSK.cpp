#include "ModemFSK.h"

ModemFSK::ModemFSK() {
    bps = 9600;
    spacing = 7000;
}

Modem *ModemFSK::factory() {
    return new ModemFSK;
}

ModemArgInfoList ModemFSK::getSettings() {
    ModemArgInfoList args;
    
    return args;
}

void ModemFSK::writeSetting(std::string setting, std::string value) {
    if (setting == "bps") {
        
    } else if (setting == "spacing") {
        
    }
}

std::string ModemFSK::readSetting(std::string setting) {
    if (setting == "bps") {
        return std::to_string(bps);
    } else if (setting == "spacing") {
        return std::to_string(spacing);
    }
    return "";
}

ModemKit *ModemFSK::buildKit(long long sampleRate, int audioSampleRate) {
    ModemKitFSK *dkit = new ModemKitFSK;
    dkit->m           = 1;
    dkit->k           = sampleRate / 9600;
    dkit->bandwidth   = 7000.0 / sampleRate;

    dkit->demodFSK = fskdem_create(dkit->m, dkit->k, dkit->bandwidth);

    dkit->sampleRate = sampleRate;
    dkit->audioSampleRate = audioSampleRate;
 
    return dkit;
}

void ModemFSK::disposeKit(ModemKit *kit) {
    ModemKitFSK *dkit = (ModemKitFSK *)kit;
    
    delete dkit;
}

std::string ModemFSK::getName() {
    return "FSK";
}

ModemFSK::~ModemFSK() {

}

void ModemFSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitFSK *dkit = (ModemKitFSK *)kit;
    
    digitalStart(dkit, nullptr, input);

    dkit->inputBuffer.insert(dkit->inputBuffer.end(),input->data.begin(),input->data.end());

    while (dkit->inputBuffer.size() >= dkit->k) {
        unsigned int sym_out;

        sym_out = fskdem_demodulate(dkit->demodFSK, &dkit->inputBuffer[0]);
//        std::cout << "ferror: " << fskdem_get_frequency_error(dkit->demodFSK) << std::endl;
        printf("%01X", sym_out);
        dkit->inputBuffer.erase(dkit->inputBuffer.begin(),dkit->inputBuffer.begin()+dkit->k);
    }
    
    digitalFinish(dkit, nullptr);
}
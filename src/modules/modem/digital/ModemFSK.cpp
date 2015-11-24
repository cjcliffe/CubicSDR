#include "ModemFSK.h"

ModemFSK::ModemFSK() {
    // DMR defaults?
    bps = 9600;
    spacing = 7000;
}

Modem *ModemFSK::factory() {
    return new ModemFSK;
}

ModemArgInfoList ModemFSK::getSettings() {
    ModemArgInfoList args;
    
    ModemArgInfo bpsArg;
    bpsArg.key = "bps";
    bpsArg.name = "Bits per-symbol";
    bpsArg.value = std::to_string(bps);
    bpsArg.description = "FSK modem bits-per-symbol";
    bpsArg.type = ModemArgInfo::STRING;
    
    std::vector<std::string> bpsOpts;
    bpsOpts.push_back("9600");
    bpsArg.options = bpsOpts;
    
    args.push_back(bpsArg);

    ModemArgInfo spacingArg;
    spacingArg.key = "spacing";
    spacingArg.name = "Symbol Spacing?";
    spacingArg.description = "Not quite sure yet :-)";
    spacingArg.type = ModemArgInfo::STRING;
    spacingArg.value = std::to_string(spacing);

    std::vector<std::string> spacingOpts;
    spacingOpts.push_back("7000");
    spacingArg.options = spacingOpts;
    
    args.push_back(spacingArg);
    
    return args;
}

void ModemFSK::writeSetting(std::string setting, std::string value) {
    if (setting == "bps") {
        bps = std::stoi(value);
        rebuildKit();
    } else if (setting == "spacing") {
        spacing = std::stoi(value);
        rebuildKit();
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
    dkit->k           = sampleRate / bps;
    dkit->bandwidth   = double(spacing) / sampleRate;

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
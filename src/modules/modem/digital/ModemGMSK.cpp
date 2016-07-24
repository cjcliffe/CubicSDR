#include "ModemGMSK.h"
#include <iomanip>

ModemGMSK::ModemGMSK() : ModemDigital()  {
    _sps = 4;
    _fdelay = 3;
    _ebf = 0.3;
    outStream << std::hex;
}

ModemGMSK::~ModemGMSK() {
    
}

std::string ModemGMSK::getName() {
    return "GMSK";
}

ModemBase *ModemGMSK::factory() {
    return new ModemGMSK;
}

int ModemGMSK::checkSampleRate(long long sampleRate, int audioSampleRate) {
    if (sampleRate < MIN_BANDWIDTH) {
        return MIN_BANDWIDTH;
    }
    return sampleRate;
}

int ModemGMSK::getDefaultSampleRate() {
    return 19200;
}

ModemArgInfoList ModemGMSK::getSettings() {
    ModemArgInfoList args;
    
    ModemArgInfo fdelayArg;
    fdelayArg.key = "fdelay";
    fdelayArg.name = "Filter delay";
    fdelayArg.value = std::to_string(_fdelay);
    fdelayArg.description = "Filter delay in samples";
    fdelayArg.type = ModemArgInfo::INT;
    fdelayArg.units = "samples";
    fdelayArg.range = ModemRange(1,128);
    args.push_back(fdelayArg);
    
    ModemArgInfo spsArg;
    spsArg.key = "sps";
    spsArg.name = "Samples / symbol";
    spsArg.value = std::to_string(_sps);
    spsArg.description = "Modem samples-per-symbol";
    spsArg.type = ModemArgInfo::INT;
    spsArg.units = "samples/symbol";
    spsArg.range = ModemRange(2,512);
    args.push_back(spsArg);
    
    ModemArgInfo ebfArg;
    ebfArg.key = "ebf";
    ebfArg.name = "Excess bandwidth";
    ebfArg.value = std::to_string(_ebf);
    ebfArg.description = "Modem excess bandwidth factor";
    ebfArg.type = ModemArgInfo::FLOAT;
    ebfArg.range = ModemRange(0.1,0.49);
    args.push_back(ebfArg);

    return args;
}

void ModemGMSK::writeSetting(std::string setting, std::string value) {
    if (setting == "fdelay") {
        _fdelay = std::stoi(value);
        rebuildKit();
    } else if (setting == "sps") {
        _sps = std::stoi(value);
        rebuildKit();
    } else if (setting == "ebf") {
        _ebf = std::stof(value);
        rebuildKit();
    }
}

std::string ModemGMSK::readSetting(std::string setting) {
    if (setting == "fdelay") {
        return std::to_string(_fdelay);
    } else if (setting == "sps") {
        return std::to_string(_sps);
    } else if (setting == "ebf") {
        return std::to_string(_ebf);
    }
    return "";
}

ModemKit *ModemGMSK::buildKit(long long sampleRate, int audioSampleRate) {
    ModemKitGMSK *dkit = new ModemKitGMSK;
    dkit->sps    = _sps;
    dkit->fdelay = _fdelay;
    dkit->ebf    = _ebf;

    dkit->demodGMSK = gmskdem_create(dkit->sps, dkit->fdelay, dkit->ebf);

    dkit->sampleRate = sampleRate;
    dkit->audioSampleRate = audioSampleRate;
 
    return dkit;
}

void ModemGMSK::disposeKit(ModemKit *kit) {
    ModemKitGMSK *dkit = (ModemKitGMSK *)kit;
    
    gmskdem_destroy(dkit->demodGMSK);
    
    delete dkit;
}

void ModemGMSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitGMSK *dkit = (ModemKitGMSK *)kit;
    unsigned int sym_out;
    
    digitalStart(dkit, nullptr, input);

    dkit->inputBuffer.insert(dkit->inputBuffer.end(),input->data.begin(),input->data.end());

    int numProcessed = 0;
    for (size_t i = 0, iMax = dkit->inputBuffer.size()/dkit->sps; i < iMax; i+= dkit->sps) {
        gmskdem_demodulate(dkit->demodGMSK, &input->data[i],&sym_out);
        outStream << sym_out;
        numProcessed += dkit->sps;
    }
    
    dkit->inputBuffer.erase(dkit->inputBuffer.begin(),dkit->inputBuffer.begin()+numProcessed);
    
    digitalFinish(dkit, nullptr);
}

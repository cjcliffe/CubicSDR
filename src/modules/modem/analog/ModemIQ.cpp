#include "ModemIQ.h"

ModemIQ::ModemIQ() {
    
}

std::string ModemIQ::getType() {
    return "analog";
}

std::string ModemIQ::getName() {
    return "I/Q";
}

ModemBase *ModemIQ::factory() {
    return new ModemIQ;
}

ModemKit *ModemIQ::buildKit(long long sampleRate, int audioSampleRate) {
    ModemKit *kit = new ModemKit;
    kit->sampleRate = sampleRate;
    kit->audioSampleRate = audioSampleRate;
    return kit;
}

void ModemIQ::disposeKit(ModemKit *kit) {
    delete kit;
}

int ModemIQ::checkSampleRate(long long /* sampleRate */, int audioSampleRate) {
    return audioSampleRate;
}

int ModemIQ::getDefaultSampleRate() {
    return 48000;
}

void ModemIQ::demodulate(ModemKit * /* kit */, ModemIQData *input, AudioThreadInput *audioOut) {
    size_t bufSize = input->data.size();
    
    if (!bufSize) {
        input->decRefCount();
        return;
    }
    
    audioOut->channels = 2;
    if (audioOut->data.capacity() < (bufSize * 2)) {
        audioOut->data.reserve(bufSize * 2);
    }
    
    audioOut->data.resize(bufSize * 2);
    for (size_t i = 0; i < bufSize; i++) {
        audioOut->data[i * 2] = input->data[i].imag;
        audioOut->data[i * 2 + 1] = input->data[i].real;
    }
}

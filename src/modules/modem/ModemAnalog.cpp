#include "ModemAnalog.h"

ModemAnalog::ModemAnalog() : aOutputCeil(1), aOutputCeilMA(1), aOutputCeilMAA(1) {
    
}

std::string ModemAnalog::getType() {
    return "analog";
}

int ModemAnalog::checkSampleRate(long long sampleRate, int /* audioSampleRate */) {
    if (sampleRate < MIN_BANDWIDTH) {
        return MIN_BANDWIDTH;
    }
    return sampleRate;
}

ModemKit *ModemAnalog::buildKit(long long sampleRate, int audioSampleRate) {
    ModemKitAnalog *akit = new ModemKitAnalog;
    
    // stop-band attenuation [dB]
    float As = 60.0f;
    
    akit->sampleRate = sampleRate;
    akit->audioSampleRate = audioSampleRate;
    akit->audioResampleRatio = double(audioSampleRate) / double(sampleRate);
    akit->audioResampler = msresamp_rrrf_create(akit->audioResampleRatio, As);
    
    return akit;
}

void ModemAnalog::disposeKit(ModemKit *kit) {
    ModemKitAnalog *akit = (ModemKitAnalog *)kit;
    
    msresamp_rrrf_destroy(akit->audioResampler);
    delete akit;
}

void ModemAnalog::initOutputBuffers(ModemKitAnalog *akit, ModemIQData *input) {
    bufSize = input->data.size();
    
    if (!bufSize) {
        return;
    }
    
    double audio_resample_ratio = akit->audioResampleRatio;
    
    size_t audio_out_size = ceil((double) (bufSize) * audio_resample_ratio) + 512;
    
    if (demodOutputData.size() != bufSize) {
        if (demodOutputData.capacity() < bufSize) {
            demodOutputData.reserve(bufSize);
        }
        demodOutputData.resize(bufSize);
    }
    if (resampledOutputData.size() != audio_out_size) {
        if (resampledOutputData.capacity() < audio_out_size) {
            resampledOutputData.reserve(audio_out_size);
        }
        resampledOutputData.resize(audio_out_size);
    }
}

void ModemAnalog::buildAudioOutput(ModemKitAnalog *akit, AudioThreadInput *audioOut, bool autoGain) {
    unsigned int numAudioWritten;

    if (autoGain) {
        aOutputCeilMA = aOutputCeilMA + (aOutputCeil - aOutputCeilMA) * 0.025;
        aOutputCeilMAA = aOutputCeilMAA + (aOutputCeilMA - aOutputCeilMAA) * 0.025;
        aOutputCeil = 0;
        
        for (size_t i = 0; i < bufSize; i++) {
            if (demodOutputData[i] > aOutputCeil) {
                aOutputCeil = demodOutputData[i];
            }
        }
        
        float gain = 0.5 / aOutputCeilMAA;
        
        for (size_t i = 0; i < bufSize; i++) {
            demodOutputData[i] *= gain;
        }
    }
    
    msresamp_rrrf_execute(akit->audioResampler, &demodOutputData[0], demodOutputData.size(), &resampledOutputData[0], &numAudioWritten);
    
    audioOut->channels = 1;
    audioOut->sampleRate = akit->audioSampleRate;
    audioOut->data.assign(resampledOutputData.begin(), resampledOutputData.begin() + numAudioWritten);
}

std::vector<float> *ModemAnalog::getDemodOutputData() {
    return &demodOutputData;
}

std::vector<float> *ModemAnalog::getResampledOutputData() {
    return &resampledOutputData;
}

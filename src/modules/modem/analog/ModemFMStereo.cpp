// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemFMStereo.h"
#include "ResampleGuard.h"
#include <iostream>

// Master output trim (-4.9 dB headroom); M/S balance is unity by construction.
static const float STEREO_OUT_GAIN = 0.568f;

ModemFMStereo::ModemFMStereo() {
    demodFM = freqdem_create(0.5);
    _demph = 75;
}

ModemFMStereo::~ModemFMStereo() {
    freqdem_destroy(demodFM);
}

std::string ModemFMStereo::getType() {
    return "analog";
}

std::string ModemFMStereo::getName() {
    return "FMS";
}

ModemBase *ModemFMStereo::factory() {
    return new ModemFMStereo;
}

int ModemFMStereo::checkSampleRate(long long sampleRate, int /* audioSampleRate */) {
    if (sampleRate < 100000) {
        return 100000;
    } else if (sampleRate < 1500) {
        return 1500;
    } else {
        return (int)sampleRate;
    }
}

int ModemFMStereo::getDefaultSampleRate() {
    return 200000;
}

ModemArgInfoList ModemFMStereo::getSettings() {
    ModemArgInfoList args;
    
    ModemArgInfo demphArg;
    demphArg.key = "demph";
    demphArg.name = "De-emphasis";
    demphArg.value = std::to_string(_demph);
    demphArg.description = "FM Stereo De-Emphasis, typically 75us in US/Canada, 50us elsewhere.";
    
    demphArg.type = ModemArgInfo::Type::STRING;
    
    std::vector<std::string> demphOptNames;
    demphOptNames.push_back("None");
    demphOptNames.push_back("10us");
    demphOptNames.push_back("25us");
    demphOptNames.push_back("32us");
    demphOptNames.push_back("50us");
    demphOptNames.push_back("75us");
    demphArg.optionNames = demphOptNames;
    
    std::vector<std::string> demphOpts;
    demphOpts.push_back("0");
    demphOpts.push_back("10");
    demphOpts.push_back("25");
    demphOpts.push_back("32");
    demphOpts.push_back("50");
    demphOpts.push_back("75");
    demphArg.options = demphOpts;

    args.push_back(demphArg);
    
    return args;
}

void ModemFMStereo::writeSetting(std::string setting, std::string value) {
    if (setting == "demph") {
        _demph = std::stoi(value);
        rebuildKit();
    }
}

std::string ModemFMStereo::readSetting(std::string setting) {
    if (setting == "demph") {
        return std::to_string(_demph);
    }
    return "";
}

ModemKit *ModemFMStereo::buildKit(long long sampleRate, int audioSampleRate) {
    auto *kit = new ModemKitFMStereo;
    
    kit->audioResampleRatio = clampResampleRatio(double(audioSampleRate) / double(sampleRate));
    kit->sampleRate = sampleRate;
    kit->audioSampleRate = audioSampleRate;
   
    float As = 60.0f;         // stop-band attenuation [dB]
    
    kit->audioResampler = msresamp_rrrf_create((float)kit->audioResampleRatio, As);
    kit->stereoResampler = msresamp_rrrf_create((float)kit->audioResampleRatio, As);
    if (!kit->audioResampler || !kit->stereoResampler) {
        std::cout << "ModemFMStereo: msresamp_create failed; audio muted." << std::endl << std::flush;
    }
    
    // Stereo filters / shifters
    float firStereoCutoff = 16000.0f / float(audioSampleRate);
    // filter transition
    float ft = 1000.0f / float(audioSampleRate);
    // fractional timing offset
    float mu = 0.0f;
    
    if (firStereoCutoff < 0) {
        firStereoCutoff = 0;
    }
    
    if (firStereoCutoff > 0.5) {
        firStereoCutoff = 0.5;
    }
    
    unsigned int h_len = estimate_req_filter_len(ft, As);
    float *h = new float[h_len];
    liquid_firdes_kaiser(h_len, firStereoCutoff, As, mu, h);

    // The designed taps are not unity-gain: their sum grows with length, so
    // without this the filter amplifies more at higher audio rates.
    double tapSum = 0.0;
    for (unsigned int i = 0; i < h_len; i++) {
        tapSum += h[i];
    }
    if (tapSum != 0.0) {
        float invSum = (float)(1.0 / tapSum);
        for (unsigned int i = 0; i < h_len; i++) {
            h[i] *= invSum;
        }
    }
    
    kit->firStereoLeft = firfilt_rrrf_create(h, h_len);
    kit->firStereoRight = firfilt_rrrf_create(h, h_len);

    delete[] h;
    
    // stereo pilot filter
    float bw = float(sampleRate);
    if (bw < 100000.0) {
        bw = 100000.0;
    }
    unsigned int order =   5;       // filter order
    float        f0    =   ((float) 19000 / bw);
    float        fc    =   ((float) 19500 / bw);
    float        Ap    =   1.0f;
    
    kit->iirStereoPilot = iirfilt_crcf_create_prototype(LIQUID_IIRDES_CHEBY2, LIQUID_IIRDES_BANDPASS, LIQUID_IIRDES_SOS, order, fc, f0, Ap, As);

    // Measure the pilot filter phase at 19 kHz through a scratch twin so
    // the doubled PLL reference lands in phase; see stereoReferencePhase.
    iirfilt_crcf pilotProbe = iirfilt_crcf_create_prototype(LIQUID_IIRDES_CHEBY2, LIQUID_IIRDES_BANDPASS, LIQUID_IIRDES_SOS, order, fc, f0, Ap, As);
    float phaseAccR = 0.0f, phaseAccI = 0.0f;
    float pilotOm = (float)(2.0 * M_PI * 19000.0 / (double)sampleRate);
    for (int n = 0; n < 576; n++) {
        liquid_float_complex toneIn, toneOut;
        toneIn.real = cosf(pilotOm * (float)n);
        toneIn.imag = sinf(pilotOm * (float)n);
        iirfilt_crcf_execute(pilotProbe, toneIn, &toneOut);
        if (n >= 512) {
            phaseAccR += toneOut.real * toneIn.real + toneOut.imag * toneIn.imag;
            phaseAccI += toneOut.imag * toneIn.real - toneOut.real * toneIn.imag;
        }
    }
    iirfilt_crcf_destroy(pilotProbe);
    float pilotPhase = (phaseAccR != 0.0f || phaseAccI != 0.0f) ? atan2f(phaseAccI, phaseAccR) : 0.0f;
    kit->stereoRefPhase = stereoReferencePhase(pilotPhase);

    kit->firStereoR2C = firhilbf_create(5, 60.0f);
    kit->firStereoC2R = firhilbf_create(5, 60.0f);
    
    kit->stereoPilot = nco_crcf_create(LIQUID_VCO);
    nco_crcf_reset(kit->stereoPilot);
    // Loop gains derive straight from bandwidth (alpha=bw, beta=sqrt(bw));
    // Narrow enough to keep noise out of the 38 kHz reference, wide enough to acquire in ms.
    nco_crcf_pll_set_bandwidth(kit->stereoPilot, 0.02f);
    
    kit->demph = _demph;
    
    if (_demph) {
        double f = (1.0 / (2.0 * M_PI * double(_demph) * 1e-6));
        double t = 1.0 / (2.0 * M_PI * f);
        t = 1.0 / (2.0 * double(audioSampleRate) * tan(1.0 / (2.0 * double(audioSampleRate) * t)));
        
        double tb = (1.0 + 2.0 * t * double(audioSampleRate));
        float b_demph[2] = { (float)(1.0 / tb), (float)(1.0 / tb) };
        float a_demph[2] = { 1.0, (float)((1.0 - 2.0 * t * double(audioSampleRate)) / tb) };
        
        kit->iirDemphL = iirfilt_rrrf_create(b_demph, 2, a_demph, 2);
        kit->iirDemphR = iirfilt_rrrf_create(b_demph, 2, a_demph, 2);
    } else {
        kit->iirDemphL = nullptr;
        kit->iirDemphR = nullptr;
        kit->demph = 0;
    }

    kit->monoDelay.setDelay(STEREO_PATH_DELAY);
    kit->blender.setRates((double)sampleRate, 20.0f, 200.0f);
    return kit;
}

void ModemFMStereo::disposeKit(ModemKit *kit) {
    auto *fmkit = (ModemKitFMStereo *)kit;
    
    if (fmkit->audioResampler) {
        msresamp_rrrf_destroy(fmkit->audioResampler);
    }
    if (fmkit->stereoResampler) {
        msresamp_rrrf_destroy(fmkit->stereoResampler);
    }
    firfilt_rrrf_destroy(fmkit->firStereoLeft);
    firfilt_rrrf_destroy(fmkit->firStereoRight);
    iirfilt_crcf_destroy(fmkit->iirStereoPilot);
    firhilbf_destroy(fmkit->firStereoR2C);
    firhilbf_destroy(fmkit->firStereoC2R);
    nco_crcf_destroy(fmkit->stereoPilot);
    if (fmkit->iirDemphR) { iirfilt_rrrf_destroy(fmkit->iirDemphR); }
    if (fmkit->iirDemphL) { iirfilt_rrrf_destroy(fmkit->iirDemphL); }
}


SquelchScheme ModemFMStereo::getSquelchScheme() const {
    return SquelchScheme::Noise;
}

float ModemFMStereo::getSquelchMetric() const {
    return squelchMetricDb;
}

void ModemFMStereo::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    auto *fmkit = (ModemKitFMStereo *)kit;
    size_t bufSize = input->data.size();
    liquid_float_complex u, v, w, x, y;
    
    double audio_resample_ratio = fmkit->audioResampleRatio;
    
    if (demodOutputData.size() != bufSize) {
        if (demodOutputData.capacity() < bufSize) {
            demodOutputData.reserve(bufSize);
        }
        demodOutputData.resize(bufSize);
    }
    
    size_t audio_out_size = (size_t)ceil((double) (bufSize) * audio_resample_ratio) + 512;
    
    freqdem_demodulate_block(demodFM, &input->data[0], (int)bufSize, &demodOutputData[0]);
    squelchMetricDb = noiseMeter.process(&demodOutputData[0], bufSize, (float)input->sampleRate);

    if (monoDelayedData.size() != bufSize) {
        if (monoDelayedData.capacity() < bufSize) {
            monoDelayedData.reserve(bufSize);
        }
        monoDelayedData.resize(bufSize);
    }
    fmkit->monoDelay.apply(&demodOutputData[0], bufSize, &monoDelayedData[0]);

    if (resampledOutputData.size() != audio_out_size) {
        if (resampledOutputData.capacity() < audio_out_size) {
            resampledOutputData.reserve(audio_out_size);
        }
        resampledOutputData.resize(audio_out_size);
    }
    
    unsigned int numAudioWritten;
    bool haveResamplers = fmkit->audioResampler && fmkit->stereoResampler;

    if (haveResamplers) {
        msresamp_rrrf_execute(fmkit->audioResampler, &monoDelayedData[0], (int)bufSize, &resampledOutputData[0], &numAudioWritten);
    }
    
    if (demodStereoData.size() != bufSize) {
        if (demodStereoData.capacity() < bufSize) {
            demodStereoData.reserve(bufSize);
        }
        demodStereoData.resize(bufSize);
    }
    
    float phase_error = 0;
    
    for (size_t i = 0; i < bufSize; i++) {
        // real -> complex
        firhilbf_r2c_execute(fmkit->firStereoR2C, demodOutputData[i], &x);
        
        // 19khz pilot band-pass
        iirfilt_crcf_execute(fmkit->iirStereoPilot, x, &v);
        nco_crcf_cexpf(fmkit->stereoPilot, &w);
        
        w.imag = -w.imag; // conjf(w)
        
        // multiply u = v * conjf(w)
        u.real = v.real * w.real - v.imag * w.imag;
        u.imag = v.real * w.imag + v.imag * w.real;
        
        // cargf(u), offset so the doubled reference lands in phase
        phase_error = atan2f(u.imag,u.real) - fmkit->stereoRefPhase;
        
        // step pll
        nco_crcf_pll_step(fmkit->stereoPilot, phase_error);
        nco_crcf_step(fmkit->stereoPilot);
        
        // 38khz down-mix
        nco_crcf_mix_down(fmkit->stereoPilot, x, &y);
        nco_crcf_mix_down(fmkit->stereoPilot, y, &x);
        
        // complex -> real
        float usb_discard;
        firhilbf_c2r_execute(fmkit->firStereoC2R, x, &demodStereoData[i], &usb_discard);
        demodStereoData[i] *= fmkit->blender.process(v.real, v.imag);
    }
    
    //            std::cout << "[PLL] phase error: " << phase_error;
    //            std::cout << " freq:" << (((nco_crcf_get_frequency(stereoPilot) / (2.0 * M_PI)) * inp->sampleRate)) << std::endl;
    
    if (audio_out_size != resampledStereoData.size()) {
        if (resampledStereoData.capacity() < audio_out_size) {
            resampledStereoData.reserve(audio_out_size);
        }
        resampledStereoData.resize(audio_out_size);
    }
    
    if (haveResamplers) {
        msresamp_rrrf_execute(fmkit->stereoResampler, &demodStereoData[0], (int)bufSize, &resampledStereoData[0], &numAudioWritten);
    } else {
        numAudioWritten = 0;
    }
    
    audioOut->channels = 2;
    if (audioOut->data.capacity() < (numAudioWritten * 2)) {
        audioOut->data.reserve(numAudioWritten * 2);
    }
    audioOut->data.resize(numAudioWritten * 2);
    // The calibrated reference decodes +S, so L = M + S and R = M - S.
    for (size_t i = 0; i < numAudioWritten; i++) {
        float l, r;
        float ld, rd;

        if (fmkit->demph) {
            iirfilt_rrrf_execute(fmkit->iirDemphL, STEREO_OUT_GAIN * (resampledOutputData[i] + (resampledStereoData[i])), &ld);
            iirfilt_rrrf_execute(fmkit->iirDemphR, STEREO_OUT_GAIN * (resampledOutputData[i] - (resampledStereoData[i])), &rd);
            
            firfilt_rrrf_push(fmkit->firStereoLeft, ld);
            firfilt_rrrf_execute(fmkit->firStereoLeft, &l);
            
            firfilt_rrrf_push(fmkit->firStereoRight, rd);
            firfilt_rrrf_execute(fmkit->firStereoRight, &r);
        } else {
            firfilt_rrrf_push(fmkit->firStereoLeft, STEREO_OUT_GAIN * (resampledOutputData[i] + (resampledStereoData[i])));
            firfilt_rrrf_execute(fmkit->firStereoLeft, &l);
            
            firfilt_rrrf_push(fmkit->firStereoRight, STEREO_OUT_GAIN * (resampledOutputData[i] - (resampledStereoData[i])));
            firfilt_rrrf_execute(fmkit->firStereoRight, &r);
        }
        
        audioOut->data[i * 2] = l;
        audioOut->data[i * 2 + 1] = r;
    }
}

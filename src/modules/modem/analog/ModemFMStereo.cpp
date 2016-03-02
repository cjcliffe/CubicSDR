#include "ModemFMStereo.h"

ModemFMStereo::ModemFMStereo() {
    demodFM = freqdem_create(0.5);
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

Modem *ModemFMStereo::factory() {
    return new ModemFMStereo;
}

int ModemFMStereo::checkSampleRate(long long sampleRate, int /* audioSampleRate */) {
    if (sampleRate < 100000) {
        return 100000;
    } else if (sampleRate < 1500) {
        return 1500;
    } else {
        return sampleRate;
    }
}

int ModemFMStereo::getDefaultSampleRate() {
    return 200000;
}

ModemKit *ModemFMStereo::buildKit(long long sampleRate, int audioSampleRate) {
    ModemKitFMStereo *kit = new ModemKitFMStereo;
    
    kit->audioResampleRatio = double(audioSampleRate) / double(sampleRate);
    kit->sampleRate = sampleRate;
    kit->audioSampleRate = audioSampleRate;
   
    float As = 60.0f;         // stop-band attenuation [dB]
    
    kit->audioResampler = msresamp_rrrf_create(kit->audioResampleRatio, As);
    kit->stereoResampler = msresamp_rrrf_create(kit->audioResampleRatio, As);
    
    // Stereo filters / shifters
    double firStereoCutoff = 16000.0 / double(audioSampleRate);
    // filter transition
    float ft = 1000.0 / double(audioSampleRate);
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
    
    kit->firStereoLeft = firfilt_rrrf_create(h, h_len);
    kit->firStereoRight = firfilt_rrrf_create(h, h_len);
    
    // stereo pilot filter
    float bw = sampleRate;
    if (bw < 100000.0) {
        bw = 100000.0;
    }
    unsigned int order =   5;       // filter order
    float        f0    =   ((double) 19000 / bw);
    float        fc    =   ((double) 19500 / bw);
    float        Ap    =   1.0f;
    
    kit->iirStereoPilot = iirfilt_crcf_create_prototype(LIQUID_IIRDES_CHEBY2, LIQUID_IIRDES_BANDPASS, LIQUID_IIRDES_SOS, order, fc, f0, Ap, As);
    
    kit->firStereoR2C = firhilbf_create(5, 60.0f);
    kit->firStereoC2R = firhilbf_create(5, 60.0f);
    
    kit->stereoPilot = nco_crcf_create(LIQUID_VCO);
    nco_crcf_reset(kit->stereoPilot);
    nco_crcf_pll_set_bandwidth(kit->stereoPilot, 0.25f);
    
    return kit;
}

void ModemFMStereo::disposeKit(ModemKit *kit) {
    ModemKitFMStereo *fmkit = (ModemKitFMStereo *)kit;
    
    msresamp_rrrf_destroy(fmkit->audioResampler);
    msresamp_rrrf_destroy(fmkit->stereoResampler);
    firfilt_rrrf_destroy(fmkit->firStereoLeft);
    firfilt_rrrf_destroy(fmkit->firStereoRight);
    firhilbf_destroy(fmkit->firStereoR2C);
    firhilbf_destroy(fmkit->firStereoC2R);
    nco_crcf_destroy(fmkit->stereoPilot);
}


void ModemFMStereo::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitFMStereo *fmkit = (ModemKitFMStereo *)kit;
    size_t bufSize = input->data.size();
    liquid_float_complex u, v, w, x, y;
    
    double audio_resample_ratio = fmkit->audioResampleRatio;
    
    if (demodOutputData.size() != bufSize) {
        if (demodOutputData.capacity() < bufSize) {
            demodOutputData.reserve(bufSize);
        }
        demodOutputData.resize(bufSize);
    }
    
    size_t audio_out_size = ceil((double) (bufSize) * audio_resample_ratio) + 512;
    
    freqdem_demodulate_block(demodFM, &input->data[0], bufSize, &demodOutputData[0]);
    
    if (resampledOutputData.size() != audio_out_size) {
        if (resampledOutputData.capacity() < audio_out_size) {
            resampledOutputData.reserve(audio_out_size);
        }
        resampledOutputData.resize(audio_out_size);
    }
    
    unsigned int numAudioWritten;
    
    msresamp_rrrf_execute(fmkit->audioResampler, &demodOutputData[0], bufSize, &resampledOutputData[0], &numAudioWritten);
    
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
        
        // cargf(u)
        phase_error = atan2f(u.imag,u.real);
        
        // step pll
        nco_crcf_pll_step(fmkit->stereoPilot, phase_error);
        nco_crcf_step(fmkit->stereoPilot);
        
        // 38khz down-mix
        nco_crcf_mix_down(fmkit->stereoPilot, x, &y);
        nco_crcf_mix_down(fmkit->stereoPilot, y, &x);
        
        // complex -> real
        firhilbf_c2r_execute(fmkit->firStereoC2R, x, &demodStereoData[i]);
    }
    
    //            std::cout << "[PLL] phase error: " << phase_error;
    //            std::cout << " freq:" << (((nco_crcf_get_frequency(stereoPilot) / (2.0 * M_PI)) * inp->sampleRate)) << std::endl;
    
    if (audio_out_size != resampledStereoData.size()) {
        if (resampledStereoData.capacity() < audio_out_size) {
            resampledStereoData.reserve(audio_out_size);
        }
        resampledStereoData.resize(audio_out_size);
    }
    
    msresamp_rrrf_execute(fmkit->stereoResampler, &demodStereoData[0], bufSize, &resampledStereoData[0], &numAudioWritten);
    
    audioOut->channels = 2;
    if (audioOut->data.capacity() < (numAudioWritten * 2)) {
        audioOut->data.reserve(numAudioWritten * 2);
    }
    audioOut->data.resize(numAudioWritten * 2);
    for (size_t i = 0; i < numAudioWritten; i++) {
        float l, r;
        
        firfilt_rrrf_push(fmkit->firStereoLeft, 0.568 * (resampledOutputData[i] - (resampledStereoData[i])));
        firfilt_rrrf_execute(fmkit->firStereoLeft, &l);
        
        firfilt_rrrf_push(fmkit->firStereoRight, 0.568 * (resampledOutputData[i] + (resampledStereoData[i])));
        firfilt_rrrf_execute(fmkit->firStereoRight, &r);
        
        audioOut->data[i * 2] = l;
        audioOut->data[i * 2 + 1] = r;
    }
}

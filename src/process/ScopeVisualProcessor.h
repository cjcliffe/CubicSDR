#pragma once

#include "VisualProcessor.h"
#include "AudioThread.h"
#include "ScopePanel.h"

#if USE_FFTW3
#include "fftw3.h"
#endif

class ScopeRenderData: public ReferenceCounter {
public:
	std::vector<float> waveform_points;
    ScopePanel::ScopeMode mode;
    int inputRate;
    int sampleRate;
	int channels;
    bool spectrum;
    int fft_size;
    double fft_floor, fft_ceil;
};

typedef ThreadQueue<ScopeRenderData *> ScopeRenderDataQueue;

class ScopeVisualProcessor : public VisualProcessor<AudioThreadInput, ScopeRenderData> {
public:
    ScopeVisualProcessor();
    ~ScopeVisualProcessor();
    void setup(int fftSize_in);
    void setScopeEnabled(bool scopeEnable);
    void setSpectrumEnabled(bool spectrumEnable);
protected:
    void process();
    ReBuffer<ScopeRenderData> outputBuffers;

    std::atomic_bool scopeEnabled;
    std::atomic_bool spectrumEnabled;
    
#if USE_FFTW3
    std::vector<float> fftInData;
    std::vector<fftwf_complex> fftwOutput;
    std::vector<fftwf_plan> fftw_plan;
#else
    std::vector<liquid_float_complex> fftInData;
    std::vector<liquid_float_complex> fftOutput;
    fftplan fftPlan;
#endif
    
    unsigned int fftSize;
    int desiredInputSize;
    unsigned int maxScopeSamples;
    
    double fft_ceil_ma, fft_ceil_maa;
    double fft_floor_ma, fft_floor_maa;
    double fft_average_rate;
    
    std::vector<double> fft_result;
    std::vector<double> fft_result_ma;
    std::vector<double> fft_result_maa;
};

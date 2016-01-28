#pragma once

#include "VisualProcessor.h"
#include "AudioThread.h"
#include "fftw3.h"
#include "ScopePanel.h"

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
    
    float *fftInData;
    fftwf_complex *fftwOutput;
    fftwf_plan fftw_plan;
    unsigned int fftSize;
    int desiredInputSize;
    unsigned int maxScopeSamples;
    
    double fft_ceil_ma, fft_ceil_maa;
    double fft_floor_ma, fft_floor_maa;
    float fft_average_rate;
    
    std::vector<double> fft_result;
    std::vector<double> fft_result_ma;
    std::vector<double> fft_result_maa;
};

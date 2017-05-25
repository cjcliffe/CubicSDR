// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "VisualProcessor.h"
#include "AudioThread.h"
#include "ScopePanel.h"
#include <memory>

class ScopeRenderData {
public:
	std::vector<float> waveform_points;
    ScopePanel::ScopeMode mode = ScopePanel::SCOPE_MODE_Y;
    int inputRate;
    int sampleRate;
	int channels;
    bool spectrum;
    int fft_size;
    double fft_floor, fft_ceil;

    virtual ~ScopeRenderData() {

    }
};

typedef std::shared_ptr<ScopeRenderData> ScopeRenderDataPtr;

typedef ThreadBlockingQueue<ScopeRenderDataPtr> ScopeRenderDataQueue;

class ScopeVisualProcessor : public VisualProcessor<AudioThreadInput, ScopeRenderData> {
public:
    ScopeVisualProcessor();
    ~ScopeVisualProcessor();
    void setup(int fftSize_in);
    void setScopeEnabled(bool scopeEnable);
    void setSpectrumEnabled(bool spectrumEnable);
protected:
    virtual void process();
    ReBuffer<ScopeRenderData> outputBuffers;

    std::atomic_bool scopeEnabled;
    std::atomic_bool spectrumEnabled;
    
    std::vector<liquid_float_complex> fftInData;
    std::vector<liquid_float_complex> fftOutput;
    fftplan fftPlan;
    
    unsigned int fftSize = 0;
    int desiredInputSize;
    unsigned int maxScopeSamples;
    
    double fft_ceil_ma, fft_ceil_maa;
    double fft_floor_ma, fft_floor_maa;
    double fft_average_rate;
    
    std::vector<double> fft_result;
    std::vector<double> fft_result_ma;
    std::vector<double> fft_result_maa;
};

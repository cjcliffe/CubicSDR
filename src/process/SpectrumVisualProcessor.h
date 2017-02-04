// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "VisualProcessor.h"
#include "DemodDefs.h"
#include <cmath>

#define SPECTRUM_VZM 2
#define PEAK_RESET_COUNT 30

class SpectrumVisualData : public ReferenceCounter {
public:
    std::vector<float> spectrum_points;
    std::vector<float> spectrum_hold_points;
    double fft_ceiling, fft_floor;
    long long centerFreq;
    int bandwidth;
};

typedef ThreadQueue<SpectrumVisualData *> SpectrumVisualDataQueue;

class SpectrumVisualProcessor : public VisualProcessor<DemodulatorThreadIQData, SpectrumVisualData> {
public:
    SpectrumVisualProcessor();
    ~SpectrumVisualProcessor();
    
    bool isView();
    void setView(bool bView);
    void setView(bool bView, long long centerFreq_in, long bandwidth_in);
    
    void setFFTAverageRate(float fftAverageRate);
    float getFFTAverageRate();
    
    void setCenterFrequency(long long centerFreq_in);
    long long getCenterFrequency();
    
    void setBandwidth(long bandwidth_in);
    long getBandwidth();
    
    void setPeakHold(bool peakHold_in);
    bool getPeakHold();
    
    int getDesiredInputSize();
    
    void setup(unsigned int fftSize);
    void setFFTSize(unsigned int fftSize);
    unsigned int getFFTSize();
    void setHideDC(bool hideDC);
    
    void setScaleFactor(float sf);
    float getScaleFactor();
    
protected:
    virtual void process();
    
    ReBuffer<SpectrumVisualData> outputBuffers;
    std::atomic_bool is_view;
    std::atomic_uint fftSize, newFFTSize;
    std::atomic_uint fftSizeInternal;
    std::atomic_llong centerFreq;
    std::atomic_long bandwidth;
    
private:
    long lastInputBandwidth;
    long lastBandwidth;
    bool lastView;

    liquid_float_complex *fftInput, *fftOutput, *fftInData, *fftLastData;
    fftplan fftPlan;

    unsigned int lastDataSize;
    
    double fft_ceil_ma, fft_ceil_maa;
    double fft_floor_ma, fft_floor_maa;
    double fft_ceil_peak, fft_floor_peak;
    std::atomic<float> fft_average_rate;
    
    std::vector<double> fft_result;
    std::vector<double> fft_result_ma;
    std::vector<double> fft_result_maa;
    std::vector<double> fft_result_peak;
    std::vector<double> fft_result_temp;
    
    msresamp_crcf resampler;
    double resamplerRatio;
    nco_crcf freqShifter;
    long shiftFrequency;
    
    std::vector<liquid_float_complex> shiftBuffer;
    std::vector<liquid_float_complex> resampleBuffer;
    std::atomic_int desiredInputSize;
   
    std::mutex busy_run;
    std::atomic_bool hideDC, peakHold;
    std::atomic_int peakReset;
    std::atomic<float> scaleFactor;
    std::atomic_bool fftSizeChanged;
};

// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "VisualProcessor.h"
#include "DemodDefs.h"
#include <cmath>
#include <memory>

#define SPECTRUM_VZM 2
#define PEAK_RESET_COUNT 30

class SpectrumVisualData {
public:
    std::vector<float> spectrum_points;
    std::vector<float> spectrum_hold_points;
    double fft_ceiling, fft_floor;
    long long centerFreq;
    int bandwidth;

    virtual ~SpectrumVisualData() = default;;
};

typedef std::shared_ptr<SpectrumVisualData> SpectrumVisualDataPtr;
typedef ThreadBlockingQueue<SpectrumVisualDataPtr> SpectrumVisualDataQueue;
typedef std::shared_ptr<SpectrumVisualDataQueue> SpectrumVisualDataQueuePtr;

class SpectrumVisualProcessor : public VisualProcessor<DemodulatorThreadIQData, SpectrumVisualData> {
public:
    SpectrumVisualProcessor();
    ~SpectrumVisualProcessor() override;
    
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
    void setHideDC(bool hideDC_in);
    
    void setScaleFactor(float sf);
    float getScaleFactor();
    
protected:
    void process() override;
    
    ReBuffer<SpectrumVisualData> outputBuffers;
  
    
private:
	//protects all access to fields below
	std::mutex busy_run;

	bool is_view;
	size_t fftSize, newFFTSize;
	size_t fftSizeInternal;
	long long centerFreq;
	size_t bandwidth;


    long lastInputBandwidth;
    long lastBandwidth;
    bool lastView;

    liquid_float_complex *fftInput, *fftOutput, *fftInData, *fftLastData;
    fftplan fftPlan;

    unsigned int lastDataSize;
    
    double fft_ceil_ma, fft_ceil_maa;
    double fft_floor_ma, fft_floor_maa;
    double fft_ceil_peak, fft_floor_peak;
    float fft_average_rate;
    
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
    size_t desiredInputSize;
   
    
    bool hideDC, peakHold;
    int peakReset;
    float scaleFactor;
    bool fftSizeChanged;
};

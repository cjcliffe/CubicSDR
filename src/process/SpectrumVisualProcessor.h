#pragma once

#include "VisualProcessor.h"
#include "DemodDefs.h"
#include "fftw3.h"
#include <cmath>

#define SPECTRUM_VZM 2

class SpectrumVisualData : public ReferenceCounter {
public:
    std::vector<float> spectrum_points;
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
    
    void setFFTAverageRate(float fftAverageRate);
    float getFFTAverageRate();
    
    void setCenterFrequency(long long centerFreq_in);
    long long getCenterFrequency();
    
    void setBandwidth(long bandwidth_in);
    long getBandwidth();
    
    int getDesiredInputSize();
    
    void setup(int fftSize);
    void setHideDC(bool hideDC);
    
    void setScaleFactor(float sf);
    float getScaleFactor();
    
protected:
    void process();
    
    ReBuffer<SpectrumVisualData> outputBuffers;
    std::atomic_bool is_view;
    std::atomic_int fftSize;
    std::atomic_int fftSizeInternal;
    std::atomic_llong centerFreq;
    std::atomic_long bandwidth;
    
private:
    long lastInputBandwidth;
    long lastBandwidth;
    
    fftwf_complex *fftwInput, *fftwOutput, *fftInData, *fftLastData;
    unsigned int lastDataSize;
    fftwf_plan fftw_plan;
    
    double fft_ceil_ma, fft_ceil_maa;
    double fft_floor_ma, fft_floor_maa;
    std::atomic<float> fft_average_rate;
    
    std::vector<double> fft_result;
    std::vector<double> fft_result_ma;
    std::vector<double> fft_result_maa;
    
    msresamp_crcf resampler;
    double resamplerRatio;
    nco_crcf freqShifter;
    long shiftFrequency;
    
    std::vector<liquid_float_complex> shiftBuffer;
    std::vector<liquid_float_complex> resampleBuffer;
    std::atomic_int desiredInputSize;
    std::mutex busy_run;
    std::atomic_bool hideDC;
    std::atomic<float> scaleFactor;
};

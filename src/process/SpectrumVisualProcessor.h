#pragma once

#include "VisualProcessor.h"
#include "DemodDefs.h"
#include "fftw3.h"

class SpectrumVisualData : public ReferenceCounter {
public:
    std::vector<float> spectrum_points;
    double fft_ceiling, fft_floor;
};

typedef ThreadQueue<SpectrumVisualData *> SpectrumVisualDataQueue;

class SpectrumVisualProcessor : public VisualProcessor<DemodulatorThreadIQData, SpectrumVisualData> {
public:
    SpectrumVisualProcessor();
    ~SpectrumVisualProcessor();
    
    bool isView();
    void setView(bool bView);
    
    void setCenterFrequency(long long centerFreq_in);
    long long getCenterFrequency();
    
    void setBandwidth(long bandwidth_in);
    long getBandwidth();
    
    int getDesiredInputSize();
    
    void setup(int fftSize);
    
protected:
    void process();
    
    ReBuffer<SpectrumVisualData> outputBuffers;
    std::atomic_bool is_view;
    std::atomic_int fftSize;
    std::atomic_llong centerFreq;
    std::atomic_long bandwidth;
    
private:
    long lastInputBandwidth;
    long lastBandwidth;
    
    fftwf_complex *fftwInput, *fftwOutput, *fftInData, *fftLastData;
    unsigned int lastDataSize;
    fftwf_plan fftw_plan;
    
    float fft_ceil_ma, fft_ceil_maa;
    float fft_floor_ma, fft_floor_maa;
    
    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;
    
    msresamp_crcf resampler;
    double resamplerRatio;
    nco_crcf freqShifter;
    long shiftFrequency;
    
    std::vector<liquid_float_complex> shiftBuffer;
    std::vector<liquid_float_complex> resampleBuffer;
    int desiredInputSize;
};


class FFTDataDistributor : public VisualProcessor<DemodulatorThreadIQData, DemodulatorThreadIQData> {
public:
    void setFFTSize(int fftSize) {
        this->fftSize = fftSize;
    }
    
protected:
    void process() {
        while (!input->empty()) {
            if (!isAnyOutputEmpty()) {
                return;
            }
            DemodulatorThreadIQData *inp;
            input->pop(inp);
            if (inp) {
                if (inp->data.size() >= fftSize) {
                    for (int i = 0, iMax = inp->data.size()-fftSize; i < iMax; i += fftSize) {
                        DemodulatorThreadIQData *outp = outputBuffers.getBuffer();
                        outp->frequency = inp->frequency;
                        outp->sampleRate = inp->sampleRate;
                        outp->data.assign(inp->data.begin()+i,inp->data.begin()+i+fftSize);
                        distribute(outp);
                    }
                }
                inp->decRefCount();
            }
        }
    }
    
    ReBuffer<DemodulatorThreadIQData> outputBuffers;
    int fftSize;
};

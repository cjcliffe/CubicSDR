#include "SpectrumVisualProcessor.h"
#include "CubicSDR.h"


SpectrumVisualProcessor::SpectrumVisualProcessor() : lastInputBandwidth(0), lastBandwidth(0), fftwInput(NULL), fftwOutput(NULL), fftInData(NULL), fftLastData(NULL), lastDataSize(0), fftw_plan(NULL), resampler(NULL), resamplerRatio(0) {
    
    is_view.store(false);
    fftSize.store(0);
    centerFreq.store(0);
    bandwidth.store(0);
    
    freqShifter = nco_crcf_create(LIQUID_NCO);
    shiftFrequency = 0;
    
    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;
    desiredInputSize = 0;
    fft_average_rate = 0.65;
}

SpectrumVisualProcessor::~SpectrumVisualProcessor() {
    nco_crcf_destroy(freqShifter);
}

bool SpectrumVisualProcessor::isView() {
    return is_view.load();
}

void SpectrumVisualProcessor::setView(bool bView) {
    is_view.store(bView);
}

void SpectrumVisualProcessor::setFFTAverageRate(float fftAverageRate) {
    this->fft_average_rate = fftAverageRate;
}

float SpectrumVisualProcessor::getFFTAverageRate() {
    return this->fft_average_rate;
}

void SpectrumVisualProcessor::setCenterFrequency(long long centerFreq_in) {
    centerFreq.store(centerFreq_in);
}

long long SpectrumVisualProcessor::getCenterFrequency() {
    return centerFreq.load();
}

void SpectrumVisualProcessor::setBandwidth(long bandwidth_in) {
    bandwidth.store(bandwidth_in);
}

long SpectrumVisualProcessor::getBandwidth() {
    return bandwidth.load();
}

int SpectrumVisualProcessor::getDesiredInputSize() {
    return desiredInputSize;
}

void SpectrumVisualProcessor::setup(int fftSize_in) {
    fftSize = fftSize_in;
    desiredInputSize = fftSize;
    
    if (fftwInput) {
        free(fftwInput);
    }
    fftwInput = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);
    if (fftInData) {
        free(fftInData);
    }
    fftInData = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);
    if (fftLastData) {
        free(fftLastData);
    }
    fftLastData = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);
    if (fftwOutput) {
        free(fftwOutput);
    }
    fftwOutput = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);
    if (fftw_plan) {
        fftwf_destroy_plan(fftw_plan);
    }
    fftw_plan = fftwf_plan_dft_1d(fftSize, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_ESTIMATE);
    
}

void SpectrumVisualProcessor::process() {
    if (!isOutputEmpty()) {
        return;
    }
    if (!input || input->empty()) {
        return;
    }
    
    DemodulatorThreadIQData *iqData;
    
    input->pop(iqData);
    
    if (!iqData) {
        return;
    }
    
    iqData->busy_rw.lock();
    
    std::vector<liquid_float_complex> *data = &iqData->data;
    
    if (data && data->size()) {
        SpectrumVisualData *output = outputBuffers.getBuffer();
        
        if (output->spectrum_points.size() < fftSize * 2) {
            output->spectrum_points.resize(fftSize * 2);
        }
        
        unsigned int num_written;
        
        if (is_view.load()) {
            if (!iqData->frequency || !iqData->sampleRate) {
                iqData->decRefCount();
                iqData->busy_rw.unlock();
                return;
            }
            
            resamplerRatio = (double) (bandwidth) / (double) iqData->sampleRate;
            
            int desired_input_size = fftSize / resamplerRatio;
            
            this->desiredInputSize = desired_input_size;
            
            if (iqData->data.size() < desired_input_size) {
                //                std::cout << "fft underflow, desired: " << desired_input_size << " actual:" << input->data.size() << std::endl;
                desired_input_size = iqData->data.size();
            }
            
            if (centerFreq != iqData->frequency) {
                if ((centerFreq - iqData->frequency) != shiftFrequency || lastInputBandwidth != iqData->sampleRate) {
                    if (abs(iqData->frequency - centerFreq) < (wxGetApp().getSampleRate() / 2)) {
                        shiftFrequency = centerFreq - iqData->frequency;
                        nco_crcf_reset(freqShifter);
                        nco_crcf_set_frequency(freqShifter, (2.0 * M_PI) * (((double) abs(shiftFrequency)) / ((double) iqData->sampleRate)));
                    }
                }
                
                if (shiftBuffer.size() != desired_input_size) {
                    if (shiftBuffer.capacity() < desired_input_size) {
                        shiftBuffer.reserve(desired_input_size);
                    }
                    shiftBuffer.resize(desired_input_size);
                }
                
                if (shiftFrequency < 0) {
                    nco_crcf_mix_block_up(freqShifter, &iqData->data[0], &shiftBuffer[0], desired_input_size);
                } else {
                    nco_crcf_mix_block_down(freqShifter, &iqData->data[0], &shiftBuffer[0], desired_input_size);
                }
            } else {
                shiftBuffer.assign(iqData->data.begin(), iqData->data.end());
            }
            
            if (!resampler || bandwidth != lastBandwidth || lastInputBandwidth != iqData->sampleRate) {
                float As = 60.0f;
                
                if (resampler) {
                    msresamp_crcf_destroy(resampler);
                }
                resampler = msresamp_crcf_create(resamplerRatio, As);
                
                lastBandwidth = bandwidth;
                lastInputBandwidth = iqData->sampleRate;
            }
            
            
            int out_size = ceil((double) (desired_input_size) * resamplerRatio) + 512;
            
            if (resampleBuffer.size() != out_size) {
                if (resampleBuffer.capacity() < out_size) {
                    resampleBuffer.reserve(out_size);
                }
                resampleBuffer.resize(out_size);
            }
            
            
            msresamp_crcf_execute(resampler, &shiftBuffer[0], desired_input_size, &resampleBuffer[0], &num_written);
            
            resampleBuffer.resize(fftSize);
            
            if (num_written < fftSize) {
                for (int i = 0; i < num_written; i++) {
                    fftInData[i][0] = resampleBuffer[i].real;
                    fftInData[i][1] = resampleBuffer[i].imag;
                }
                for (int i = num_written; i < fftSize; i++) {
                    fftInData[i][0] = 0;
                    fftInData[i][1] = 0;
                }
            } else {
                for (int i = 0; i < fftSize; i++) {
                    fftInData[i][0] = resampleBuffer[i].real;
                    fftInData[i][1] = resampleBuffer[i].imag;
                }
            }
        } else {
            num_written = data->size();
            if (data->size() < fftSize) {
                for (int i = 0, iMax = data->size(); i < iMax; i++) {
                    fftInData[i][0] = (*data)[i].real;
                    fftInData[i][1] = (*data)[i].imag;
                }
                for (int i = data->size(); i < fftSize; i++) {
                    fftInData[i][0] = 0;
                    fftInData[i][1] = 0;
                }
            } else {
                for (int i = 0; i < fftSize; i++) {
                    fftInData[i][0] = (*data)[i].real;
                    fftInData[i][1] = (*data)[i].imag;
                }
            }
        }
        
        bool execute = false;
        
        if (num_written >= fftSize) {
            execute = true;
            memcpy(fftwInput, fftInData, fftSize * sizeof(fftwf_complex));
            memcpy(fftLastData, fftwInput, fftSize * sizeof(fftwf_complex));
            
        } else {
            if (lastDataSize + num_written < fftSize) { // priming
                unsigned int num_copy = fftSize - lastDataSize;
                if (num_written > num_copy) {
                    num_copy = num_written;
                }
                memcpy(fftLastData, fftInData, num_copy * sizeof(fftwf_complex));
                lastDataSize += num_copy;
            } else {
                unsigned int num_last = (fftSize - num_written);
                memcpy(fftwInput, fftLastData + (lastDataSize - num_last), num_last * sizeof(fftwf_complex));
                memcpy(fftwInput + num_last, fftInData, num_written * sizeof(fftwf_complex));
                memcpy(fftLastData, fftwInput, fftSize * sizeof(fftwf_complex));
                execute = true;
            }
        }
        
        if (execute) {
            fftwf_execute(fftw_plan);
            
            float fft_ceil = 0, fft_floor = 1;
            
            if (fft_result.size() < fftSize) {
                fft_result.resize(fftSize);
                fft_result_ma.resize(fftSize);
                fft_result_maa.resize(fftSize);
            }
            
            for (int i = 0, iMax = fftSize / 2; i < iMax; i++) {
                float a = fftwOutput[i][0];
                float b = fftwOutput[i][1];
                float c = sqrt(a * a + b * b);
                
                float x = fftwOutput[fftSize / 2 + i][0];
                float y = fftwOutput[fftSize / 2 + i][1];
                float z = sqrt(x * x + y * y);
                
                fft_result[i] = (z);
                fft_result[fftSize / 2 + i] = (c);
            }
            
            for (int i = 0, iMax = fftSize; i < iMax; i++) {
                if (is_view.load()) {
                    fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * fft_average_rate;
                    fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * fft_average_rate;
                } else {
                    fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * fft_average_rate;
                    fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * fft_average_rate;
                }
                
                if (fft_result_maa[i] > fft_ceil) {
                    fft_ceil = fft_result_maa[i];
                }
                if (fft_result_maa[i] < fft_floor) {
                    fft_floor = fft_result_maa[i];
                }
            }
            
            fft_ceil_ma = fft_ceil_ma + (fft_ceil - fft_ceil_ma) * 0.05;
            fft_ceil_maa = fft_ceil_maa + (fft_ceil_ma - fft_ceil_maa) * 0.05;
            
            fft_floor_ma = fft_floor_ma + (fft_floor - fft_floor_ma) * 0.05;
            fft_floor_maa = fft_floor_maa + (fft_floor_ma - fft_floor_maa) * 0.05;
            
            for (int i = 0, iMax = fftSize; i < iMax; i++) {
                float v = (log10(fft_result_maa[i]+0.25 - (fft_floor_maa-1.0)) / log10((fft_ceil_maa+0.25) - (fft_floor_maa-1.0)));
                output->spectrum_points[i * 2] = ((float) i / (float) iMax);
                output->spectrum_points[i * 2 + 1] = v;
            }
            
            output->fft_ceiling = fft_ceil_maa;
            output->fft_floor = fft_floor_maa;
        }
        
        distribute(output);
    }
 
    iqData->decRefCount();
    iqData->busy_rw.unlock();
}


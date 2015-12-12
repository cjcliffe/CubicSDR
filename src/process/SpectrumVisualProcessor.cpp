#include "SpectrumVisualProcessor.h"
#include "CubicSDR.h"


SpectrumVisualProcessor::SpectrumVisualProcessor() : lastInputBandwidth(0), lastBandwidth(0), fftwInput(NULL), fftwOutput(NULL), fftInData(NULL), fftLastData(NULL), lastDataSize(0), fftw_plan(NULL), resampler(NULL), resamplerRatio(0), outputBuffers("SpectrumVisualProcessorBuffers") {
    
    is_view.store(false);
    fftSize.store(0);
    centerFreq.store(0);
    bandwidth.store(0);
    hideDC.store(false);
    
    freqShifter = nco_crcf_create(LIQUID_NCO);
    shiftFrequency = 0;
    
    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;
    desiredInputSize.store(0);
    fft_average_rate = 0.65;
    scaleFactor.store(1.0);
}

SpectrumVisualProcessor::~SpectrumVisualProcessor() {
    nco_crcf_destroy(freqShifter);
}

bool SpectrumVisualProcessor::isView() {
    return is_view.load();
}

void SpectrumVisualProcessor::setView(bool bView) {
    busy_run.lock();
    is_view.store(bView);
    busy_run.unlock();
}

void SpectrumVisualProcessor::setView(bool bView, long long centerFreq_in, long bandwidth_in) {
    busy_run.lock();
    is_view.store(bView);
    bandwidth.store(bandwidth_in);
    centerFreq.store(centerFreq_in);
    busy_run.unlock();
}


void SpectrumVisualProcessor::setFFTAverageRate(float fftAverageRate) {
    busy_run.lock();
    this->fft_average_rate.store(fftAverageRate);
    busy_run.unlock();
}

float SpectrumVisualProcessor::getFFTAverageRate() {
    return this->fft_average_rate.load();
}

void SpectrumVisualProcessor::setCenterFrequency(long long centerFreq_in) {
    busy_run.lock();
    centerFreq.store(centerFreq_in);
    busy_run.unlock();
}

long long SpectrumVisualProcessor::getCenterFrequency() {
    return centerFreq.load();
}

void SpectrumVisualProcessor::setBandwidth(long bandwidth_in) {
    busy_run.lock();
    bandwidth.store(bandwidth_in);
    busy_run.unlock();
}

long SpectrumVisualProcessor::getBandwidth() {
    return bandwidth.load();
}

int SpectrumVisualProcessor::getDesiredInputSize() {
    return desiredInputSize.load();
}

void SpectrumVisualProcessor::setup(int fftSize_in) {
    busy_run.lock();

    fftSize = fftSize_in;
    fftSizeInternal = fftSize_in * SPECTRUM_VZM;
    desiredInputSize.store(fftSizeInternal);
    
    if (fftwInput) {
        free(fftwInput);
    }
    fftwInput = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSizeInternal);
    if (fftInData) {
        free(fftInData);
    }
    fftInData = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSizeInternal);
    if (fftLastData) {
        free(fftLastData);
    }
    fftLastData = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSizeInternal);
    if (fftwOutput) {
        free(fftwOutput);
    }
    fftwOutput = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSizeInternal);
    if (fftw_plan) {
        fftwf_destroy_plan(fftw_plan);
    }
    fftw_plan = fftwf_plan_dft_1d(fftSizeInternal, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_ESTIMATE);
    busy_run.unlock();
}

void SpectrumVisualProcessor::setHideDC(bool hideDC) {
    this->hideDC.store(hideDC);
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
    busy_run.lock();
    
    std::vector<liquid_float_complex> *data = &iqData->data;
    
    if (data && data->size()) {
        SpectrumVisualData *output = outputBuffers.getBuffer();
        
        if (output->spectrum_points.size() < fftSize * 2) {
            output->spectrum_points.resize(fftSize * 2);
        }
        
        unsigned int num_written;
        long resampleBw = iqData->sampleRate;
        bool newResampler = false;
        int bwDiff;
        
//        if (bandwidth > resampleBw) {
//            iqData->decRefCount();
//            iqData->busy_rw.unlock();
//            busy_run.unlock();
//            return;
//        }
        
        if (is_view.load()) {
            if (!iqData->frequency || !iqData->sampleRate) {
                iqData->decRefCount();
                iqData->busy_rw.unlock();
                busy_run.unlock();
                return;
            }
            
//            resamplerRatio = (double) (bandwidth) / (double) iqData->sampleRate;
            while (resampleBw / SPECTRUM_VZM >= bandwidth) {
                resampleBw /= SPECTRUM_VZM;
            }
            
            resamplerRatio = (double) (resampleBw) / (double) iqData->sampleRate;
            
            int desired_input_size = fftSizeInternal / resamplerRatio;
            
            this->desiredInputSize.store(desired_input_size);
            
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
            
            if (!resampler || resampleBw != lastBandwidth || lastInputBandwidth != iqData->sampleRate) {
                float As = 60.0f;
                
                if (resampler) {
                    msresamp_crcf_destroy(resampler);
                }
                
                resampler = msresamp_crcf_create(resamplerRatio, As);
                
                bwDiff = resampleBw-lastBandwidth;
                lastBandwidth = resampleBw;
                lastInputBandwidth = iqData->sampleRate;
                newResampler = true;
            }
            
            
            int out_size = ceil((double) (desired_input_size) * resamplerRatio) + 512;
            
            if (resampleBuffer.size() != out_size) {
                if (resampleBuffer.capacity() < out_size) {
                    resampleBuffer.reserve(out_size);
                }
                resampleBuffer.resize(out_size);
            }
            
            
            msresamp_crcf_execute(resampler, &shiftBuffer[0], desired_input_size, &resampleBuffer[0], &num_written);
            
            resampleBuffer.resize(fftSizeInternal);
            
            if (num_written < fftSizeInternal) {
                for (int i = 0; i < num_written; i++) {
                    fftInData[i][0] = resampleBuffer[i].real;
                    fftInData[i][1] = resampleBuffer[i].imag;
                }
                for (int i = num_written; i < fftSizeInternal; i++) {
                    fftInData[i][0] = 0;
                    fftInData[i][1] = 0;
                }
            } else {
                for (int i = 0; i < fftSizeInternal; i++) {
                    fftInData[i][0] = resampleBuffer[i].real;
                    fftInData[i][1] = resampleBuffer[i].imag;
                }
            }
        } else {
            num_written = data->size();
            if (data->size() < fftSizeInternal) {
                for (int i = 0, iMax = data->size(); i < iMax; i++) {
                    fftInData[i][0] = (*data)[i].real;
                    fftInData[i][1] = (*data)[i].imag;
                }
                for (int i = data->size(); i < fftSizeInternal; i++) {
                    fftInData[i][0] = 0;
                    fftInData[i][1] = 0;
                }
            } else {
                for (int i = 0; i < fftSizeInternal; i++) {
                    fftInData[i][0] = (*data)[i].real;
                    fftInData[i][1] = (*data)[i].imag;
                }
            }
        }
        
        bool execute = false;
        
        if (num_written >= fftSizeInternal) {
            execute = true;
            memcpy(fftwInput, fftInData, fftSizeInternal * sizeof(fftwf_complex));
            memcpy(fftLastData, fftwInput, fftSizeInternal * sizeof(fftwf_complex));
            
        } else {
            if (lastDataSize + num_written < fftSizeInternal) { // priming
                unsigned int num_copy = fftSizeInternal - lastDataSize;
                if (num_written > num_copy) {
                    num_copy = num_written;
                }
                memcpy(fftLastData, fftInData, num_copy * sizeof(fftwf_complex));
                lastDataSize += num_copy;
            } else {
                unsigned int num_last = (fftSizeInternal - num_written);
                memcpy(fftwInput, fftLastData + (lastDataSize - num_last), num_last * sizeof(fftwf_complex));
                memcpy(fftwInput + num_last, fftInData, num_written * sizeof(fftwf_complex));
                memcpy(fftLastData, fftwInput, fftSizeInternal * sizeof(fftwf_complex));
                execute = true;
            }
        }
        
        if (execute) {
            fftwf_execute(fftw_plan);
            
            float fft_ceil = 0, fft_floor = 1;
            
            if (fft_result.size() < fftSizeInternal) {
                fft_result.resize(fftSizeInternal);
                fft_result_ma.resize(fftSizeInternal);
                fft_result_maa.resize(fftSizeInternal);
                fft_result_temp.resize(fftSizeInternal);
            }
            
            for (int i = 0, iMax = fftSizeInternal / 2; i < iMax; i++) {
                float a = fftwOutput[i][0];
                float b = fftwOutput[i][1];
                float c = sqrt(a * a + b * b);
                
                float x = fftwOutput[fftSizeInternal / 2 + i][0];
                float y = fftwOutput[fftSizeInternal / 2 + i][1];
                float z = sqrt(x * x + y * y);
                
                fft_result[i] = (z);
                fft_result[fftSizeInternal / 2 + i] = (c);
            }
            
            if (newResampler) {
                if (bwDiff < 0) {
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_temp[i] = fft_result_ma[(fftSizeInternal/4) + (i/2)];
                    }
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_ma[i] = fft_result_temp[i];
                        
                        fft_result_temp[i] = fft_result_maa[(fftSizeInternal/4) + (i/2)];
                    }
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_maa[i] = fft_result_temp[i];
                    }
                } else {
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        if (i < fftSizeInternal/4) {
                            fft_result_temp[i] = 0;
                        } else if (i >= fftSizeInternal - fftSizeInternal/4) {
                            fft_result_temp[i] = 0;
                        } else {
                            fft_result_temp[i] = fft_result_ma[(i-fftSizeInternal/4)*2];
                        }
                    }
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_ma[i] = fft_result_temp[i];
                        
                        if (i < fftSizeInternal/4) {
                            fft_result_temp[i] = 0;
                        } else if (i >= fftSizeInternal - fftSizeInternal/4) {
                            fft_result_temp[i] = 0;
                        } else {
                            fft_result_temp[i] = fft_result_maa[(i-fftSizeInternal/4)*2];
                        }
                    }
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_maa[i] = fft_result_temp[i];
                    }
                }
            }
            
            for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * fft_average_rate;
                fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * fft_average_rate;
                
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
            
            float sf = scaleFactor.load();
            
//            for (int i = 0, iMax = fftSize; i < iMax; i++) {
//                float v = (log10(fft_result_maa[i*SPECTRUM_VZM]+0.25 - (fft_floor_maa-0.75)) / log10((fft_ceil_maa+0.25) - (fft_floor_maa-0.75)));
//                output->spectrum_points[i * 2] = ((float) i / (float) iMax);
//                output->spectrum_points[i * 2 + 1] = v*sf;
//            }
            double visualRatio = (double(bandwidth) / double(resampleBw));
            double visualStart = (double(fftSizeInternal) / 2.0) - (double(fftSizeInternal) * (visualRatio / 2.0));
            double visualAccum = 0;
            double acc = 0, accCount = 0, i = 0;
            
            for (int x = 0, xMax = output->spectrum_points.size() / 2; x < xMax; x++) {
                visualAccum += visualRatio * double(SPECTRUM_VZM);
//                while (visualAccum >= 1.0) {
//                    visualAccum -= 1.0;
//                    i++;
//                }
//                acc = (log10(fft_result_maa[visualStart+i]+0.25 - (fft_floor_maa-0.75)) / log10((fft_ceil_maa+0.25) - (fft_floor_maa-0.75)));
//                output->spectrum_points[x * 2] = (float(x) / float(xMax));
//                output->spectrum_points[x * 2 + 1] = acc*sf;

                while (visualAccum >= 1.0) {
                    int idx = round(visualStart+i);
                    if (idx < 0) {
                        idx = 0;
                    }
                    if (idx > fftSizeInternal) {
                        idx = fftSizeInternal;
                    }
                    acc += fft_result_maa[idx];
                    accCount += 1.0;
                    visualAccum -= 1.0;
                    i++;
                }
                if (accCount) {
                    output->spectrum_points[x * 2] = ((float) x / (float) xMax);
                    output->spectrum_points[x * 2 + 1] = ((log10((acc/accCount)+0.25 - (fft_floor_maa-0.75)) / log10((fft_ceil_maa+0.25) - (fft_floor_maa-0.75))))*sf;
                    acc = 0.0;
                    accCount = 0.0;
                }
            }
            
            if (hideDC.load()) { // DC-spike removal
                long long freqMin = centerFreq-(bandwidth/2);
                long long freqMax = centerFreq+(bandwidth/2);
                long long zeroPt = (iqData->frequency-freqMin);
                
                if (freqMin < iqData->frequency && freqMax > iqData->frequency) {
                    int freqRange = int(freqMax-freqMin);
                    int freqStep = freqRange/fftSize;
                    int fftStart = (zeroPt/freqStep)-(2000/freqStep);
                    int fftEnd = (zeroPt/freqStep)+(2000/freqStep);
                    
//                    std::cout << "range:" << freqRange << ", step: " << freqStep << ", start: " << fftStart << ", end: " << fftEnd << std::endl;
                    
                    if (fftEnd-fftStart < 2) {
                        fftEnd++;
                        fftStart--;
                    }
                    
                    int numSteps = (fftEnd-fftStart);
                    int halfWay = fftStart+(numSteps/2);

                    if ((fftEnd+numSteps/2+1 < fftSize) && (fftStart-numSteps/2-1 >= 0) && (fftEnd > fftStart)) {
                        int n = 1;
                        for (int i = fftStart; i < halfWay; i++) {
                            output->spectrum_points[i * 2 + 1] = output->spectrum_points[(fftStart - n) * 2 + 1];
                            n++;
                        }
                        n = 1;
                        for (int i = halfWay; i < fftEnd; i++) {
                            output->spectrum_points[i * 2 + 1] = output->spectrum_points[(fftEnd + n) * 2 + 1];
                            n++;
                        }
                    }
                }
            }
            
            output->fft_ceiling = fft_ceil_maa/sf;
            output->fft_floor = fft_floor_maa;
        }
        
        output->centerFreq = centerFreq;
        output->bandwidth = bandwidth;
        distribute(output);
    }
 
    iqData->decRefCount();
    iqData->busy_rw.unlock();
    busy_run.unlock();
}


void SpectrumVisualProcessor::setScaleFactor(float sf) {
    scaleFactor.store(sf);
}


float SpectrumVisualProcessor::getScaleFactor() {
    return scaleFactor.load();
}


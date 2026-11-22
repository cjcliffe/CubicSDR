// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SpectrumVisualProcessor.h"
#include "SpectrumDsp.h"
#include "AppServices.h"

#include <chrono>
#include <cstdint>

//50 ms
#define HEARTBEAT_CHECK_PERIOD_MICROS (50 * 1000) 

#define SPECTRUM_RESAMPLE_MIN_INTERVAL_MS 100

static uint64_t steadyClockMs() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

SpectrumVisualProcessor::SpectrumVisualProcessor() : outputBuffers("SpectrumVisualProcessorBuffers") {
    lastInputBandwidth = 0;
    lastBandwidth = 0;
    lastDataSize = 0;
    resampler = nullptr;
    resamplerRatio = 0;

    fftInput = nullptr;
    fftOutput = nullptr;
    fftInData = nullptr;
    fftLastData = nullptr;
    fftPlan = nullptr;
    
    is_view = false;
    fftSize = 0;
    centerFreq = 0;
    bandwidth = 0;
    hideDC = false;
    
    freqShifter = nco_crcf_create(LIQUID_NCO);
    shiftFrequency = 0;
    
    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;
    fft_floor_peak = 0.0;
    desiredInputSize = 0;
    fft_average_rate = 0.65f;
    scaleFactor = 1.0;
    fftSizeChanged = false;
    newFFTSize = 0;
    lastView = false;
    peakHold = false;
    peakReset = false;
    
}

SpectrumVisualProcessor::~SpectrumVisualProcessor() {
    nco_crcf_destroy(freqShifter);
}

bool SpectrumVisualProcessor::isView() {
	
	std::lock_guard < std::mutex > busy_lock(busy_run);
    
	return is_view;
}

void SpectrumVisualProcessor::setView(bool bView) {
	
	std::lock_guard < std::mutex > busy_lock(busy_run);

    is_view = bView;  
}

void SpectrumVisualProcessor::setView(bool bView, long long centerFreq_in, long bandwidth_in) {
    
    std::lock_guard < std::mutex > busy_lock(busy_run);    
    is_view = bView;
    bandwidth = bandwidth_in;
    centerFreq = centerFreq_in; 
}


void SpectrumVisualProcessor::setFFTAverageRate(float fftAverageRate) {
   
    std::lock_guard < std::mutex > busy_lock(busy_run);    

    this->fft_average_rate = fftAverageRate;    
}

float SpectrumVisualProcessor::getFFTAverageRate() {

	std::lock_guard < std::mutex > busy_lock(busy_run);

    return this->fft_average_rate;
}

void SpectrumVisualProcessor::setCenterFrequency(long long centerFreq_in) {
   
    std::lock_guard < std::mutex > busy_lock(busy_run);  

    centerFreq = centerFreq_in;  
}

long long SpectrumVisualProcessor::getCenterFrequency() {
	
	std::lock_guard < std::mutex > busy_lock(busy_run);

    return centerFreq;
}

void SpectrumVisualProcessor::setBandwidth(long bandwidth_in) {
   
    std::lock_guard < std::mutex > busy_lock(busy_run);    

	bandwidth = bandwidth_in;
}

long SpectrumVisualProcessor::getBandwidth() {

	std::lock_guard < std::mutex > busy_lock(busy_run);

    return bandwidth;
}

void SpectrumVisualProcessor::setPeakHold(bool peakHold_in) {
	
	std::lock_guard < std::mutex > busy_lock(busy_run);

    if (peakHold && peakHold_in) {
        peakReset = PEAK_RESET_COUNT;
    } else {
        peakHold = peakHold_in;
        peakReset = 1;
    }
}

bool SpectrumVisualProcessor::getPeakHold() {

	std::lock_guard < std::mutex > busy_lock(busy_run);

    return peakHold;
}

int SpectrumVisualProcessor::getDesiredInputSize() {
	std::lock_guard < std::mutex > busy_lock(busy_run);

    return desiredInputSize;
}

bool SpectrumVisualProcessor::setup(unsigned int fftSize_in) {

    std::lock_guard < std::mutex > busy_lock(busy_run);    

    int fftSizeInternal_in = fftSize_in * SPECTRUM_VZM;
    int memSize = sizeof(liquid_float_complex) * fftSizeInternal_in;

    liquid_float_complex *fftInput_in = (liquid_float_complex*)malloc(memSize);
    liquid_float_complex *fftInData_in = (liquid_float_complex*)malloc(memSize);
    liquid_float_complex *fftLastData_in = (liquid_float_complex*)malloc(memSize);
    liquid_float_complex *fftOutput_in = (liquid_float_complex*)malloc(memSize);

    if (!fftInput_in || !fftInData_in || !fftLastData_in || !fftOutput_in) {
        free(fftInput_in);
        free(fftInData_in);
        free(fftLastData_in);
        free(fftOutput_in);
        return false;
    }

    memset(fftInput_in,0,memSize);
    memset(fftInData_in,0,memSize);
    memset(fftLastData_in,0,memSize);
    memset(fftOutput_in,0,memSize);
    
    if (fftInput) {
        free(fftInput);
    }
    fftInput = fftInput_in;
    
    if (fftInData) {
        free(fftInData);
    }
    fftInData = fftInData_in;
    
    if (fftLastData) {
        free(fftLastData);
    }
    fftLastData = fftLastData_in;
    
    if (fftOutput) {
        free(fftOutput);
    }
    fftOutput = fftOutput_in;
    
    if (fftPlan) {
        fft_destroy_plan(fftPlan);
    }

    fftSize = fftSize_in;
    fftSizeInternal = fftSizeInternal_in;
    lastDataSize = 0;

    fftPlan = fft_create_plan(fftSizeInternal, fftInput, fftOutput, LIQUID_FFT_FORWARD, 0);

    generateHannWindow(fftWindow, fftSizeInternal);

    return true;
}

void SpectrumVisualProcessor::setFFTSize(unsigned int fftSize_in) {

	//then get the busy_lock
	std::lock_guard < std::mutex > busy_lock(busy_run);

    if (fftSize_in == fftSize) {
        return;
    }
    newFFTSize = fftSize_in;
    fftSizeChanged = true;
}

unsigned int SpectrumVisualProcessor::getFFTSize() {

	//then get the busy_lock
	std::lock_guard < std::mutex > busy_lock(busy_run);

    if (fftSizeChanged) {
        return newFFTSize;
    }
    return fftSize;
}


void SpectrumVisualProcessor::setHideDC(bool hideDC_in) {

	std::lock_guard < std::mutex > busy_lock(busy_run);

    hideDC = hideDC_in;
}


void SpectrumVisualProcessor::process() {
    if (!isOutputEmpty()) {
        return;
    }
    if (!input || input->empty()) {
        return;
    }
    
	bool executeSetup = false;

	{ // scoped lock here
		std::lock_guard < std::mutex > busy_lock(busy_run);
		if (fftSizeChanged) {
			executeSetup = true;
			fftSizeChanged = false;
		}
	}

	if (executeSetup) {
		if (!setup(newFFTSize)) {
			std::lock_guard < std::mutex > busy_lock(busy_run);
			fftSizeChanged = true;
		}
	}
   
    DemodulatorThreadIQDataPtr iqData;
    
    if (!input->pop(iqData, HEARTBEAT_CHECK_PERIOD_MICROS)) {
        return;
    }
    
    if (!iqData) {
        return;
    }

    //then get the busy_lock for the rest of the processing.
    std::lock_guard < std::mutex > busy_lock(busy_run);    
   
    bool doPeak = peakHold && (peakReset == 0);
    
    if (fft_result.size() != fftSizeInternal) {

        if (fft_result.capacity() < fftSizeInternal) {
            fft_result.reserve(fftSizeInternal);
            fft_result_ma.reserve(fftSizeInternal);
            fft_result_maa.reserve(fftSizeInternal);
            fft_result_avg.reserve(fftSizeInternal);
            fft_result_peak.reserve(fftSizeInternal);
        }
        fft_result.resize(fftSizeInternal);
        fft_result_ma.resize(fftSizeInternal);
        fft_result_maa.resize(fftSizeInternal);
        fft_result_avg.resize(fftSizeInternal);
        fft_result_temp.resize(fftSizeInternal);
        fft_result_peak.resize(fftSizeInternal);
    }
    
    if (peakReset != 0) {
        peakReset--;
        if (peakReset == 0) {
            for (unsigned int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                fft_result_peak[i] = fft_floor_maa;
            }
            fft_ceil_peak = fft_floor_maa;
            fft_floor_peak = fft_ceil_maa;
        }
    }
    
    std::vector<liquid_float_complex> *data = &iqData->data;
    
    if (data && !data->empty()) {
        unsigned int num_written;
        long resampleBw = iqData->sampleRate;
        bool newResampler = false;
        int bwDiff = 0;
        
        if (is_view) {
            if (!iqData->sampleRate) {
               
                return;
            }
            
            while (resampleBw / SPECTRUM_VZM >= (long) bandwidth) {
                resampleBw /= SPECTRUM_VZM;
            }
            
            resamplerRatio = (double) (resampleBw) / (double) iqData->sampleRate;
            
            size_t desired_input_size = fftSizeInternal / resamplerRatio;
            
            this->desiredInputSize = desired_input_size;
            
            if (iqData->data.size() < desired_input_size) {
                //                std::cout << "fft underflow, desired: " << desired_input_size << " actual:" << input->data.size() << std::endl;
                desired_input_size = iqData->data.size();
            }
            
            if (centerFreq != iqData->frequency) {
                if ((centerFreq - iqData->frequency) != shiftFrequency || lastInputBandwidth != iqData->sampleRate) {
                    if (abs(iqData->frequency - centerFreq) < (GetAppServices().getSampleRate() / 2)) {
                        long lastShiftFrequency = shiftFrequency;
                        shiftFrequency = centerFreq - iqData->frequency;
                        nco_crcf_set_frequency(freqShifter, (2.0 * M_PI) * (((double) abs(shiftFrequency)) / ((double) iqData->sampleRate)));
                        
                        if (is_view) {
                            long freqDiff = shiftFrequency - lastShiftFrequency;
                            
                            if (lastBandwidth!=0) {
                                double binPerHz = double(lastBandwidth) / double(fftSizeInternal);
                                
                                unsigned int numShift = floor(double(abs(freqDiff)) / binPerHz);
                                
                                if (numShift < fftSizeInternal/2 && numShift) {
                                    if (freqDiff > 0) {
                                        memmove(&fft_result_ma[0], &fft_result_ma[numShift], (fftSizeInternal-numShift) * sizeof(double));
                                        memmove(&fft_result_maa[0], &fft_result_maa[numShift], (fftSizeInternal-numShift) * sizeof(double));
//                                        memmove(&fft_result_peak[0], &fft_result_peak[numShift], (fftSizeInternal-numShift) * sizeof(double));
//                                        memset(&fft_result_peak[fftSizeInternal-numShift], 0, numShift * sizeof(double));
                                    } else {
                                        memmove(&fft_result_ma[numShift], &fft_result_ma[0], (fftSizeInternal-numShift) * sizeof(double));
                                        memmove(&fft_result_maa[numShift], &fft_result_maa[0], (fftSizeInternal-numShift) * sizeof(double));
//                                        memmove(&fft_result_peak[numShift], &fft_result_peak[0], (fftSizeInternal-numShift) * sizeof(double));
//                                        memset(&fft_result_peak[0], 0, numShift * sizeof(double));
                                    }
                                }
                            }
                        }
                    }
                    peakReset = PEAK_RESET_COUNT;
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
                shiftBuffer.assign(iqData->data.begin(), iqData->data.begin()+desired_input_size);
            }
            
            bool resampleChanged = !resampler || resampleBw != lastBandwidth || lastInputBandwidth != iqData->sampleRate;
            // Recreation waits out the throttle window mid-gesture; a pending
            // change retries on a later frame once the window elapses.
            if (resampleChanged && resampleGate.shouldFire(steadyClockMs(), SPECTRUM_RESAMPLE_MIN_INTERVAL_MS)) {
                float As = 60.0;
                
                if (resampler) {
                    msresamp_crcf_destroy(resampler);
                }
                
                resampler = msresamp_crcf_create(resamplerRatio, As);
                
                bwDiff = resampleBw-lastBandwidth;
                lastBandwidth = resampleBw;
                lastInputBandwidth = iqData->sampleRate;
                newResampler = true;
                peakReset = PEAK_RESET_COUNT;
            }
             
            unsigned int out_size = ceil((double) (desired_input_size) * resamplerRatio) + 512;
            
            if (resampleBuffer.size() != out_size) {
                if (resampleBuffer.capacity() < out_size) {
                    resampleBuffer.reserve(out_size);
                }
                resampleBuffer.resize(out_size);
            }
            
            msresamp_crcf_execute(resampler, &shiftBuffer[0], desired_input_size, &resampleBuffer[0], &num_written);
            
            if (num_written < fftSizeInternal) {
                memcpy(fftInData, resampleBuffer.data(), num_written * sizeof(liquid_float_complex));
                memset(&(fftInData[num_written]), 0, (fftSizeInternal-num_written) * sizeof(liquid_float_complex));
            } else {
                memcpy(fftInData, resampleBuffer.data(), fftSizeInternal * sizeof(liquid_float_complex));
            }
        } else {
            this->desiredInputSize = fftSizeInternal;

            num_written = data->size();
            if (data->size() < fftSizeInternal) {
                memcpy(fftInData, data->data(), data->size() * sizeof(liquid_float_complex));
                memset(&fftInData[data->size()], 0, (fftSizeInternal - data->size()) * sizeof(liquid_float_complex));
            } else {
                memcpy(fftInData, data->data(), fftSizeInternal * sizeof(liquid_float_complex));
            }
        }
        
        bool execute = false;

        if (num_written >= fftSizeInternal) {
            execute = true;
            memcpy(fftInput, fftInData, fftSizeInternal * sizeof(liquid_float_complex));
            memcpy(fftLastData, fftInput, fftSizeInternal * sizeof(liquid_float_complex));
            
        } else {
            if (lastDataSize + num_written < fftSizeInternal) { // priming
                unsigned int num_copy = fftSizeInternal - lastDataSize;
                if (num_written > num_copy) {
                    num_copy = num_written;
                }
                memcpy(fftLastData, fftInData, num_copy * sizeof(liquid_float_complex));
                lastDataSize += num_copy;
            } else {
                unsigned int num_last = (fftSizeInternal - num_written);
                memcpy(fftInput, fftLastData + (lastDataSize - num_last), num_last * sizeof(liquid_float_complex));
                memcpy(fftInput + num_last, fftInData, num_written * sizeof(liquid_float_complex));
                memcpy(fftLastData, fftInput, fftSizeInternal * sizeof(liquid_float_complex));
                execute = true;
            }
        }
        
        if (execute) {
            for (unsigned int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                fftInput[i].real *= fftWindow[i];
                fftInput[i].imag *= fftWindow[i];
            }

            SpectrumVisualDataPtr output = outputBuffers.getBuffer();
            
            if (output->spectrum_points.size() != fftSize * 2) {
                output->spectrum_points.resize(fftSize * 2);
            }
            if (doPeak) {
                if (output->spectrum_hold_points.size() != fftSize * 2) {
                    output->spectrum_hold_points.resize(fftSize * 2);
                }
            } else {
                output->spectrum_hold_points.resize(0);
            }
            

            fft_execute(fftPlan);
            
            for (int i = 0, iMax = fftSizeInternal / 2; i < iMax; i++) {
                float a = fftOutput[i].real;
                float b = fftOutput[i].imag;
                float c = sqrt(a * a + b * b);
                
                float x = fftOutput[fftSizeInternal / 2 + i].real;
                float y = fftOutput[fftSizeInternal / 2 + i].imag;
                float z = sqrt(x * x + y * y);
                
                fft_result[i] = (z);
                fft_result[fftSizeInternal / 2 + i] = (c);
            }
            
            if (newResampler && lastView) {
                if (bwDiff < 0) {
                    for (unsigned int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_temp[i] = fft_result_ma[(fftSizeInternal/4) + (i/2)];
                    }
                    for (unsigned int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_ma[i] = fft_result_temp[i];
                        
                        fft_result_temp[i] = fft_result_maa[(fftSizeInternal/4) + (i/2)];
                    }
                    for (unsigned int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_maa[i] = fft_result_temp[i];
                    }
                } else {
                    double dbFloorFill = magToDb(fft_floor_maa);
                    for (size_t i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        if (i < fftSizeInternal/4) {
                            fft_result_temp[i] = dbFloorFill; // fft_result_ma[fftSizeInternal/4];
                        } else if (i >= fftSizeInternal - fftSizeInternal/4) {
                            fft_result_temp[i] = dbFloorFill; // fft_result_ma[fftSizeInternal - fftSizeInternal/4-1];
                        } else {
                            fft_result_temp[i] = fft_result_ma[(i-fftSizeInternal/4)*2];
                        }
                    }
                    for (unsigned int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_ma[i] = fft_result_temp[i];
                        
                        if (i < fftSizeInternal/4) {
                            fft_result_temp[i] = dbFloorFill; //fft_result_maa[fftSizeInternal/4];
                        } else if (i >= fftSizeInternal - fftSizeInternal/4) {
                            fft_result_temp[i] = dbFloorFill; // fft_result_maa[fftSizeInternal - fftSizeInternal/4-1];
                        } else {
                            fft_result_temp[i] = fft_result_maa[(i-fftSizeInternal/4)*2];
                        }
                    }
                    for (unsigned int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_maa[i] = fft_result_temp[i];
                    }
                }
            }
            
            for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                double db = magToDb(fft_result[i]);
                if (fft_result_maa[i] != fft_result_maa[i]) fft_result_maa[i] = db;
                fft_result_maa[i] = iirStep(fft_result_maa[i], fft_result_ma[i], fft_average_rate);
                if (fft_result_ma[i] != fft_result_ma[i]) fft_result_ma[i] = db;
                fft_result_ma[i] = iirStep(fft_result_ma[i], db, fft_average_rate);
                fft_result_avg[i] = dbToMag(fft_result_maa[i]);
                
                if (doPeak) {
                    if (fft_result_avg[i] > fft_result_peak[i]) {
                        fft_result_peak[i] = fft_result_avg[i];
                    }
                }
            }
            
            SpectrumRange range = scanSpectrumRange(fft_result_avg, hideDC, fftSizeInternal / 2);
            double fft_ceil = range.ceil, fft_floor = range.floor;

            if (fft_ceil_ma != fft_ceil_ma) fft_ceil_ma = fft_ceil;
            fft_ceil_ma = fft_ceil_ma + (fft_ceil - fft_ceil_ma) * 0.05;
            if (fft_ceil_maa != fft_ceil_maa) fft_ceil_maa = fft_ceil;
            fft_ceil_maa = fft_ceil_maa + (fft_ceil_ma - fft_ceil_maa) * 0.05;
            
            if (fft_floor_ma != fft_floor_ma) fft_floor_ma = fft_floor;
            fft_floor_ma = fft_floor_ma + (fft_floor - fft_floor_ma) * 0.05;
            if (fft_floor_maa != fft_floor_maa) fft_floor_maa = fft_floor;
            fft_floor_maa = fft_floor_maa + (fft_floor_ma - fft_floor_maa) * 0.05;

            if (doPeak) {
                if (fft_ceil_maa > fft_ceil_peak) {
                    fft_ceil_peak = fft_ceil_maa;
                }
                if (fft_floor_maa < fft_floor_peak) {
                    fft_floor_peak = fft_floor_maa;
                }
            }
            
            float sf = scaleFactor;
 
            double visualRatio = (double(bandwidth) / double(resampleBw));
            double visualStart = (double(fftSizeInternal) / 2.0) - (double(fftSizeInternal) * (visualRatio / 2.0));
            double visualAccum = 0;
            double accCount = 0, i = 0;
            std::vector<unsigned int> pixelIdx;
   
            double point_ceil = doPeak?fft_ceil_peak:fft_ceil_maa;
            double point_floor = doPeak?fft_floor_peak:fft_floor_maa;
            
            for (int x = 0, xMax = output->spectrum_points.size() / 2; x < xMax; x++) {
                visualAccum += visualRatio * double(SPECTRUM_VZM);
                pixelIdx.clear();

                while (visualAccum >= 1.0) {
                    pixelIdx.push_back((unsigned int)round(visualStart+i));
                    accCount += 1.0;
                    visualAccum -= 1.0;
                    i++;
                }

                output->spectrum_points[x * 2] = ((float) x / (float) xMax);
                if (doPeak) {
                    output->spectrum_hold_points[x * 2] = ((float) x / (float) xMax);
                }
                if (accCount) {
                    double pix = reducePixelMax(fft_result_avg, pixelIdx, fft_floor_maa);
                    output->spectrum_points[x * 2 + 1] = ((log10(pix+0.25 - (point_floor-0.75)) / log10((point_ceil+0.25) - (point_floor-0.75))))*sf;
                    if (doPeak) {
                        double ppix = reducePixelMax(fft_result_peak, pixelIdx, fft_floor_maa);
                        output->spectrum_hold_points[x * 2 + 1] = ((log10(ppix+0.25 - (point_floor-0.75)) / log10((point_ceil+0.25) - (point_floor-0.75))))*sf;
                    }
                    accCount = 0.0;
                }
            }
            
            if (hideDC) { // DC-spike removal
                long long freqMin = centerFreq-(bandwidth/2);
                long long freqMax = centerFreq+(bandwidth/2);
                long long zeroPt = (iqData->frequency-freqMin);
                
                if (freqMin < iqData->frequency && freqMax > iqData->frequency) {
                    int freqRange = int(freqMax-freqMin);
                    DCRange dcPatch = dcPatchRange(zeroPt, freqRange, fftSize);
                    int fftStart = dcPatch.start;
                    int fftEnd = dcPatch.end;
                    
                    int numSteps = (fftEnd-fftStart);
                    int halfWay = fftStart+(numSteps/2);

                    if ((fftEnd+numSteps/2+1 < (long long) fftSize) && (fftStart-numSteps/2-1 >= 0) && (fftEnd > fftStart)) {
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
                        if (doPeak) {
                            int n = 1;
                            for (int i = fftStart; i < halfWay; i++) {
                                output->spectrum_hold_points[i * 2 + 1] = output->spectrum_hold_points[(fftStart - n) * 2 + 1];
                                n++;
                            }
                            n = 1;
                            for (int i = halfWay; i < fftEnd; i++) {
                                output->spectrum_hold_points[i * 2 + 1] = output->spectrum_hold_points[(fftEnd + n) * 2 + 1];
                                n++;
                            }
                        }
                    }
                }
            }
            
            output->fft_ceiling = point_ceil/sf;
            output->fft_floor = point_floor;

            output->centerFreq = centerFreq;
            output->bandwidth = bandwidth;

            distribute(output);
        }
    }  
    
    lastView = is_view;
}


void SpectrumVisualProcessor::setScaleFactor(float sf) {
	std::lock_guard < std::mutex > busy_lock(busy_run);

    scaleFactor = sf;
}


float SpectrumVisualProcessor::getScaleFactor() {
	std::lock_guard < std::mutex > busy_lock(busy_run);
    return scaleFactor;
}


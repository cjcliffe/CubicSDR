// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ScopeVisualProcessor.h"
#include <cstring>

ScopeVisualProcessor::ScopeVisualProcessor(): outputBuffers("ScopeVisualProcessorBuffers") {
    scopeEnabled.store(true);
    spectrumEnabled.store(true);
    fft_average_rate = 0.65f;
	fft_ceil_ma = fft_ceil_maa = 0;
	fft_floor_ma = fft_floor_maa = 0;
    maxScopeSamples = DEFAULT_DMOD_FFT_SIZE;
    fftPlan = nullptr;
}

ScopeVisualProcessor::~ScopeVisualProcessor() {
    if (fftPlan) {
        fft_destroy_plan(fftPlan);
    }
}


void ScopeVisualProcessor::setup(int fftSize_in) {
    fftSize = fftSize_in;
    desiredInputSize = fftSize;
 
    fftInData.resize(fftSize);
    fftOutput.resize(fftSize);

    if (fftPlan) {
        fft_destroy_plan(fftPlan);
    }
    fftPlan = fft_create_plan(fftSize, fftInData.data(), fftOutput.data(), LIQUID_FFT_FORWARD, 0);
}

void ScopeVisualProcessor::setScopeEnabled(bool scopeEnable) {
    scopeEnabled.store(scopeEnable);
}

void ScopeVisualProcessor::setSpectrumEnabled(bool spectrumEnable) {
    spectrumEnabled.store(spectrumEnable);
}

void ScopeVisualProcessor::process() {
    if (!isOutputEmpty()) {
        return;
    }
    AudioThreadInputPtr audioInputData;

    if (input->try_pop(audioInputData)) {
          
        if (!audioInputData) {
            return;
        }
        size_t i, iMax = audioInputData->data.size();
        if (!iMax) {
            //discard audioInputData.
            audioInputData = nullptr;
            return;
        }
                
        ScopeRenderDataPtr renderData = nullptr;
        
        if (scopeEnabled) {
            iMax = audioInputData->data.size();
            if (iMax > maxScopeSamples) {
                iMax = maxScopeSamples;
            }

            renderData = outputBuffers.getBuffer();
            renderData->channels = audioInputData->channels;
            renderData->inputRate = audioInputData->inputRate;
            renderData->sampleRate = audioInputData->sampleRate;

            if (renderData->waveform_points.size() != iMax * 2) {
                renderData->waveform_points.resize(iMax * 2);
            }

            float peak = 1.0f;
            
            for (i = 0; i < iMax; i++) {
                float p = fabs(audioInputData->data[i]);
                if (p > peak) {
                    peak = p;
                }
            }
            
            if (audioInputData->type == 1) {
                iMax = audioInputData->data.size();
                if (renderData->waveform_points.size() != iMax * 2) {
                    renderData->waveform_points.resize(iMax * 2);
                }
                for (i = 0; i < iMax; i++) {
                    renderData->waveform_points[i * 2] = (((double) (i % (iMax/2)) / (double) iMax) * 2.0 - 0.5) * 2.0;
                    renderData->waveform_points[i * 2 + 1] = audioInputData->data[i] / peak;
                }
                renderData->mode = ScopePanel::SCOPE_MODE_2Y;
            } else if (audioInputData->type == 2) {
                iMax = audioInputData->data.size();
                if (renderData->waveform_points.size() != iMax) {
                    renderData->waveform_points.resize(iMax);
                }
                for (i = 0; i < iMax/2; i++) {
                    renderData->waveform_points[i * 2] = audioInputData->data[i * 2] / peak;
                    renderData->waveform_points[i * 2 + 1] = audioInputData->data[i * 2 + 1] / peak;
                }
                renderData->mode = ScopePanel::SCOPE_MODE_XY;
            } else {
                for (i = 0; i < iMax; i++) {
                    renderData->waveform_points[i * 2] = (((double) i / (double) iMax) - 0.5) * 2.0;
                    renderData->waveform_points[i * 2 + 1] = audioInputData->data[i] / peak;
                }
                renderData->mode = ScopePanel::SCOPE_MODE_Y;
            }

            renderData->spectrum = false;
            distribute(renderData);
        }
        
        if (spectrumEnabled) {
            iMax = audioInputData->data.size();

            if (audioInputData->channels==1) {
                for (i = 0; i < fftSize; i++) {
                    if (i < iMax) {
                        fftInData[i].real = audioInputData->data[i];
                        fftInData[i].imag = 0;
                    } else {
                        fftInData[i].real = 0;
                        fftInData[i].imag = 0;
                    }
                }
            } else if (audioInputData->channels==2) {
                iMax = iMax/2;
                for (i = 0; i < fftSize; i++) {
                    if (i < iMax) {
                        fftInData[i].real = audioInputData->data[i] + audioInputData->data[iMax+i];
                        fftInData[i].imag = 0;
                    } else {
                        fftInData[i].real = 0;
                        fftInData[i].imag = 0;
                    }
                }
            }
            
            renderData = outputBuffers.getBuffer();

            renderData->channels = audioInputData->channels;
            renderData->inputRate = audioInputData->inputRate;
            renderData->sampleRate = audioInputData->sampleRate;
            
            audioInputData = nullptr; //->decRefCount();

            double fft_ceil = 0, fft_floor = 1;
            
            if (fft_result.size() < (fftSize/2)) {
                fft_result.resize((fftSize/2));
                fft_result_ma.resize((fftSize/2));
                fft_result_maa.resize((fftSize/2));
            }

            fft_execute(fftPlan);
            for (i = 0; i < (fftSize/2); i++) {

                //cast result to double to prevent overflows / excessive precision losses in the following computations...
                double a = (double) fftOutput[i].real;
                double b = (double) fftOutput[i].imag;

                //computes norm = sqrt(a**2 + b**2)
                //being actually floats cast into doubles, we are indeed overflow-free here.
                fft_result[i] = sqrt(a*a + b*b);
            }
            
            for (i = 0; i < (fftSize/2); i++) {
                fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * fft_average_rate;
				fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * fft_average_rate;

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

            unsigned int outSize = fftSize/2;
            
            if (renderData->sampleRate != renderData->inputRate) {
                outSize = (int)floor((float)outSize * ((float)renderData->sampleRate/(float)renderData->inputRate));
            }
            
            if (renderData->waveform_points.size() != outSize*2) {
                renderData->waveform_points.resize(outSize*2);
            }
            
            for (i = 0; i < outSize; i++) {
                float v = (log10(fft_result_maa[i]+0.25 - (fft_floor_maa-0.75)) / log10((fft_ceil_maa+0.25) - (fft_floor_maa-0.75)));
                renderData->waveform_points[i * 2] = ((double) i / (double) (outSize));
                renderData->waveform_points[i * 2 + 1] = v;
            }
            
            renderData->fft_floor = fft_floor_maa;
            renderData->fft_ceil = fft_ceil_maa;
            renderData->fft_size = fftSize/2;
            renderData->spectrum = true;

            distribute(renderData);
        } 
    } //end if try_pop()
}

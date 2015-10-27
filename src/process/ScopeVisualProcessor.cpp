#include "ScopeVisualProcessor.h"
#include <cstring>
#include <string>

ScopeVisualProcessor::ScopeVisualProcessor(): fftInData(NULL), fftwOutput(NULL), fftw_plan(NULL), maxScopeSamples(1024) {
    scopeEnabled.store(true);
    spectrumEnabled.store(true);
    fft_average_rate = 0.65;
	fft_ceil_ma = fft_ceil_maa = 0;
	fft_floor_ma = fft_floor_maa = 0;
}

ScopeVisualProcessor::~ScopeVisualProcessor() {
    /*if (fftInData) {
        free(fftInData);
    }
    if (fftwOutput) {
        free(fftwOutput);
    }*/
    if (fftw_plan) {
        fftwf_destroy_plan(fftw_plan);
    }
}


void ScopeVisualProcessor::setup(int fftSize_in) {
    fftSize = fftSize_in;
    desiredInputSize = fftSize;
    
    if (fftInData) {
        free(fftInData);
    }
    fftInData = (float*) fftwf_malloc(sizeof(float) * fftSize);
    if (fftwOutput) {
        free(fftwOutput);
    }
    fftwOutput = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);
    if (fftw_plan) {
        fftwf_destroy_plan(fftw_plan);
    }
    fftw_plan = fftwf_plan_dft_r2c_1d(fftSize, fftInData, fftwOutput, FFTW_ESTIMATE);
    //(fftSize, fftInData, fftwOutput, 0);
    //(fftSize, fftwInput, fftwOutput, FFTW_R2HC, FFTW_ESTIMATE);
    
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
    if (!input->empty()) {
        AudioThreadInput *audioInputData;
        input->pop(audioInputData);
        
        if (!audioInputData) {
            return;
        }
        int i, iMax = audioInputData->data.size();
        if (!iMax) {
            audioInputData->decRefCount();
            return;
        }
                
        ScopeRenderData *renderData = NULL;
        
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
            
            if (audioInputData->channels == 2) {
                iMax = audioInputData->data.size();
                if (renderData->waveform_points.size() != iMax * 2) {
                    renderData->waveform_points.resize(iMax * 2);
                }
                for (i = 0; i < iMax; i++) {
                    renderData->waveform_points[i * 2] = (((double) (i % (iMax/2)) / (double) iMax) * 2.0 - 0.5) * 2.0;
                    renderData->waveform_points[i * 2 + 1] = audioInputData->data[i] / peak;
                }
            } else {
                for (i = 0; i < iMax; i++) {
                    renderData->waveform_points[i * 2] = (((double) i / (double) iMax) - 0.5) * 2.0;
                    renderData->waveform_points[i * 2 + 1] = audioInputData->data[i] / peak;
                }
            }

            renderData->spectrum = false;
            
            distribute(renderData);
        }
        
        if (spectrumEnabled) {
            iMax = audioInputData->data.size();

            if (audioInputData->channels==1) {
                for (i = 0; i < fftSize; i++) {
                    if (i < iMax) {
                        fftInData[i] = audioInputData->data[i];
                    } else {
                        fftInData[i] = 0;
                    }
                }
            } else if (audioInputData->channels==2) {
                iMax = iMax/2;
                for (i = 0; i < fftSize; i++) {
                    if (i < iMax) {
                        fftInData[i] = audioInputData->data[i] + audioInputData->data[iMax+i];
                    } else {
                        fftInData[i] = 0;
                    }
                }
            }

            
            fftwf_execute(fftw_plan);
            
            float fft_ceil = 0, fft_floor = 1;
            
            if (fft_result.size() < (fftSize/2)) {
                fft_result.resize((fftSize/2));
                fft_result_ma.resize((fftSize/2));
                fft_result_maa.resize((fftSize/2));
            }
            
            for (i = 0; i < (fftSize/2); i++) {
                float a = fftwOutput[i][0];
                float b = fftwOutput[i][1];
                fft_result[i] = sqrt( a * a + b * b);
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
            
			if (fft_floor == fft_ceil) {
				audioInputData->decRefCount();
				return;
			}

            fft_ceil_ma = fft_ceil_ma + (fft_ceil - fft_ceil_ma) * 0.05;
            fft_ceil_maa = fft_ceil_maa + (fft_ceil_ma - fft_ceil_maa) * 0.05;
            
            fft_floor_ma = fft_floor_ma + (fft_floor - fft_floor_ma) * 0.05;
            fft_floor_maa = fft_floor_maa + (fft_floor_ma - fft_floor_maa) * 0.05;

            int outSize = fftSize/2;
            
            if (audioInputData->sampleRate != audioInputData->inputRate) {
                outSize = (int)floor((float)outSize * ((float)audioInputData->sampleRate/(float)audioInputData->inputRate));
            }
            
			renderData = outputBuffers.getBuffer();

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
            renderData->inputRate = audioInputData->inputRate;
            renderData->sampleRate = audioInputData->sampleRate;
            renderData->spectrum = true;
            distribute(renderData);
        }
        
        audioInputData->decRefCount();
    }
}

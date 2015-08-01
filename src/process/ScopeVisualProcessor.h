#pragma once

#include "VisualProcessor.h"
#include "AudioThread.h"

class ScopeRenderData: public ReferenceCounter {
public:
	std::vector<float> waveform_points;
	int channels;
};

typedef ThreadQueue<ScopeRenderData *> ScopeRenderDataQueue;

class ScopeVisualProcessor : public VisualProcessor<AudioThreadInput, ScopeRenderData> {
protected:
    virtual void process() {
    	if (isOutputEmpty()) {
    		return;
    	}
        if (!input->empty()) {
            AudioThreadInput *audioInputData;
            input->pop(audioInputData);

            if (!audioInputData) {
                return;
            }
            int iMax = audioInputData->data.size();
            if (!iMax) {
                audioInputData->decRefCount();
                return;
            }

            ScopeRenderData *renderData = outputBuffers.getBuffer();
            renderData->channels = audioInputData->channels;

            if (renderData->waveform_points.size() != iMax * 2) {
            	renderData->waveform_points.resize(iMax * 2);
            }
            
            for (int i = 0; i < iMax; i++) {
            	renderData->waveform_points[i * 2 + 1] = audioInputData->data[i] * 0.5f;
            	renderData->waveform_points[i * 2] = ((double) i / (double) iMax);
            }
            
            distribute(renderData);
            // ati->channels
        }
    }

    ReBuffer<ScopeRenderData> outputBuffers;
};


/*
 if (!wxGetApp().getAudioVisualQueue()->empty()) {
            AudioThreadInput *demodAudioData;
            wxGetApp().getAudioVisualQueue()->pop(demodAudioData);
            
            int iMax = demodAudioData?demodAudioData->data.size():0;
            
            if (demodAudioData && iMax) {
                if (waveform_points.size() != iMax * 2) {
                    waveform_points.resize(iMax * 2);
                }
                
                demodAudioData->busy_update.lock();
                
                for (int i = 0; i < iMax; i++) {
                    waveform_points[i * 2 + 1] = demodAudioData->data[i] * 0.5f;
                    waveform_points[i * 2] = ((double) i / (double) iMax);
                }
                
                demodAudioData->busy_update.unlock();
                
                setStereo(demodAudioData->channels == 2);
            } else {
                std::cout << "Incoming Demodulator data empty?" << std::endl;
            }
        }
*/

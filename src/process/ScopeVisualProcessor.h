#pragma once

#include "VisualProcessor.h"
#include "AudioThread.h"

class ScopeVisualProcessor : public VisualProcessor {
protected:
    std::vector<float> waveform_points;

    virtual void process() {
        if (!input->empty()) {
            ReferenceCounter *ati_ref;
            input->pop(ati_ref);
            
            AudioThreadInput *ati = (AudioThreadInput *)ati_ref;
            if (!ati) {
                return;
            }
            int iMax = ati->data.size();
            if (!iMax) {
                ati->decRefCount();
                return;
            }
            if (waveform_points.size() != iMax * 2) {
                waveform_points.resize(iMax * 2);
            }
            
            for (int i = 0; i < iMax; i++) {
                waveform_points[i * 2 + 1] = ati->data[i] * 0.5f;
                waveform_points[i * 2] = ((double) i / (double) iMax);
            }
            
            // ati->channels
        }
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
    }
};
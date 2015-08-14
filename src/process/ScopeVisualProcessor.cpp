#include "ScopeVisualProcessor.h"

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
        int iMax = audioInputData->data.size();
        if (!iMax) {
            audioInputData->decRefCount();
            return;
        }
        
        audioInputData->busy_update.lock();
        ScopeRenderData *renderData = outputBuffers.getBuffer();
        renderData->channels = audioInputData->channels;
        
        if (renderData->waveform_points.size() != iMax * 2) {
            renderData->waveform_points.resize(iMax * 2);
        }

        float peak = 1.0f;
        
        for (int i = 0; i < iMax; i++) {
            float p = fabs(audioInputData->data[i]);
            if (p > peak) {
                peak = p;
            }
        }
        
        if (audioInputData->channels == 2) {
            for (int i = 0; i < iMax; i++) {
                renderData->waveform_points[i * 2] = (((double) (i % (iMax/2)) / (double) iMax) * 2.0 - 0.5) * 2.0;
                renderData->waveform_points[i * 2 + 1] = audioInputData->data[i] / peak;
            }
        } else {
            for (int i = 0; i < iMax; i++) {
                renderData->waveform_points[i * 2] = (((double) i / (double) iMax) - 0.5) * 2.0;
                renderData->waveform_points[i * 2 + 1] = audioInputData->data[i] / peak;
            }
        }

        distribute(renderData);
        audioInputData->busy_update.unlock();
    }
}

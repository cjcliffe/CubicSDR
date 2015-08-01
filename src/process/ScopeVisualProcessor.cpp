#include "ScopeVisualProcessor.h"

void ScopeVisualProcessor::process() {
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
    }
}

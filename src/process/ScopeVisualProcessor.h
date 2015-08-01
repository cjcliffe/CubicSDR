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
    void process();
    ReBuffer<ScopeRenderData> outputBuffers;
};

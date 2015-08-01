#pragma once

#include "VisualProcessor.h"
#include "SpectrumCanvas.h"

class SpectrumVisualData : public ReferenceCounter {

};

class SpectrumVisualProcessor : public VisualProcessor<DemodulatorThreadIQData, SpectrumVisualData> {
protected:
    void process();
};

#pragma once

#include "VisualProcessor.h"
#include "WaterfallCanvas.h"

class WaterfallVisualData : public ReferenceCounter {

};

class WaterfallVisualProcessor : public VisualProcessor<DemodulatorThreadIQData, WaterfallVisualData> {
protected:
    void process();
};

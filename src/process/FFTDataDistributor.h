#pragma once

#include "VisualProcessor.h"
#include "DemodDefs.h"
#include <cmath>

class FFTDataDistributor : public VisualProcessor<DemodulatorThreadIQData, DemodulatorThreadIQData> {
public:
    FFTDataDistributor();
    void setFFTSize(int fftSize);
    void setLinesPerSecond(int lines);
    int getLinesPerSecond();

protected:
    void process();
    
    DemodulatorThreadIQData inputBuffer, tempBuffer;
    ReBuffer<DemodulatorThreadIQData> outputBuffers;
    int fftSize;
    int linesPerSecond;
    double lineRateAccum;
    int bufferMax, bufferOffset, bufferedItems;
};

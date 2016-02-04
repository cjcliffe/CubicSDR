#pragma once

#include "VisualProcessor.h"
#include "DemodDefs.h"
#include <cmath>
#include <cstring>

class FFTDataDistributor : public VisualProcessor<DemodulatorThreadIQData, DemodulatorThreadIQData> {
public:
    FFTDataDistributor();
    void setFFTSize(unsigned int fftSize);
    void setLinesPerSecond(unsigned int lines);
    unsigned int getLinesPerSecond();

protected:
    void process();
    
    DemodulatorThreadIQData inputBuffer, tempBuffer;
    ReBuffer<DemodulatorThreadIQData> outputBuffers;
    unsigned int fftSize;
    unsigned int linesPerSecond;
    double lineRateAccum;
    size_t bufferMax, bufferOffset, bufferedItems;
};

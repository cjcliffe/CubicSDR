// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "VisualProcessor.h"
#include "DemodDefs.h"
#include <cmath>
#include <cstring>
#include <atomic>

class FFTDataDistributor : public VisualProcessor<DemodulatorThreadIQData, DemodulatorThreadIQData> {
public:
    FFTDataDistributor();
    void setFFTSize(unsigned int size);
    void setLinesPerSecond(unsigned int lines);
    unsigned int getLinesPerSecond();

protected:
    virtual void process();
    
    DemodulatorThreadIQData inputBuffer, tempBuffer;
    ReBuffer<DemodulatorThreadIQData> outputBuffers;
    std::atomic<unsigned int> fftSize;
   
    unsigned int linesPerSecond;
    double lineRateAccum;
    size_t bufferMax = 0;
    size_t bufferOffset = 0;
    size_t bufferedItems = 0;
};

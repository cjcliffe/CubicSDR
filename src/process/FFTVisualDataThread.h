// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include <memory>
#include "IOThread.h"
#include "SpectrumVisualProcessor.h"
#include "FFTDataDistributor.h"

class FFTVisualDataThread : public IOThread {
public:
    FFTVisualDataThread();
    ~FFTVisualDataThread();
    
    void setLinesPerSecond(int lps);
    int getLinesPerSecond();
    SpectrumVisualProcessor *getProcessor();
    
    virtual void run();
    
protected:
    FFTDataDistributor fftDistrib;
    DemodulatorThreadInputQueuePtr fftQueue = std::make_shared<DemodulatorThreadInputQueue>();
    SpectrumVisualProcessor wproc;
    
    std::atomic_int linesPerSecond;
    std::atomic_bool lpsChanged;
};

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
    ~FFTVisualDataThread() override;
    
    void setLinesPerSecond(int lps);
    int getLinesPerSecond();
    SpectrumVisualProcessor *getProcessor();
    
    void run() override;

    void terminate() override;
    
protected:
    FFTDataDistributor fftDistrib;
    DemodulatorThreadInputQueuePtr fftQueue = std::make_shared<DemodulatorThreadInputQueue>();
    SpectrumVisualProcessor wproc;
    
    std::atomic_int linesPerSecond;
    std::atomic_bool lpsChanged;
};

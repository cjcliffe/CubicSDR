// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "IOThread.h"
#include "SpectrumVisualProcessor.h"

class SpectrumVisualDataThread : public IOThread {
public:
    SpectrumVisualDataThread();
    ~SpectrumVisualDataThread() override;
    SpectrumVisualProcessor *getProcessor();
    
    void run() override;

    void terminate() override;
    
protected:
    SpectrumVisualProcessor sproc;
};

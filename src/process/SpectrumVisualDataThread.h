// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "IOThread.h"
#include "SpectrumVisualProcessor.h"

class SpectrumVisualDataThread : public IOThread {
public:
    SpectrumVisualDataThread();
    ~SpectrumVisualDataThread();
    SpectrumVisualProcessor *getProcessor();
    
    virtual void run();

    virtual void terminate();
    
protected:
    SpectrumVisualProcessor sproc;
};

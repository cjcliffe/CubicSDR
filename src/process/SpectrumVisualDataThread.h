#pragma once

#include "IOThread.h"
#include "SpectrumVisualProcessor.h"

class SpectrumVisualDataThread : public IOThread {
public:
    SpectrumVisualDataThread();
    ~SpectrumVisualDataThread();
    SpectrumVisualProcessor *getProcessor();
    
    void run();
    
protected:
    SpectrumVisualProcessor sproc;
};

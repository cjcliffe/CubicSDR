#include "SpectrumVisualDataThread.h"
#include "CubicSDR.h"

SpectrumVisualDataThread::SpectrumVisualDataThread() {
}

SpectrumVisualDataThread::~SpectrumVisualDataThread() {
    
}

SpectrumVisualProcessor *SpectrumVisualDataThread::getProcessor() {
    return &sproc;
}

void SpectrumVisualDataThread::run() {
    std::cout << "Spectrum visual data thread started." << std::endl;
    
    while(!terminated) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        sproc.run();
    }
    
    std::cout << "Spectrum visual data thread done." << std::endl;
}


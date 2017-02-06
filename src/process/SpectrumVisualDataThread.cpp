// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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
    
    while(!stopping) {
        //this if fed by FFTDataDistributor which has a buffer of FFT_DISTRIBUTOR_BUFFER_IN_SECONDS
        //so sleep for << FFT_DISTRIBUTOR_BUFFER_IN_SECONDS not to be overflown
        std::this_thread::sleep_for(std::chrono::milliseconds((int)(FFT_DISTRIBUTOR_BUFFER_IN_SECONDS * 1000.0 / 25.0)));

        sproc.run();
    }
    
//    std::cout << "Spectrum visual data thread done." << std::endl;
}


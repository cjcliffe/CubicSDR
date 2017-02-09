// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "FFTVisualDataThread.h"
#include "CubicSDR.h"

FFTVisualDataThread::FFTVisualDataThread() {
	linesPerSecond.store(DEFAULT_WATERFALL_LPS);
    lpsChanged.store(true);
}

FFTVisualDataThread::~FFTVisualDataThread() {
    
}

void FFTVisualDataThread::setLinesPerSecond(int lps) {
    linesPerSecond.store(lps);
    lpsChanged.store(true);
}

int FFTVisualDataThread::getLinesPerSecond() {
    return linesPerSecond.load();
}

SpectrumVisualProcessor *FFTVisualDataThread::getProcessor() {
    return &wproc;
}

void FFTVisualDataThread::run() {
    DemodulatorThreadInputQueue *pipeIQDataIn = static_cast<DemodulatorThreadInputQueue *>(getInputQueue("IQDataInput"));
    SpectrumVisualDataQueue *pipeFFTDataOut = static_cast<SpectrumVisualDataQueue *>(getOutputQueue("FFTDataOutput"));
    

    fftQueue.set_max_num_items(100); 
    pipeFFTDataOut->set_max_num_items(100);

    //FFT distributor plumbing:
    // IQDataInput push samples to process to FFT Data distributor. 
    fftDistrib.setInput(pipeIQDataIn);

    //The FFT distributor has actually 1 output only, so it doesn't distribute at all :) 
    fftDistrib.attachOutput(&fftQueue);
    
    //FFT Distributor output is ==> SpectrumVisualProcessor input.
    wproc.setInput(&fftQueue);
    wproc.attachOutput(pipeFFTDataOut);
    wproc.setup(DEFAULT_FFT_SIZE);

//    std::cout << "FFT visual data thread started." << std::endl;
    
    while(!stopping) {
        
        //this if fed by FFTDataDistributor which has a buffer of FFT_DISTRIBUTOR_BUFFER_IN_SECONDS
        //so sleep for << FFT_DISTRIBUTOR_BUFFER_IN_SECONDS not to be overflown
        std::this_thread::sleep_for(std::chrono::milliseconds((int)(FFT_DISTRIBUTOR_BUFFER_IN_SECONDS * 1000.0 / 25.0)));
        
        int fftSize = wproc.getDesiredInputSize();
        
        if (fftSize) {
            fftDistrib.setFFTSize(fftSize);
        } else {
            fftDistrib.setFFTSize(DEFAULT_FFT_SIZE * SPECTRUM_VZM);
        }
    
        if (lpsChanged.load()) {
            fftDistrib.setLinesPerSecond(linesPerSecond.load());
            lpsChanged.store(false);
        }
        
        //Make FFT Distributor process IQ samples
        //and package them into ready-to-FFT sample sets (representing 1 line) by wproc
        fftDistrib.run();
      
        // Make wproc do a FFT of each of the sample sets provided by fftDistrib: 
        while (!wproc.isInputEmpty()) {
            wproc.run();
        }
    }
    
//    std::cout << "FFT visual data thread done." << std::endl;
}


#include "FFTVisualDataThread.h"
#include "CubicSDR.h"

FFTVisualDataThread::FFTVisualDataThread() : linesPerSecond(DEFAULT_WATERFALL_LPS) {
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
    DemodulatorThreadInputQueue *pipeIQDataIn = (DemodulatorThreadInputQueue *)getInputQueue("IQDataInput");
    SpectrumVisualDataQueue *pipeFFTDataOut = (SpectrumVisualDataQueue *)getOutputQueue("FFTDataOutput");
    
    fftDistrib.setInput(pipeIQDataIn);
    fftDistrib.attachOutput(&fftQueue);
    wproc.setInput(&fftQueue);
    wproc.attachOutput(pipeFFTDataOut);
    wproc.setup(2048);

    std::cout << "FFT visual data thread started." << std::endl;
    
    while(!terminated) {
        
        std::this_thread::sleep_for(std::chrono::milliseconds(12));
        
        int fftSize = wproc.getDesiredInputSize();
        
        if (fftSize) {
            fftDistrib.setFFTSize(fftSize);
        } else {
            fftDistrib.setFFTSize(DEFAULT_FFT_SIZE);
        }
    
        if (lpsChanged.load()) {
            fftDistrib.setLinesPerSecond(linesPerSecond.load());
            pipeIQDataIn->set_max_num_items(linesPerSecond.load());
            lpsChanged.store(false);
        }
        
        fftDistrib.run();
        
        while (!wproc.isInputEmpty()) {
            wproc.run();
        }
    }
    
    std::cout << "FFT visual data thread done." << std::endl;
}


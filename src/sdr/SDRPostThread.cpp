#include "SDRPostThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include "CubicSDR.h"

SDRPostThread::SDRPostThread() :
        iqDataInQueue(NULL), iqDataOutQueue(NULL), iqVisualQueue(NULL), terminated(false) {
    dev = NULL;
    sample_rate = SRATE;
}

SDRPostThread::~SDRPostThread() {
    rtlsdr_close(dev);
}

void SDRPostThread::threadMain() {
    int n_read;
    double seconds = 0.0;

    dcFilter = iirfilt_crcf_create_dc_blocker(0.0005);

    liquid_float_complex x, y;

    std::cout << "SDR post-processing thread started.." << std::endl;

    while (!terminated) {
        SDRThreadIQData data_in;

        iqDataInQueue.load()->pop(data_in);

        if (data_in.data.size()) {
            SDRThreadIQData dataOut;

            dataOut.frequency = data_in.frequency;
            dataOut.bandwidth = data_in.bandwidth;
            dataOut.data = data_in.data;

            for (int i = 0, iMax = dataOut.data.size() / 2; i < iMax; i++) {
                x.real = (float) dataOut.data[i * 2] / 127.0;
                x.imag = (float) dataOut.data[i * 2 + 1] / 127.0;

                iirfilt_crcf_execute(dcFilter, x, &y);

                dataOut.data[i * 2] = (signed char) floor(y.real * 127.0);
                dataOut.data[i * 2 + 1] = (signed char) floor(y.imag * 127.0);
            }

            if (iqDataOutQueue != NULL) {
                iqDataOutQueue.load()->push(dataOut);
            }

            if (iqVisualQueue != NULL) {
                if (iqVisualQueue.load()->empty()) {
                    iqVisualQueue.load()->push(dataOut);
                }
            }

            if (demodulators.size()) {
                DemodulatorThreadIQData demodDataOut;
                demodDataOut.frequency = data_in.frequency;
                demodDataOut.bandwidth = data_in.bandwidth;
                demodDataOut.data = data_in.data;

                for (int i = 0, iMax = demodulators.size(); i < iMax; i++) {
                    DemodulatorInstance *demod = demodulators[i];
                    DemodulatorThreadInputQueue *demodQueue = demod->threadQueueDemod;
                    demodQueue->push(demodDataOut);
                }
            }
        }
    }
    std::cout << "SDR post-processing thread done." << std::endl;
}

void SDRPostThread::terminate() {
    terminated = true;
    SDRThreadIQData dummy;
    iqDataInQueue.load()->push(dummy);
}

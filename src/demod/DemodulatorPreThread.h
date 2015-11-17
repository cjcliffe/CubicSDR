#pragma once

#include <queue>
#include <vector>

#include "CubicSDRDefs.h"
#include "DemodDefs.h"
#include "DemodulatorWorkerThread.h"

class DemodulatorPreThread : public IOThread {
public:

    DemodulatorPreThread();
    ~DemodulatorPreThread();

    void run();

    DemodulatorThreadParameters &getParams() {
        return params;
    }

    void setParams(DemodulatorThreadParameters &params_in) {
        params = params_in;
    }

    void initialize();
    void terminate();

#ifdef __APPLE__
    static void *pthread_helper(void *context) {
        return ((DemodulatorPreThread *) context)->threadMain();
    }
#endif

protected:
    msresamp_crcf iqResampler;
    double iqResampleRatio;
    std::vector<liquid_float_complex> resampledData;

//    msresamp_rrrf audioResampler;
//    msresamp_rrrf stereoResampler;
//    double audioResampleRatio;

//    firfilt_rrrf firStereoLeft;
//    firfilt_rrrf firStereoRight;
//    iirfilt_crcf iirStereoPilot;

    DemodulatorThreadParameters params;
    DemodulatorThreadParameters lastParams;

    nco_crcf freqShifter;
    int shiftFrequency;

    std::atomic_bool initialized;

    DemodulatorWorkerThread *workerThread;
    std::thread *t_Worker;

    DemodulatorThreadWorkerCommandQueue *workerQueue;
    DemodulatorThreadWorkerResultQueue *workerResults;

    DemodulatorThreadInputQueue* iqInputQueue;
    DemodulatorThreadPostInputQueue* iqOutputQueue;
    DemodulatorThreadCommandQueue* threadQueueNotify;
    DemodulatorThreadCommandQueue* commandQueue;
};

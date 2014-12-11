#include <DemodulatorMgr.h>
#include <sstream>
#include <algorithm>
#include "CubicSDR.h"

DemodulatorInstance::DemodulatorInstance() :
        t_Demod(NULL), t_Audio(NULL), threadQueueDemod(NULL), demodulatorThread(NULL) {

    threadQueueDemod = new DemodulatorThreadInputQueue;
    threadQueueCommand = new DemodulatorThreadCommandQueue;
    demodulatorThread = new DemodulatorThread(threadQueueDemod);
    demodulatorThread->setCommandQueue(threadQueueCommand);
    audioInputQueue = new AudioThreadInputQueue;
    audioThread = new AudioThread(audioInputQueue);
    demodulatorThread->setAudioInputQueue(audioInputQueue);
}

DemodulatorInstance::~DemodulatorInstance() {

    delete audioThread;
    delete t_Audio;

    delete audioInputQueue;
    delete threadQueueDemod;
    delete demodulatorThread;
#ifndef __APPLE__
    delete t_Demod;
#endif
}

void DemodulatorInstance::setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
    demodulatorThread->setVisualOutputQueue(tQueue);
}

void DemodulatorInstance::run() {
    if (t_Demod) {
        terminate();

        delete threadQueueDemod;
        delete demodulatorThread;
        delete t_Demod;
        delete audioThread;
        delete audioInputQueue;
        delete t_Audio;

        threadQueueDemod = new DemodulatorThreadInputQueue;
        threadQueueCommand = new DemodulatorThreadCommandQueue;
        demodulatorThread = new DemodulatorThread(threadQueueDemod);
        demodulatorThread->setCommandQueue(threadQueueCommand);

        audioInputQueue = new AudioThreadInputQueue;
        audioThread = new AudioThread(audioInputQueue);

        demodulatorThread->setAudioInputQueue(audioInputQueue);
    }

    t_Audio = new std::thread(&AudioThread::threadMain, audioThread);

#ifdef __APPLE__	// Already using pthreads, might as well do some custom init..
    pthread_attr_t attr;
    size_t size;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 2048000);
    pthread_attr_getstacksize(&attr, &size);
    pthread_create(&t_Demod, &attr, &DemodulatorThread::pthread_helper, demodulatorThread);
    pthread_attr_destroy(&attr);

    std::cout << "Initialized demodulator stack size of " << size << std::endl;

#else
    t_Demod = new std::thread(&DemodulatorThread::threadMain, demodulatorThread);
#endif
}

DemodulatorThreadCommandQueue *DemodulatorInstance::getCommandQueue() {
    return threadQueueCommand;
}

DemodulatorThreadParameters &DemodulatorInstance::getParams() {
    return demodulatorThread->getParams();
}

void DemodulatorInstance::terminate() {
    std::cout << "Terminating demodulator thread.." << std::endl;
    demodulatorThread->terminate();
//#ifdef __APPLE__
//    pthread_join(t_Demod,NULL);
//#else
//#endif
    std::cout << "Terminating demodulator audio thread.." << std::endl;
    audioThread->terminate();
}

std::string DemodulatorInstance::getLabel() {
    return label;
}

void DemodulatorInstance::setLabel(std::string labelStr) {
    label = labelStr;
}

DemodulatorMgr::DemodulatorMgr() :
        activeDemodulator(NULL), lastActiveDemodulator(NULL), activeVisualDemodulator(NULL) {

}

DemodulatorMgr::~DemodulatorMgr() {
    terminateAll();
}

DemodulatorInstance *DemodulatorMgr::newThread() {
    DemodulatorInstance *newDemod = new DemodulatorInstance;

    demods.push_back(newDemod);

    std::stringstream label;
    label << demods.size();
    newDemod->setLabel(label.str());

    return newDemod;
}

void DemodulatorMgr::terminateAll() {
    while (demods.size()) {
        DemodulatorInstance *d = demods.back();
        demods.pop_back();
        d->terminate();
    }
}

std::vector<DemodulatorInstance *> &DemodulatorMgr::getDemodulators() {
    return demods;
}

void DemodulatorMgr::deleteThread(DemodulatorInstance *demod) {
    std::vector<DemodulatorInstance *>::iterator i;

    i = std::find(demods.begin(),demods.end(),demod);

    if (activeDemodulator == demod) {
        activeDemodulator = NULL;
    }
    if (lastActiveDemodulator == demod) {
        lastActiveDemodulator = NULL;
    }
    if (activeVisualDemodulator == demod) {
        activeVisualDemodulator = NULL;
    }

    if (i != demods.end()) {
        demods.erase(i);
        demod->terminate();
    }
}

std::vector<DemodulatorInstance *> *DemodulatorMgr::getDemodulatorsAt(int freq, int bandwidth) {
    std::vector<DemodulatorInstance *> *foundDemods = new std::vector<DemodulatorInstance *>();

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        DemodulatorInstance *testDemod = demods[i];

        int freqTest = testDemod->getParams().frequency;
        int bandwidthTest = testDemod->getParams().bandwidth;
        int halfBandwidthTest = bandwidthTest / 2;

        int halfBuffer = bandwidth / 2;

        if ((freq <= (freqTest + halfBandwidthTest + halfBuffer)) && (freq >= (freqTest - halfBandwidthTest - halfBuffer))) {
            foundDemods->push_back(testDemod);
        }
    }

    return foundDemods;
}

void DemodulatorMgr::setActiveDemodulator(DemodulatorInstance *demod, bool temporary) {
    if (!temporary) {
        if (activeDemodulator != NULL) {
            lastActiveDemodulator = activeDemodulator;
        } else {
            lastActiveDemodulator = demod;
        }
    }

    if (activeVisualDemodulator) {
        activeVisualDemodulator->setVisualOutputQueue(NULL);
    }
    if (demod) {
        demod->setVisualOutputQueue(wxGetApp().getAudioVisualQueue());
        activeVisualDemodulator = demod;
    } else {
        DemodulatorInstance *last = getLastActiveDemodulator();
        if (last) {
            last->setVisualOutputQueue(wxGetApp().getAudioVisualQueue());
        }
        activeVisualDemodulator = last;
    }


    activeDemodulator = demod;
}

DemodulatorInstance *DemodulatorMgr::getActiveDemodulator() {
    return activeDemodulator;
}

DemodulatorInstance *DemodulatorMgr::getLastActiveDemodulator() {
    if (std::find(demods.begin(), demods.end(), lastActiveDemodulator) == demods.end()) {
        lastActiveDemodulator = activeDemodulator;
    }

    return lastActiveDemodulator;
}

#pragma once

//WX_GL_CORE_PROFILE 1
//WX_GL_MAJOR_VERSION 3
//WX_GL_MINOR_VERSION 2

#include <thread>

#include "wx/glcanvas.h"
#include "PrimaryGLContext.h"

#include "ThreadQueue.h"
#include "SDRThread.h"
#include "SDRPostThread.h"
#include "AudioThread.h"
#include "DemodulatorMgr.h"

#define NUM_DEMODULATORS 1

class CubicSDR: public wxApp {
public:
    CubicSDR() :
            m_glContext(NULL), t_PostSDR(NULL), t_SDR(NULL), audioVisualQueue(NULL), threadCmdQueueSDR(NULL), iqVisualQueue(NULL), frequency(
                    DEFAULT_FREQ), sdrPostThread(NULL), iqPostDataQueue(NULL), sdrThread(NULL) {

    }

    PrimaryGLContext &GetContext(wxGLCanvas *canvas);

    virtual bool OnInit();
    virtual int OnExit();

    void setFrequency(unsigned int freq);
    int getFrequency();

    DemodulatorThreadOutputQueue* getAudioVisualQueue() {
        return audioVisualQueue;
    }

    SDRThreadIQDataQueue* getIQVisualQueue() {
        return iqVisualQueue;
    }

    DemodulatorInstance *getDemodTest() {
        return demodulatorTest[0];
    }

    DemodulatorMgr &getDemodMgr() {
        return demodMgr;
    }

private:
    PrimaryGLContext *m_glContext;

    DemodulatorMgr demodMgr;

    unsigned int frequency;

    DemodulatorInstance *demodulatorTest[NUM_DEMODULATORS];

    SDRThread *sdrThread;
    SDRPostThread *sdrPostThread;

    SDRThreadCommandQueue* threadCmdQueueSDR;
    SDRThreadIQDataQueue* iqVisualQueue;
    SDRThreadIQDataQueue* iqPostDataQueue;
    DemodulatorThreadOutputQueue* audioVisualQueue;

    std::thread *t_SDR;
    std::thread *t_PostSDR;
};

DECLARE_APP(CubicSDR)

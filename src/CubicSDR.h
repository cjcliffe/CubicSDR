#pragma once

//WX_GL_CORE_PROFILE 1
//WX_GL_MAJOR_VERSION 3
//WX_GL_MINOR_VERSION 2

#include <thread>

#include "wx/glcanvas.h"
#include "PrimaryGLContext.h"

#include "ThreadQueue.h"
#include "SDRThread.h"
#include "AudioThread.h"
#include "DemodulatorMgr.h"

class CubicSDR: public wxApp {
public:
    CubicSDR() {
        m_glContext = NULL;
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
        return demodulatorTest;
    }

    DemodulatorMgr &getDemodMgr() {
        return demodMgr;
    }

private:
    PrimaryGLContext *m_glContext;

    DemodulatorMgr demodMgr;

    unsigned int frequency;

    DemodulatorInstance *demodulatorTest;

    AudioThreadInputQueue *audioInputQueue;
    AudioThread *audioThread;

    SDRThread *sdrThread;
    SDRThreadCommandQueue* threadCmdQueueSDR;
    SDRThreadIQDataQueue* iqVisualQueue;
    DemodulatorThreadOutputQueue* audioVisualQueue;

    std::thread *threadAudio;
    std::thread *threadSDR;
};

DECLARE_APP(CubicSDR)

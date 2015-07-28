#pragma once

//WX_GL_CORE_PROFILE 1
//WX_GL_MAJOR_VERSION 3
//WX_GL_MINOR_VERSION 2

#include <thread>

#include "GLExt.h"
#include "PrimaryGLContext.h"

#include "ThreadQueue.h"
#include "SDRThread.h"
#include "SDRPostThread.h"
#include "AudioThread.h"
#include "DemodulatorMgr.h"
#include "AppConfig.h"
#include "AppFrame.h"

#include <wx/cmdline.h>

#define NUM_DEMODULATORS 1

class CubicSDR: public wxApp {
public:
    CubicSDR();

    PrimaryGLContext &GetContext(wxGLCanvas *canvas);

    virtual bool OnInit();
    virtual int OnExit();

    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

    void setFrequency(long long freq);
    long long getFrequency();

    void setOffset(long long ofs);
    long long getOffset();

    void setDirectSampling(int mode);
    int getDirectSampling();

    void setSwapIQ(bool swapIQ);
    bool getSwapIQ();

    void setSampleRate(long long rate_in);
    long long getSampleRate();

    std::vector<SDRDeviceInfo *> *getDevices();
    void setDevice(int deviceId);
    int getDevice();

    DemodulatorThreadOutputQueue* getAudioVisualQueue();
    DemodulatorThreadInputQueue* getIQVisualQueue();
    DemodulatorMgr &getDemodMgr();

    void bindDemodulator(DemodulatorInstance *demod);
    void removeDemodulator(DemodulatorInstance *demod);

    void setFrequencySnap(int snap);
    int getFrequencySnap();

    AppConfig *getConfig();
    void saveConfig();

    void setPPM(int ppm_in);
    int getPPM();

    void showFrequencyInput();

private:
    AppFrame *appframe;
    AppConfig config;
    PrimaryGLContext *m_glContext;
    std::vector<SDRDeviceInfo *> devs;

    DemodulatorMgr demodMgr;

    long long frequency;
    long long offset;
    int ppm, snap;
    long long sampleRate;
    int directSamplingMode;

    SDRThread *sdrThread;
    SDRPostThread *sdrPostThread;

    SDRThreadCommandQueue* threadCmdQueueSDR;
    SDRThreadIQDataQueue* iqPostDataQueue;
    DemodulatorThreadInputQueue* iqVisualQueue;
    DemodulatorThreadOutputQueue* audioVisualQueue;

    std::thread *t_SDR;
    std::thread *t_PostSDR;
};

static const wxCmdLineEntryDesc commandLineInfo [] =
{
    { wxCMD_LINE_SWITCH, "h", "help", "Command line parameter help", wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_OPTION, "c", "config", "Specify a named configuration to use, i.e. '-c ham'" },
    { wxCMD_LINE_NONE }
};

DECLARE_APP(CubicSDR)

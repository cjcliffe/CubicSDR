#define OPENGL

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"
#include "AppFrame.h"

IMPLEMENT_APP(CubicSDR)

bool CubicSDR::OnInit() {
    if (!wxApp::OnInit())
        return false;

    frequency = DEFAULT_FREQ;

    demodulatorTest = demodMgr.newThread();
    demodulatorTest->getParams().frequency = DEFAULT_FREQ;
    demodulatorTest->run();

    audioVisualQueue = new DemodulatorThreadOutputQueue();
    demodulatorTest->setVisualOutputQueue(audioVisualQueue);

    threadCmdQueueSDR = new SDRThreadCommandQueue;
    sdrThread = new SDRThread(threadCmdQueueSDR);

    sdrPostThread = new SDRPostThread();

    iqPostDataQueue = new SDRThreadIQDataQueue;
    iqVisualQueue = new SDRThreadIQDataQueue;

    sdrThread->setIQDataOutQueue(iqPostDataQueue);
    sdrPostThread->setIQDataInQueue(iqPostDataQueue);
    sdrPostThread->setIQVisualQueue(iqVisualQueue);

    sdrPostThread->bindDemodulator(demodulatorTest);

    t_PostSDR = new std::thread(&SDRPostThread::threadMain, sdrPostThread);
    t_SDR = new std::thread(&SDRThread::threadMain, sdrThread);

    AppFrame *appframe = new AppFrame();

#ifdef __APPLE__
    int main_policy;
    struct sched_param main_param;

    main_policy = SCHED_RR;
    main_param.sched_priority = sched_get_priority_min(SCHED_RR)+2;

    pthread_setschedparam(pthread_self(), main_policy, &main_param);
#endif

    return true;
}

int CubicSDR::OnExit() {
    std::cout << "Terminating SDR thread.." << std::endl;
    sdrThread->terminate();
    t_SDR->join();

    std::cout << "Terminating SDR post-processing thread.." << std::endl;
    sdrPostThread->terminate();
    t_PostSDR->join();

    delete sdrThread;
    delete t_SDR;

    delete sdrPostThread;
    delete t_PostSDR;

    demodMgr.terminateAll();


    delete threadCmdQueueSDR;

    delete iqVisualQueue;
    delete audioVisualQueue;
    delete iqPostDataQueue;

    delete m_glContext;

    return wxApp::OnExit();
}

PrimaryGLContext& CubicSDR::GetContext(wxGLCanvas *canvas) {
    PrimaryGLContext *glContext;
    if (!m_glContext) {
        m_glContext = new PrimaryGLContext(canvas, NULL);
    }
    glContext = m_glContext;

    glContext->SetCurrent(*canvas);

    return *glContext;
}

void CubicSDR::setFrequency(unsigned int freq) {
    frequency = freq;
//    demodulatorTest->getParams().frequency = freq;
    SDRThreadCommand command(SDRThreadCommand::SDR_THREAD_CMD_TUNE);
    command.int_value = freq;
    threadCmdQueueSDR->push(command);
}

int CubicSDR::getFrequency() {
    return frequency;
}

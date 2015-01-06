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
    offset = 0;

    audioVisualQueue = new DemodulatorThreadOutputQueue();
    audioVisualQueue->set_max_num_items(1);

    threadCmdQueueSDR = new SDRThreadCommandQueue;
    sdrThread = new SDRThread(threadCmdQueueSDR);

    sdrPostThread = new SDRPostThread();
    sdrPostThread->setNumVisSamples(2048);

    iqPostDataQueue = new SDRThreadIQDataQueue;
    iqVisualQueue = new DemodulatorThreadInputQueue;
    iqVisualQueue->set_max_num_items(1);

    sdrThread->setIQDataOutQueue(iqPostDataQueue);
    sdrPostThread->setIQDataInQueue(iqPostDataQueue);
    sdrPostThread->setIQVisualQueue(iqVisualQueue);

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

#ifdef __APPLE__
    AudioThread::deviceCleanup();
#endif

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

void CubicSDR::setFrequency(long long freq) {
    if (freq < SRATE/2) {
        freq = SRATE/2;
    }
    frequency = freq;
    SDRThreadCommand command(SDRThreadCommand::SDR_THREAD_CMD_TUNE);
    command.llong_value = freq;
    threadCmdQueueSDR->push(command);
}

long long CubicSDR::getOffset() {
    return offset;
}

void CubicSDR::setOffset(long long ofs) {
    offset = ofs;
    SDRThreadCommand command(SDRThreadCommand::SDR_THREAD_CMD_SET_OFFSET);
    command.llong_value = ofs;
    threadCmdQueueSDR->push(command);
}

long long CubicSDR::getFrequency() {
    return frequency;
}

DemodulatorThreadOutputQueue* CubicSDR::getAudioVisualQueue() {
    return audioVisualQueue;
}

DemodulatorThreadInputQueue* CubicSDR::getIQVisualQueue() {
    return iqVisualQueue;
}

DemodulatorMgr &CubicSDR::getDemodMgr() {
    return demodMgr;
}

void CubicSDR::bindDemodulator(DemodulatorInstance *demod) {
    if (!demod) {
        return;
    }
    sdrPostThread->bindDemodulator(demod);
}

void CubicSDR::removeDemodulator(DemodulatorInstance *demod) {
    if (!demod) {
        return;
    }
    sdrPostThread->removeDemodulator(demod);
}

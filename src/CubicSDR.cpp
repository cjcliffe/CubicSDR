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

    audioInputQueue = new AudioThreadInputQueue;
    audioThread = new AudioThread(audioInputQueue);

    threadAudio = new std::thread(&AudioThread::threadMain, audioThread);

    demodulatorTest = demodMgr.newThread();
    demodulatorTest->getParams().audioInputQueue = audioInputQueue;
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

    threadPostSDR = new std::thread(&SDRPostThread::threadMain, sdrPostThread);
    threadSDR = new std::thread(&SDRThread::threadMain, sdrThread);

    AppFrame *appframe = new AppFrame();

    return true;
}

int CubicSDR::OnExit() {
    std::cout << "Terminating SDR thread.." << std::endl;
    sdrThread->terminate();
    threadSDR->join();

    sdrPostThread->terminate();
    threadPostSDR->join();

    delete sdrThread;
    delete threadSDR;

    delete sdrPostThread;
    delete threadPostSDR;

    demodMgr.terminateAll();

    audioThread->terminate();
    threadAudio->join();

    delete audioThread;
    delete threadAudio;

    delete audioInputQueue;
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

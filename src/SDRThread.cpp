#include "SDRThread.h"

static void rtl_callback(unsigned char *buf, uint32_t len, void *ctx) {
    std::cout << "Got buffer! length: " << len << std::endl;
}

SDRThread::SDRThread(wxApp *app) :
        wxThread(wxTHREAD_DETACHED) {
    dev = NULL;
    this->handler = handler;
}
SDRThread::~SDRThread() {

}

void SDRThread::enumerate_rtl() {

    unsigned int rtl_count = rtlsdr_get_device_count();

    std::cout << "RTL Devices: " << rtl_count << std::endl;

    char manufact[256];
    char product[256];
    char serial[256];

    for (int i = 0; i < rtl_count; i++) {
        std::cout << "Device #" << i << ": " << rtlsdr_get_device_name(i) << std::endl;
        if (rtlsdr_get_device_usb_strings(i, manufact, product, serial) == 0) {
            std::cout << "\tManufacturer: " << manufact << ", Product Name: " << product << ", Serial: " << serial << std::endl;

            rtlsdr_open(&dev, i);

            std::cout << "\t Tuner type: ";
            switch (rtlsdr_get_tuner_type(dev)) {
            case RTLSDR_TUNER_UNKNOWN:
                std::cout << "Unknown";
                break;
            case RTLSDR_TUNER_E4000:
                std::cout << "Elonics E4000";
                break;
            case RTLSDR_TUNER_FC0012:
                std::cout << "Fitipower FC0012";
                break;
            case RTLSDR_TUNER_FC0013:
                std::cout << "Fitipower FC0013";
                break;
            case RTLSDR_TUNER_FC2580:
                std::cout << "Fitipower FC2580";
                break;
            case RTLSDR_TUNER_R820T:
                std::cout << "Rafael Micro R820T";
                break;
            case RTLSDR_TUNER_R828D:
                break;
            }

            std::cout << std::endl;
            /*
             int num_gains = rtlsdr_get_tuner_gains(dev, NULL);

             int *gains = (int *)malloc(sizeof(int) * num_gains);
             rtlsdr_get_tuner_gains(dev, gains);

             std::cout << "\t Valid gains: ";
             for (int g = 0; g < num_gains; g++) {
             if (g > 0) {
             std::cout << ", ";
             }
             std::cout << ((float)gains[g]/10.0f);
             }
             std::cout << std::endl;

             free(gains);
             */

            rtlsdr_close(dev);

        } else {
            std::cout << "\tUnable to access device #" << i << " (in use?)" << std::endl;
        }

    }

}

#define BUF_SIZE 16 * 32 * 512

wxThread::ExitCode SDRThread::Entry() {
    __int8 *buf = (__int8 *) malloc(BUF_SIZE);

    enumerate_rtl();

    rtlsdr_open(&dev, 4);
    rtlsdr_set_sample_rate(dev, 2500000);
    rtlsdr_set_center_freq(dev, 105700000);
    rtlsdr_reset_buffer(dev);

    int n_read;
    int i = 0;
    //    rtlsdr_read_async(dev, rtl_callback, NULL, 0, 0);

    std::cout << "Sampling..";
    while (!TestDestroy()) {

        rtlsdr_read_sync(dev, buf, BUF_SIZE, &n_read);
//        std::cout << "Got buffer! length: " << n_read << std::endl;
        std::cout << ".";

        if (i%50==0) {
            std::cout << std::endl;
        }

        i++;

        //wxQueueEvent(app,new wxThreadEvent(wxEVT_COMMAND_SDRThread_UPDATE));
    }
    std::cout << std::endl << "Done." << std::endl << std::endl;
    //    wxQueueEvent(m_pHandler,new wxThreadEvent(wxEVT_COMMAND_SDRThread_COMPLETED));

    free(buf);
    rtlsdr_close(dev);

    return (wxThread::ExitCode) 0; // success
}

/*
 class MyFrame: public wxFrame {
 public:
 ~MyFrame() {
 // it's better to do any thread cleanup in the OnClose()
 // event handler, rather than in the destructor.
 // This is because the event loop for a top-level window is not
 // active anymore when its destructor is called and if the thread
 // sends events when ending, they won't be processed unless
 // you ended the thread from OnClose.
 // See @ref overview_windowdeletion for more info.
 }

 void DoStartThread();
 void DoPauseThread();
 // a resume routine would be nearly identic to DoPauseThread()
 void DoResumeThread() {
 }
 void OnThreadUpdate(wxThreadEvent&);
 void OnThreadCompletion(wxThreadEvent&);
 void OnClose(wxCloseEvent&);
 protected:
 SDRThread *m_pThread;
 wxCriticalSection m_pThreadCS; // protects the m_pThread pointer
 wxDECLARE_EVENT_TABLE();
 };
 wxBEGIN_EVENT_TABLE(MyFrame, wxFrame) EVT_CLOSE(MyFrame::OnClose)
 EVT_MENU(Minimal_Start, MyFrame::DoStartThread)
 EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_SDRThread_UPDATE, MyFrame::OnThreadUpdate)
 EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_SDRThread_COMPLETED, MyFrame::OnThreadCompletion)
 wxEND_EVENT_TABLE()
 wxDEFINE_EVENT(wxEVT_COMMAND_SDRThread_COMPLETED, wxThreadEvent)wxDEFINE_EVENT
 (wxEVT_COMMAND_SDRThread_UPDATE, wxThreadEvent)void
 MyFrame::DoStartThread() {
 m_pThread = new SDRThread(this);
 if (m_pThread->Run() != wxTHREAD_NO_ERROR) {
 wxLogError
 ("Can't create the thread!");
 delete m_pThread;
 m_pThread = NULL;
 }
 // after the call to wxThread::Run(), the m_pThread pointer is "unsafe":
 // at any moment the thread may cease to exist (because it completes its work).
 // To avoid dangling pointers OnThreadExit() will set m_pThread
 // to NULL when the thread dies.
 }
 wxThread::ExitCode SDRThread::Entry() {
 while (!TestDestroy()) {
 // ... do a bit of work...
 wxQueueEvent(m_pHandler,
 new wxThreadEvent(wxEVT_COMMAND_SDRThread_UPDATE));
 }
 // signal the event handler that this thread is going to be destroyed
 // NOTE: here we assume that using the m_pHandler pointer is safe,
 // (in this case this is assured by the MyFrame destructor)
 wxQueueEvent(m_pHandler,
 new wxThreadEvent(wxEVT_COMMAND_SDRThread_COMPLETED));
 return (wxThread::ExitCode) 0; // success
 }
 SDRThread::~SDRThread() {
 wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);
 // the thread is being destroyed; make sure not to leave dangling pointers around
 m_pHandler->m_pThread = NULL;
 }
 void MyFrame::OnThreadCompletion(wxThreadEvent&) {
 wxMessageOutputDebug().Printf("MYFRAME: SDRThread exited!\n");
 }
 void MyFrame::OnThreadUpdate(wxThreadEvent&) {
 wxMessageOutputDebug().Printf("MYFRAME: SDRThread update...\n");
 }
 void MyFrame::DoPauseThread() {
 // anytime we access the m_pThread pointer we must ensure that it won't
 // be modified in the meanwhile; since only a single thread may be
 // inside a given critical section at a given time, the following code
 // is safe:
 wxCriticalSectionLocker enter(m_pThreadCS);
 if (m_pThread) // does the thread still exist?
 {
 // without a critical section, once reached this point it may happen
 // that the OS scheduler gives control to the SDRThread::Entry() function,
 // which in turn may return (because it completes its work) making
 // invalid the m_pThread pointer
 if (m_pThread->Pause() != wxTHREAD_NO_ERROR)
 wxLogError
 ("Can't pause the thread!");
 }
 }
 void MyFrame::OnClose(wxCloseEvent&) {
 {
 wxCriticalSectionLocker enter(m_pThreadCS);
 if (m_pThread) // does the thread still exist?
 {
 wxMessageOutputDebug().Printf("MYFRAME: deleting thread");
 if (m_pThread->Delete() != wxTHREAD_NO_ERROR)
 wxLogError
 ("Can't delete the thread!");
 }
 } // exit from the critical section to give the thread
 // the possibility to enter its destructor
 // (which is guarded with m_pThreadCS critical section!)
 while (1) {
 { // was the ~SDRThread() function executed?
 wxCriticalSectionLocker enter(m_pThreadCS);
 if (!m_pThread)
 break;
 }
 // wait for thread completion
 wxThread::This()->Sleep(1);
 }
 Destroy();
 }
 */

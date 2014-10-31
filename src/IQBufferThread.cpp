#include "IQBufferThread.h"
#include <cstring>

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDRDefs.h"

IQBufferThread::IQBufferThread(wxApp *app) :
        wxThread(wxTHREAD_DETACHED) {
    this->handler = handler;
}
IQBufferThread::~IQBufferThread() {

}
wxThread::ExitCode IQBufferThread::Entry() {
    signed char *buf = (signed char*) malloc(BUF_SIZE);

    int n_read;
    int i = 0;

//    std::cout << "Sampling..";
    while (!TestDestroy()) {
//
//        iq_buffer.push(new_buffer);
//
//        if (iq_buffer.size() > 100) {
//            for (int i = 0; i < 50; i++) {
//                std::vector<__int8> *old_buffer = iq_buffer.front();
//                iq_buffer.pop();
//                delete old_buffer;
//            }
//            std::cout << "#";
//        }
        this->Sleep(100);
    }
    std::cout << std::endl << "Done." << std::endl << std::endl;

    free(buf);

    return (wxThread::ExitCode) 0;
}


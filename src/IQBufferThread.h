#pragma once

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"
#include "SDRThread.h"
#include <queue>


class IQBufferThread: public wxThread {
public:

    IQBufferThread(wxApp *app);
    ~IQBufferThread();

    void OnEventInput(wxEvent& event)
    {
       std::cout << "event !" << std::endl;
    }
protected:
    virtual ExitCode Entry();
    wxApp *handler;
    std::queue<std::vector<unsigned char> *> iq_buffer;
};

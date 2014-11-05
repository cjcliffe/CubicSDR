#include "SDRThreadQueue.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

SDRThreadQueue::SDRThreadQueue(wxEvtHandler* pParent) :
        m_pParent(pParent) {
}

void SDRThreadQueue::addTask(const SDRThreadTask& task, const SDR_PRIORITY& priority) {
    wxMutexLocker lock(m_MutexQueue);
    m_Tasks.insert(std::make_pair(priority, task));
    m_QueueCount.Post();
}

SDRThreadTask SDRThreadQueue::pop() {
    SDRThreadTask element;
    m_QueueCount.Wait();
    m_MutexQueue.Lock();
    element = (m_Tasks.begin())->second;
    m_Tasks.erase(m_Tasks.begin());
    m_MutexQueue.Unlock();
    return element;
}

void SDRThreadQueue::report(const SDRThreadTask::SDR_COMMAND& cmd, const wxString& sArg, int iArg) {
    wxCommandEvent evt(wxEVT_THREAD, cmd);
    evt.SetString(sArg);
    evt.SetInt(iArg);
    m_pParent->AddPendingEvent(evt);
}

size_t SDRThreadQueue::stackSize() {
    wxMutexLocker lock(m_MutexQueue);
    return m_Tasks.size();
}

wxEvtHandler* SDRThreadQueue::getHandler() {
    return m_pParent;
}

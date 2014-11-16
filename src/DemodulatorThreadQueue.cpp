#include "DemodulatorThreadQueue.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

DemodulatorThreadQueue::DemodulatorThreadQueue(wxEvtHandler* pParent) :
        m_pParent(pParent) {
}

void DemodulatorThreadQueue::addTask(const DemodulatorThreadTask& task, const DEMOD_PRIORITY& priority) {
    wxMutexLocker lock(m_MutexQueue);
    m_Tasks.insert(std::make_pair(priority, task));
    m_QueueCount.Post();
}

DemodulatorThreadTask DemodulatorThreadQueue::pop() {
    DemodulatorThreadTask element;
    m_QueueCount.Wait();
    m_MutexQueue.Lock();
    element = (m_Tasks.begin())->second;
    m_Tasks.erase(m_Tasks.begin());
    m_MutexQueue.Unlock();
    return element;
}

void DemodulatorThreadQueue::report(const DemodulatorThreadTask::DEMOD_THREAD_COMMAND& cmd, const wxString& sArg, int iArg) {
    wxCommandEvent evt(wxEVT_THREAD, cmd);
    evt.SetString(sArg);
    evt.SetInt(iArg);
    m_pParent->AddPendingEvent(evt);
}

size_t DemodulatorThreadQueue::stackSize() {
    wxMutexLocker lock(m_MutexQueue);
    return m_Tasks.size();
}

wxEvtHandler* DemodulatorThreadQueue::getHandler() {
    return m_pParent;
}

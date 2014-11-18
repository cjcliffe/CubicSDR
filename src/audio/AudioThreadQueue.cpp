#include "AudioThreadQueue.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

AudioThreadQueue::AudioThreadQueue(wxEvtHandler* pParent) :
        m_pParent(pParent) {
}

void AudioThreadQueue::addTask(const AudioThreadTask& task, const AUDIO_PRIORITY& priority) {
    wxMutexLocker lock(m_MutexQueue);
    m_Tasks.insert(std::make_pair(priority, task));
    m_QueueCount.Post();
}

AudioThreadTask AudioThreadQueue::pop() {
    AudioThreadTask element;
    m_QueueCount.Wait();
    m_MutexQueue.Lock();
    element = (m_Tasks.begin())->second;
    m_Tasks.erase(m_Tasks.begin());
    m_MutexQueue.Unlock();
    return element;
}

void AudioThreadQueue::report(const AudioThreadTask::AUDIO_THREAD_COMMAND& cmd, const wxString& sArg, int iArg) {
    wxCommandEvent evt(wxEVT_THREAD, cmd);
    evt.SetString(sArg);
    evt.SetInt(iArg);
    m_pParent->AddPendingEvent(evt);
}

size_t AudioThreadQueue::stackSize() {
    wxMutexLocker lock(m_MutexQueue);
    return m_Tasks.size();
}

wxEvtHandler* AudioThreadQueue::getHandler() {
    return m_pParent;
}

#pragma once

#include <map>
#include "AudioThreadTask.h"

#include "wx/event.h"

class AudioThreadQueue {
public:
    enum AUDIO_PRIORITY {
        AUDIO_PRIORITY_HIGHEST, AUDIO_PRIORITY_HIGHER, AUDIO_PRIORITY_NORMAL, AUDIO_PRIORITY_BELOW_NORMAL, AUDIO_PRIORITY_LOW, AUDIO_PRIORITY_IDLE
    };
    AudioThreadQueue(wxEvtHandler* pParent);

    void addTask(const AudioThreadTask& task, const AUDIO_PRIORITY& priority = AUDIO_PRIORITY_NORMAL);
    void report(const AudioThreadTask::AUDIO_THREAD_COMMAND& cmd, const wxString& sArg = wxEmptyString, int iArg = 0);

    AudioThreadTask pop();
    size_t stackSize();

    wxEvtHandler* getHandler();

private:
    wxEvtHandler* m_pParent;
    std::multimap<AUDIO_PRIORITY, AudioThreadTask> m_Tasks;
    wxMutex m_MutexQueue;
    wxSemaphore m_QueueCount;
};

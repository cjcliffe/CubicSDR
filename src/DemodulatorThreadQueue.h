#pragma once

#include <map>
#include "DemodulatorThreadTask.h"

#include "wx/event.h"

class DemodulatorThreadQueue {
public:
    enum DEMOD_PRIORITY {
        DEMOD_PRIORITY_HIGHEST, DEMOD_PRIORITY_HIGHER, DEMOD_PRIORITY_NORMAL, DEMOD_PRIORITY_BELOW_NORMAL, DEMOD_PRIORITY_LOW, DEMOD_PRIORITY_IDLE
    };
    DemodulatorThreadQueue(wxEvtHandler* pParent);

    void addTask(const DemodulatorThreadTask& task, const DEMOD_PRIORITY& priority = DEMOD_PRIORITY_NORMAL);
    void report(const DemodulatorThreadTask::DEMOD_THREAD_COMMAND& cmd, const wxString& sArg = wxEmptyString, int iArg = 0);

    DemodulatorThreadTask pop();
    size_t stackSize();

    wxEvtHandler* getHandler();

private:
    wxEvtHandler* m_pParent;
    std::multimap<DEMOD_PRIORITY, DemodulatorThreadTask> m_Tasks;
    wxMutex m_MutexQueue;
    wxSemaphore m_QueueCount;
};

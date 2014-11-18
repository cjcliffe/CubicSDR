#pragma once

#include <map>
#include <vector>
#include "SDRThreadTask.h"

#include "wx/event.h"

class SDRThreadQueue {
public:
    enum SDR_PRIORITY {
        SDR_PRIORITY_HIGHEST, SDR_PRIORITY_HIGHER, SDR_PRIORITY_NORMAL, SDR_PRIORITY_BELOW_NORMAL, SDR_PRIORITY_LOW, SDR_PRIORITY_IDLE
    };
    SDRThreadQueue(wxEvtHandler* pParent);

    void addTask(const SDRThreadTask& task, const SDR_PRIORITY& priority = SDR_PRIORITY_NORMAL);
    void sendIQData(const SDRThreadTask::SDR_COMMAND& cmd, SDRThreadIQData *data);

    SDRThreadTask pop();
    size_t stackSize();

    wxEvtHandler* getHandler();

private:
    wxEvtHandler* m_pParent;
    std::multimap<SDR_PRIORITY, SDRThreadTask> m_Tasks;
    wxMutex m_MutexQueue;
    wxSemaphore m_QueueCount;
};

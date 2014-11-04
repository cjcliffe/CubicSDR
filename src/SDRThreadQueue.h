#pragma once

#include <map>

class SDRThreadTask {
public:
    enum SDR_COMMAND {
        SDR_THREAD_EXIT = wxID_EXIT, SDR_THREAD_NULL = wxID_HIGHEST + 1, SDR_THREAD_STARTED, SDR_THREAD_PROCESS, SDR_THREAD_ERROR,
    };

    SDRThreadTask() :
            m_cmd(SDR_THREAD_NULL) {
    }
    SDRThreadTask(SDR_COMMAND cmd, const wxString& arg) :
            m_cmd(cmd), m_Arg(arg) {
    }
    SDR_COMMAND m_cmd;
    wxString m_Arg;
};

class SDRThreadQueue {
public:
    enum SDR_PRIORITY {
        SDR_PRIORITY_HIGHEST, SDR_PRIORITY_HIGHER, SDR_PRIORITY_NORMAL, SDR_PRIORITY_BELOW_NORMAL, SDR_PRIORITY_LOW, SDR_PRIORITY_IDLE
    };
    SDRThreadQueue(wxEvtHandler* pParent) :
            m_pParent(pParent) {
    }
    void AddTask(const SDRThreadTask& task, const SDR_PRIORITY& priority = SDR_PRIORITY_NORMAL) {
        wxMutexLocker lock(m_MutexQueue);
        m_Tasks.insert(std::make_pair(priority, task));
        m_QueueCount.Post();
    }
    SDRThreadTask Pop() {
        SDRThreadTask element;
        m_QueueCount.Wait();
        m_MutexQueue.Lock();
        element = (m_Tasks.begin())->second;
        m_Tasks.erase(m_Tasks.begin());
        m_MutexQueue.Unlock();
        return element;
    }
    void Report(const SDRThreadTask::SDR_COMMAND& cmd, const wxString& sArg = wxEmptyString, int iArg = 0) {
        wxCommandEvent evt(wxEVT_THREAD, cmd);
        evt.SetString(sArg);
        evt.SetInt(iArg);
        m_pParent->AddPendingEvent(evt);
    }
    size_t Stacksize() {
        wxMutexLocker lock(m_MutexQueue);
        return m_Tasks.size();
    }

    wxEvtHandler* getHandler() {
        return m_pParent;
    }

private:
    wxEvtHandler* m_pParent;
    std::multimap<SDR_PRIORITY, SDRThreadTask> m_Tasks;
    wxMutex m_MutexQueue;
    wxSemaphore m_QueueCount;
};

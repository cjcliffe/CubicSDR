#pragma once

#include <vector>
#include "wx/defs.h"
#include "wx/string.h"

class DemodulatorThreadTask {
public:
    enum DEMOD_THREAD_COMMAND {
        DEMOD_THREAD_EXIT = wxID_EXIT, DEMOD_THREAD_NULL = wxID_HIGHEST + 1, DEMOD_THREAD_STARTED, DEMOD_THREAD_PROCESS, DEMOD_THREAD_ERROR, DEMOD_THREAD_DATA
    };

    DemodulatorThreadTask() :
            m_cmd(DEMOD_THREAD_NULL) {
    }
    DemodulatorThreadTask(DEMOD_THREAD_COMMAND cmd) :
            m_cmd(cmd) {
    }

    void setData(std::vector<signed char> &data_in);
    std::vector<signed char> &getData();

    DEMOD_THREAD_COMMAND m_cmd;

    std::vector<signed char> data;
};

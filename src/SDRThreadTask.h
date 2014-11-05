#pragma once

#include "wx/defs.h"
#include "wx/string.h"

class SDRThreadTask {
public:
    enum SDR_COMMAND {
        SDR_THREAD_EXIT = wxID_EXIT, SDR_THREAD_NULL = wxID_HIGHEST + 1, SDR_THREAD_STARTED, SDR_THREAD_PROCESS, SDR_THREAD_ERROR, SDR_THREAD_TUNING
    };

    SDRThreadTask() :
            m_cmd(SDR_THREAD_NULL) {
    }
    SDRThreadTask(SDR_COMMAND cmd) :
            m_cmd(cmd) {
    }

    void setUInt(unsigned int i);
    unsigned int getUInt();

    SDR_COMMAND m_cmd;
    unsigned int arg_int;
};

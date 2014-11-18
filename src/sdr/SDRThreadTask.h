#pragma once

#include <vector>
#include "wx/defs.h"
#include "wx/string.h"
#include "wx/object.h"

class SDRThreadIQData: public wxObject {
public:
    unsigned int frequency;
    unsigned int bandwidth;
    std::vector<signed char> data;

    SDRThreadIQData(unsigned int bandwidth, unsigned int frequency, std::vector<signed char> data) :
            data(data), frequency(frequency), bandwidth(bandwidth) {

    }

    ~SDRThreadIQData() {

    }
};

class SDRThreadTask {
public:
    enum SDR_COMMAND {
        SDR_THREAD_EXIT = wxID_EXIT,
        SDR_THREAD_NULL = wxID_HIGHEST + 1,
        SDR_THREAD_STARTED,
        SDR_THREAD_PROCESS,
        SDR_THREAD_ERROR,
        SDR_THREAD_TUNING,
        SDR_THREAD_DATA
    };

    SDRThreadTask() :
            m_cmd(SDR_THREAD_NULL), arg_int(0) {
    }
    SDRThreadTask(SDR_COMMAND cmd) :
            arg_int(0), m_cmd(cmd) {
    }

    void setUInt(unsigned int i);
    unsigned int getUInt();

    SDR_COMMAND m_cmd;
    unsigned int arg_int;
};

#pragma once

#include <vector>

#include "wx/defs.h"
#include "wx/string.h"

class AudioThreadTask {
public:
    enum AUDIO_THREAD_COMMAND {
        AUDIO_THREAD_EXIT = wxID_EXIT, AUDIO_THREAD_NULL = wxID_HIGHEST + 1, AUDIO_THREAD_STARTED, AUDIO_THREAD_PROCESS, AUDIO_THREAD_ERROR, AUDIO_THREAD_DATA
    };

    AudioThreadTask() :
            m_cmd(AUDIO_THREAD_NULL) {
    }
    AudioThreadTask(AUDIO_THREAD_COMMAND cmd) :
            m_cmd(cmd) {
    }

    void setData(std::vector<float> &data_in);
    std::vector<float> &getData();


    std::vector<float> data;

    AUDIO_THREAD_COMMAND m_cmd;
};

#pragma once

#include <queue>
#include <vector>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "AudioThreadQueue.h"

#include "ao/ao.h"

class AudioThread: public wxThread {
public:
    std::queue<std::vector<float> > audio_queue;
    unsigned int audio_queue_ptr;

    AudioThread(AudioThreadQueue* pQueue, int id = 0);
    ~AudioThread();

protected:
    virtual ExitCode Entry();
    AudioThreadQueue* m_pQueue;
    int m_ID;

	ao_device *device;
	ao_sample_format format;
};

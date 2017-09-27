// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "AudioThread.h"

class AudioSinkThread : public IOThread {

public:

    AudioSinkThread();
    virtual ~AudioSinkThread();

    virtual void run();
    virtual void terminate();

    virtual void sink(AudioThreadInputPtr input) = 0;
    virtual void inputChanged(AudioThreadInput oldProps, AudioThreadInputPtr newProps) = 0;

    virtual void setSinkName(std::string sinkName_in);
    virtual std::string getSinkName();

protected:
    std::recursive_mutex m_mutex;
    AudioThreadInputQueuePtr inputQueuePtr;
    std::string sinkName;

};

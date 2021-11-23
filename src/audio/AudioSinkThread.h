// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "AudioThread.h"

class AudioSinkThread : public IOThread {

public:

    AudioSinkThread();
    ~AudioSinkThread() override;

	void run() override;
	void terminate() override;

    virtual void sink(AudioThreadInputPtr input) = 0;
    virtual void inputChanged(AudioThreadInput oldProps, AudioThreadInputPtr newProps) = 0;

protected:
    std::recursive_mutex m_mutex;
    AudioThreadInputQueuePtr inputQueuePtr;

};

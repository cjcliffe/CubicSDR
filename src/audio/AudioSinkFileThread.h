// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "AudioSinkThread.h"
#include "AudioFile.h"

class AudioSinkFileThread : public AudioSinkThread {

public:
    AudioSinkFileThread();
    ~AudioSinkFileThread();

    void sink(AudioThreadInputPtr input);
    void inputChanged(AudioThreadInput oldProps, AudioThreadInputPtr newProps);

    void setOutput(AudioFile *output);

protected:
    AudioFile *outputFileHandler = nullptr;

};


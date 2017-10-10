// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioSinkFileThread.h"

AudioSinkFileThread::AudioSinkFileThread() : AudioSinkThread() {

}

AudioSinkFileThread::~AudioSinkFileThread() {
    if (audioFileHandler != nullptr) {
        audioFileHandler->closeFile();
    }
}

void AudioSinkFileThread::sink(AudioThreadInputPtr input) {
    if (!audioFileHandler) {
        return;
    }
    // forward to output file handler
    audioFileHandler->writeToFile(input);
}

void AudioSinkFileThread::inputChanged(AudioThreadInput oldProps, AudioThreadInputPtr newProps) {
    // close, set new parameters, adjust file name sequence and re-open?
    if (!audioFileHandler) {
        return;
    }

    audioFileHandler->closeFile();
}

void AudioSinkFileThread::setAudioFileHandler(AudioFile * output) {
    audioFileHandler = output;
}

// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioSinkFileThread.h"

AudioSinkFileThread::AudioSinkFileThread() : AudioSinkThread() {

}

AudioSinkFileThread::~AudioSinkFileThread() {
    if (outputFileHandler != nullptr) {
        outputFileHandler->closeFile();
    }
}

void AudioSinkFileThread::sink(AudioThreadInputPtr input) {
    if (!outputFileHandler) {
        return;
    }
    // forward to output file handler
    outputFileHandler->writeToFile(input);
}

void AudioSinkFileThread::inputChanged(AudioThreadInput oldProps, AudioThreadInputPtr newProps) {
    // close, set new parameters, adjust file name sequence and re-open?
    if (!outputFileHandler) {
        return;
    }
}

void AudioSinkFileThread::setOutput(AudioFile * output) {
    outputFileHandler = output;
    outputFileHandler->setOutputFileName(sinkName);
}

// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioFile.h"
#include "CubicSDR.h"

AudioFile::AudioFile() {

}

AudioFile::~AudioFile() {

}

void AudioFile::setOutputFileName(std::string filename) {
    filenameBase = filename;
}

std::string AudioFile::getOutputFileName() {
    std::string recPath = wxGetApp().getConfig()->getRecordingPath();

    // TODO: Handle invalid chars, etc..
    std::string filenameBaseSafe = filenameBase;

    return recPath + filePathSeparator + filenameBaseSafe + getSuffix() + "." + getExtension();
}


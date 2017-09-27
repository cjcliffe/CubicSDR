// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioFileWAV.h"
// #include "WavFileFormatHandlerStuff.h"

AudioFileWAV::AudioFileWAV() : AudioFile() {
}

AudioFileWAV::~AudioFileWAV() {
}


std::string AudioFileWAV::getExtension()
{
    return "wav";
}

std::string AudioFileWAV::getSuffix()
{
    return suffix;
}

bool AudioFileWAV::writeToFile(AudioThreadInputPtr input)
{
    if (!outputFileStream.is_open()) {
        suffix = "";
        std::string ofName = getOutputFileName();
                
        // Check if file exists, sequence the suffix?

        outputFileStream.open(ofName.c_str(), std::ios::out | std::ios::binary);
    }

    // write input data to wav file

    return true;
}

bool AudioFileWAV::closeFile()
{
    if (outputFileStream.is_open()) {
        outputFileStream.close();
    }

    return true;
}

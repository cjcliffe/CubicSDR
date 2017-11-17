// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "AudioFile.h"

#include <fstream>

class AudioFileWAV : public AudioFile {

public:
    AudioFileWAV();
    ~AudioFileWAV();

    std::string getExtension();

    bool writeToFile(AudioThreadInputPtr input);
    bool closeFile();

protected:
    std::ofstream outputFileStream;
    size_t dataChunkPos;
};
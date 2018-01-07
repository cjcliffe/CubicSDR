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
	long long currentFileSize = 0;
	int currentSequenceNumber = 0;

private:

	size_t getMaxWritableNumberOfSamples(AudioThreadInputPtr input);

	void writeHeaderToFileStream(AudioThreadInputPtr input);

	//write [startInputPosition; endInputPosition[ samples from input into the file.
	void writePayloadToFileStream(AudioThreadInputPtr input, size_t startInputPosition, size_t endInputPosition);
};
// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "AudioFile.h"

#include <fstream>

class AudioFileWAV : public AudioFile {

public:
    AudioFileWAV();
    ~AudioFileWAV();

	//override of the base method to generate multi-part 
	//WAV to overcome the WAV format size limit.
	virtual std::string getOutputFileName();

    virtual std::string getExtension();

    virtual bool writeToFile(AudioThreadInputPtr input);
    virtual bool closeFile();

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
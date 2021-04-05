// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "AudioFile.h"

#include <fstream>

class AudioFileWAV : public AudioFile {

public:
    AudioFileWAV();
    ~AudioFileWAV() override;

	//override to manage name change with multi-part WAV. 
	void setOutputFileName(std::string filename) override;

	//override of the base method to generate multi-part 
	//WAV to overcome the WAV format size limit.
	std::string getOutputFileName() override;

    std::string getExtension() override;

    bool writeToFile(AudioThreadInputPtr input) override;
    bool closeFile() override;

protected:
    std::ofstream outputFileStream;
    size_t dataChunkPos;
	long long currentFileSize = 0;
	int currentSequenceNumber = 0;

private:

	size_t getMaxWritableNumberOfSamples(const AudioThreadInputPtr& input) const;

	void writeHeaderToFileStream(const AudioThreadInputPtr& input);

	//write [startInputPosition; endInputPosition[ samples from input into the file.
	void writePayloadToFileStream(const AudioThreadInputPtr& input, size_t startInputPosition, size_t endInputPosition);
};
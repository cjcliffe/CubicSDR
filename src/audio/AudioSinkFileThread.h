// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "AudioSinkThread.h"
#include "AudioFile.h"
#include "Timer.h"

class AudioSinkFileThread : public AudioSinkThread {

public:
    AudioSinkFileThread();
    ~AudioSinkFileThread() override;

	enum SquelchOption {
		SQUELCH_RECORD_SILENCE = 0, // default value, record as a user would hear it.
		SQUELCH_SKIP_SILENCE = 1,  // skip below-squelch level. 
		SQUELCH_RECORD_ALWAYS = 2, // record irrespective of the squelch level.
		SQUELCH_RECORD_MAX
	};

    void sink(AudioThreadInputPtr input) override;
    void inputChanged(AudioThreadInput oldProps, AudioThreadInputPtr newProps) override;

    void setAudioFileHandler(AudioFile *output);

	void setAudioFileNameBase(const std::string& baseName);

	//Squelch 
	void setSquelchOption(int squelchOptEnumValue);

	// Time limit
	void setFileTimeLimit(int nbSeconds);

protected:

	std::string fileNameBase;

    AudioFile *audioFileHandler = nullptr;

	SquelchOption squelchOption = SQUELCH_RECORD_SILENCE;
	int fileTimeLimit = 0;

	int fileTimeDurationSeconds = -1;

	Timer durationMeasurement;

};


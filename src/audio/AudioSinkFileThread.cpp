// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioSinkFileThread.h"
#include <ctime>

#define HEARTBEAT_CHECK_PERIOD_MICROS (50 * 1000) 

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

	//by default, always write something
	bool isSomethingToWrite = true;

	if (input->is_squelch_active) {

		if (squelchOption == SQUELCH_RECORD_SILENCE) {
		
			//patch with "silence"
			input->data.assign(input->data.size(), 0.0f);
			input->peak = 0.0f;
		}
		else if (squelchOption == SQUELCH_SKIP_SILENCE) {
			isSomethingToWrite = false;
		}
	}

	//else, nothing to do record as if squelch was not enabled.
	
	if (!isSomethingToWrite) {
		return;
	}

	if (fileTimeLimit > 0) {
		durationMeasurement.update();

		//duration exeeded, close this file and create another
		//with "now" as timestamp.
		if (durationMeasurement.getSeconds() > fileTimeLimit) {
			
			audioFileHandler->closeFile();

			//initialize the filename of the AudioFile with the current time
			time_t t = std::time(nullptr);
			tm ltm = *std::localtime(&t);

			//  GCC 5+
			//    fileName << "_" << std::put_time(&ltm, "%d-%m-%Y_%H-%M-%S");

			char timeStr[512];
			//International format: Year.Month.Day, also lexicographically sortable
			strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H-%M-%S", &ltm);

			audioFileHandler->setOutputFileName(fileNameBase + std::string("_") + timeStr);

			//reset duration counter
			durationMeasurement.start();
			//the following writeToFile will take care of creating another file.
		}
	}

    // forward to output file handler
    audioFileHandler->writeToFile(input);
}

void AudioSinkFileThread::inputChanged(AudioThreadInput /* oldProps */, AudioThreadInputPtr /* newProps */) {
    // close, set new parameters, adjust file name sequence and re-open?
    if (!audioFileHandler) {
        return;
    }

    audioFileHandler->closeFile();

	//reset duration counter
	durationMeasurement.start();
}

void AudioSinkFileThread::setAudioFileNameBase(const std::string& baseName) {

	fileNameBase = baseName;
}

void AudioSinkFileThread::setAudioFileHandler(AudioFile * output) {
    audioFileHandler = output;
	
	//initialize the filename of the AudioFile with the current time
	time_t t = std::time(nullptr);
	tm ltm = *std::localtime(&t);

	//  GCC 5+
	//    fileName << "_" << std::put_time(&ltm, "%d-%m-%Y_%H-%M-%S");

	char timeStr[512];
	//International format: Year.Month.Day, also lexicographically sortable
	strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H-%M-%S", &ltm);

	audioFileHandler->setOutputFileName(fileNameBase + std::string("_") + timeStr);

	// reset Timer
	durationMeasurement.start();
}

void AudioSinkFileThread::setSquelchOption(int squelchOptEnumValue) {

	if (squelchOptEnumValue == AudioSinkFileThread::SQUELCH_RECORD_SILENCE) {
		squelchOption = AudioSinkFileThread::SQUELCH_RECORD_SILENCE;
	}
	else if (squelchOptEnumValue == AudioSinkFileThread::SQUELCH_SKIP_SILENCE) {
		squelchOption = AudioSinkFileThread::SQUELCH_SKIP_SILENCE;
	}
	else if (squelchOptEnumValue == AudioSinkFileThread::SQUELCH_RECORD_ALWAYS) {
		squelchOption = AudioSinkFileThread::SQUELCH_RECORD_ALWAYS;
	}
	else {
		squelchOption = AudioSinkFileThread::SQUELCH_RECORD_SILENCE;
	}
}

// Time limit
void AudioSinkFileThread::setFileTimeLimit(int nbSeconds) {

	if (nbSeconds > 0) {
		fileTimeLimit = nbSeconds;
	}
	else {
		fileTimeLimit = 0;
	}
}

// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioFile.h"
#include "CubicSDR.h"
#include <sstream>

AudioFile::AudioFile() = default;

AudioFile::~AudioFile() = default;

void AudioFile::setOutputFileName(std::string filename) {
    filenameBase = filename;
}

std::string AudioFile::getOutputFileName() {

    std::string recPath = wxGetApp().getConfig()->getRecordingPath();

    // Strip any invalid characters from the name
    std::string stripChars("<>:\"/\\|?*");
    std::string filenameBaseSafe = filenameBase;

    for (size_t i = 0, iMax = filenameBaseSafe.length(); i < iMax; i++) {
        if (stripChars.find(filenameBaseSafe[i]) != std::string::npos) {
            filenameBaseSafe.replace(i,1,"_");
        }
    }
    
    // Create output file name
	std::stringstream outputFileName;
	outputFileName << recPath << filePathSeparator << filenameBaseSafe;

    int idx = 0;

    // If the file exists; then find the next non-existing file in sequence.
	std::string fileNameCandidate = outputFileName.str();

    while (FILE *file = fopen((fileNameCandidate + "." + getExtension()).c_str(), "r")) {
        fclose(file);
		fileNameCandidate = outputFileName.str() +  "-" + std::to_string(++idx);
    }

    return fileNameCandidate + "." + getExtension();
}


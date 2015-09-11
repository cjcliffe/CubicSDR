#include "FFTDataDistributor.h"

FFTDataDistributor::FFTDataDistributor() : linesPerSecond(DEFAULT_WATERFALL_LPS), lineRateAccum(0.0), fftSize(DEFAULT_FFT_SIZE) {
}

void FFTDataDistributor::setFFTSize(int fftSize) {
	this->fftSize = fftSize;
}

void FFTDataDistributor::setLinesPerSecond(int lines) {
	this->linesPerSecond = lines;
}

int FFTDataDistributor::getLinesPerSecond() {
	return this->linesPerSecond;
}

void FFTDataDistributor::process() {
	while (!input->empty()) {
		if (!isAnyOutputEmpty()) {
			return;
		}
		DemodulatorThreadIQData *inp;
		input->pop(inp);

		if (inp) {
			if (inputBuffer.sampleRate != inp->sampleRate || inputBuffer.frequency != inp->frequency) {
				inputBuffer.sampleRate = inp->sampleRate;
				inputBuffer.frequency = inp->frequency;
				inputBuffer.data.assign(inp->data.begin(), inp->data.end());
			} else {
				inputBuffer.data.insert(inputBuffer.data.end(), inp->data.begin(), inp->data.end());
			}
			inp->decRefCount();
		} else {
			continue;
		}

		// number of seconds contained in input
		double inputTime = (double)inputBuffer.data.size() / (double)inputBuffer.sampleRate;
		// number of lines in input
		double inputLines = (double)inputBuffer.data.size()/(double)fftSize;

		// ratio required to achieve the desired rate
		double lineRateStep = ((double)linesPerSecond * inputTime)/(double)inputLines;

		if (inputBuffer.data.size() >= fftSize) {
			int numProcessed = 0;

			if (lineRateAccum + (lineRateStep * ((double)inputBuffer.data.size()/(double)fftSize)) < 1.0) {
				// move along, nothing to see here..
				lineRateAccum += (lineRateStep * ((double)inputBuffer.data.size()/(double)fftSize));
				numProcessed = inputBuffer.data.size();
			} else {
				for (int i = 0, iMax = inputBuffer.data.size(); i < iMax; i += fftSize) {
					if ((i + fftSize) > iMax) {
						break;
					}
					lineRateAccum += lineRateStep;

					if (lineRateAccum >= 1.0) {
						DemodulatorThreadIQData *outp = outputBuffers.getBuffer();
						outp->frequency = inputBuffer.frequency;
						outp->sampleRate = inputBuffer.sampleRate;
						outp->data.assign(inputBuffer.data.begin()+i,inputBuffer.data.begin()+i+fftSize);
						distribute(outp);

						while (lineRateAccum >= 1.0) {
							lineRateAccum -= 1.0;
						}
					}

					numProcessed += fftSize;
				}
			}
			if (numProcessed) {
				inputBuffer.data.erase(inputBuffer.data.begin(), inputBuffer.data.begin() + numProcessed);
			}
		}
	}
}

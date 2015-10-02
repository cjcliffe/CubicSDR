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
                
                bufferMax = inp->sampleRate / 4;
//                std::cout << "Buffer Max: " << bufferMax << std::endl;
                bufferOffset = 0;
                
				inputBuffer.sampleRate = inp->sampleRate;
				inputBuffer.frequency = inp->frequency;
                inputBuffer.data.resize(bufferMax);
			}
            if ((bufferOffset + bufferedItems + inp->data.size()) > bufferMax) {
                memmove(&inputBuffer.data[0], &inputBuffer.data[bufferOffset], bufferedItems*sizeof(liquid_float_complex));
                bufferOffset = 0;
            } else {
                memcpy(&inputBuffer.data[bufferOffset+bufferedItems],&inp->data[0],inp->data.size()*sizeof(liquid_float_complex));
                bufferedItems += inp->data.size();
            }
			inp->decRefCount();
		} else {
			continue;
		}

		// number of seconds contained in input
		double inputTime = (double)bufferedItems / (double)inputBuffer.sampleRate;
		// number of lines in input
		double inputLines = (double)bufferedItems / (double)fftSize;

		// ratio required to achieve the desired rate
		double lineRateStep = ((double)linesPerSecond * inputTime)/(double)inputLines;

		if (bufferedItems >= fftSize) {
			int numProcessed = 0;

			if (lineRateAccum + (lineRateStep * ((double)bufferedItems/(double)fftSize)) < 1.0) {
				// move along, nothing to see here..
				lineRateAccum += (lineRateStep * ((double)bufferedItems/(double)fftSize));
				numProcessed = bufferedItems;
			} else {
				for (int i = 0, iMax = bufferedItems; i < iMax; i += fftSize) {
					if ((i + fftSize) > iMax) {
						break;
					}
					lineRateAccum += lineRateStep;

					if (lineRateAccum >= 1.0) {
						DemodulatorThreadIQData *outp = outputBuffers.getBuffer();
						outp->frequency = inputBuffer.frequency;
						outp->sampleRate = inputBuffer.sampleRate;
						outp->data.assign(inputBuffer.data.begin()+bufferOffset+i,inputBuffer.data.begin()+bufferOffset+i+fftSize);
						distribute(outp);

						while (lineRateAccum >= 1.0) {
							lineRateAccum -= 1.0;
						}
					}

					numProcessed += fftSize;
				}
			}
			if (numProcessed) {
                bufferedItems -= numProcessed;
                bufferOffset += numProcessed;
            }
            if (bufferedItems <= 0) {
                bufferedItems = 0;
                bufferOffset = 0;
            }
		}
	}
}

// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "FFTDataDistributor.h"
#include <algorithm>
#include <ThreadBlockingQueue.h>

//50 ms
#define HEARTBEAT_CHECK_PERIOD_MICROS (50 * 1000) 

FFTDataDistributor::FFTDataDistributor() : outputBuffers("FFTDataDistributorBuffers"), fftSize(DEFAULT_FFT_SIZE), linesPerSecond(DEFAULT_WATERFALL_LPS), lineRateAccum(0.0) {

}

void FFTDataDistributor::setFFTSize(unsigned int size) {
	 
    fftSize.store(size);
}

void FFTDataDistributor::setLinesPerSecond(unsigned int lines) {
	this->linesPerSecond = lines;
}

unsigned int FFTDataDistributor::getLinesPerSecond() const {
	return this->linesPerSecond;
}

void FFTDataDistributor::process() {

	while (!input->empty()) {
		if (!isAnyOutputEmpty()) {
			return;
		}
		DemodulatorThreadIQDataPtr inp;

        if (!input->pop(inp, HEARTBEAT_CHECK_PERIOD_MICROS)) {
            continue;
        }

		if (inp) {
            //Settings have changed, set new values and dump all previous samples stored in inputBuffer: 
			if (inputBuffer.sampleRate != inp->sampleRate || inputBuffer.frequency != inp->frequency) {

                //bufferMax must be at least fftSize (+ margin), else the waterfall get frozen, because no longer updated.
                bufferMax = std::max((size_t)(inp->sampleRate * FFT_DISTRIBUTOR_BUFFER_IN_SECONDS), (size_t)(1.2 * fftSize.load()));

//                std::cout << "Buffer Max: " << bufferMax << std::endl;
                bufferOffset = 0;
                bufferedItems = 0;
				inputBuffer.sampleRate = inp->sampleRate;
				inputBuffer.frequency = inp->frequency;
                inputBuffer.data.resize(bufferMax);
			}

            //adjust (bufferMax ; inputBuffer.data) in case of FFT size change only.
            if (bufferMax < (size_t)(1.2 * fftSize.load())) {
                bufferMax = (size_t)(1.2 * fftSize.load());
                inputBuffer.data.resize(bufferMax);
            }

            size_t nbSamplesToAdd = inp->data.size();

            //No room left in inputBuffer.data to accept inp->data.size() more samples.
            //so make room by sliding left of bufferOffset, which is fine because 
            //those samples has already been processed.
            if ((bufferOffset + bufferedItems + inp->data.size()) > bufferMax) {
                memmove(&inputBuffer.data[0], &inputBuffer.data[bufferOffset], bufferedItems*sizeof(liquid_float_complex));
                bufferOffset = 0;
                //if there are too much samples, we may even overflow !
                //as a fallback strategy, drop the last incomming new samples not fitting in inputBuffer.data.
                if (bufferedItems + inp->data.size() > bufferMax) {
                    //clamp nbSamplesToAdd
                    nbSamplesToAdd = bufferMax - bufferedItems;
                    std::cout << "FFTDataDistributor::process() incoming samples overflow, dropping the last " << (inp->data.size() - nbSamplesToAdd) << " input samples..." << std::endl;
                }
            }
            
            //store nbSamplesToAdd incoming samples. 
            memcpy(&inputBuffer.data[bufferOffset+bufferedItems],&inp->data[0], nbSamplesToAdd *sizeof(liquid_float_complex));
            bufferedItems += nbSamplesToAdd;
            //
		
		} else {
            //empty inp, wait for another.
			continue;
		}

		// number of seconds contained in input
		double inputTime = (double)bufferedItems / (double)inputBuffer.sampleRate;
		// number of lines in input
		double inputLines = (double)bufferedItems / (double)fftSize;

		// ratio required to achieve the desired rate:
        // it means we can achieive 'lineRateStep' times the target linesPerSecond.
        // < 1 means we cannot reach it by lack of samples.
		double lineRateStep = ((double)linesPerSecond * inputTime)/(double)inputLines;

        //we have enough samples to FFT at least one 'line' of 'fftSize' frequencies for display:
		if (bufferedItems >= fftSize) {
			size_t numProcessed = 0;
			if (lineRateAccum + (lineRateStep * ((double)bufferedItems/(double)fftSize)) < 1.0) {
				// move along, nothing to see here..
				lineRateAccum += (lineRateStep * ((double)bufferedItems/(double)fftSize));
				numProcessed = bufferedItems;
			} else {
				for (size_t i = 0, iMax = bufferedItems; i < iMax; i += fftSize) {
					if ((i + fftSize) > iMax) {
						break;
					}
					lineRateAccum += lineRateStep;

					if (lineRateAccum >= 1.0) {
                        //each i represents a FFT computation
                        DemodulatorThreadIQDataPtr outp = outputBuffers.getBuffer();

						outp->frequency = inputBuffer.frequency;
						outp->sampleRate = inputBuffer.sampleRate;
						outp->data.assign(inputBuffer.data.begin()+bufferOffset+i,
                                          inputBuffer.data.begin()+bufferOffset+i+ fftSize);
                        //authorize distribute with losses
						distribute(outp, NON_BLOCKING_TIMEOUT);

						while (lineRateAccum >= 1.0) {
							lineRateAccum -= 1.0;
						}
					}

					numProcessed += fftSize;
				} //end for 
			}
            //advance bufferOffset read pointer, 
            //reduce size of bufferedItems.
			if (numProcessed) {
                bufferedItems -= numProcessed;
                bufferOffset += numProcessed;
            }
            //clamp to zero the number of remaining items.
            if (bufferedItems <= 0) {
                bufferedItems = 0;
                bufferOffset = 0;
            }
		} //end if bufferedItems >= fftSize
	} //en while
}

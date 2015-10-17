#include "SDRPostThread.h"
#include "CubicSDRDefs.h"
#include "CubicSDR.h"

#include <vector>
#include <deque>

SDRPostThread::SDRPostThread() : IOThread() {
    iqDataInQueue = NULL;
    iqDataOutQueue = NULL;
    iqVisualQueue = NULL;

    swapIQ.store(false);
    numChannels = 0;
    channelizer = NULL;
    
    sampleRate = 0;
    nRunDemods = 0;
    
    doRefresh.store(false);
    dcFilter = iirfilt_crcf_create_dc_blocker(0.0005);
}

SDRPostThread::~SDRPostThread() {
}

void SDRPostThread::bindDemodulator(DemodulatorInstance *demod) {
    busy_demod.lock();
    demodulators.push_back(demod);
    doRefresh.store(true);
    busy_demod.unlock();
}

void SDRPostThread::removeDemodulator(DemodulatorInstance *demod) {
    if (!demod) {
        return;
    }

    busy_demod.lock();
    std::vector<DemodulatorInstance *>::iterator i = std::find(demodulators.begin(), demodulators.end(), demod);
    
    if (i != demodulators.end()) {
        demodulators.erase(i);
        doRefresh.store(true);
    }
    busy_demod.unlock();
}

void SDRPostThread::setSwapIQ(bool swapIQ) {
    this->swapIQ.store(swapIQ);
}

bool SDRPostThread::getSwapIQ() {
    return this->swapIQ.load();
}

void SDRPostThread::initPFBChannelizer() {
//    std::cout << "Initializing post-process FIR polyphase filterbank channelizer with " << numChannels << " channels." << std::endl;
    if (channelizer) {
        firpfbch_crcf_destroy(channelizer);
    }
    channelizer = firpfbch_crcf_create_kaiser(LIQUID_ANALYZER, numChannels, 4, 60);
    
    chanBw = (sampleRate / numChannels);
    
    chanCenters.resize(numChannels);
    demodChannelActive.resize(numChannels);
    
//    std::cout << "Channel bandwidth spacing: " << (chanBw) << std::endl;
}

void SDRPostThread::updateActiveDemodulators() {
    // In range?
    std::vector<DemodulatorInstance *>::iterator demod_i;
    
    nRunDemods = 0;
    
    for (demod_i = demodulators.begin(); demod_i != demodulators.end(); demod_i++) {
        DemodulatorInstance *demod = *demod_i;
        DemodulatorThreadInputQueue *demodQueue = demod->getIQInputDataPipe();
        
        // not in range?
        if (abs(frequency - demod->getFrequency()) > (sampleRate / 2)) {
            // deactivate if active
            if (demod->isActive() && !demod->isFollow() && !demod->isTracking()) {
                demod->setActive(false);
                DemodulatorThreadIQData *dummyDataOut = new DemodulatorThreadIQData;
                dummyDataOut->frequency = frequency;
                dummyDataOut->sampleRate = sampleRate;
                demodQueue->push(dummyDataOut);
            }
            
            // follow if follow mode
            if (demod->isFollow() && wxGetApp().getFrequency() != demod->getFrequency()) {
                wxGetApp().setFrequency(demod->getFrequency());
                demod->setFollow(false);
            }
        } else if (!demod->isActive()) { // in range, activate if not activated
            demod->setActive(true);
            if (wxGetApp().getDemodMgr().getLastActiveDemodulator() == NULL) {
                wxGetApp().getDemodMgr().setActiveDemodulator(demod);
            }
        }
        
        if (!demod->isActive()) {
            continue;
        }
        
        // Add to the current run
        if (nRunDemods == runDemods.size()) {
            runDemods.push_back(demod);
            demodChannel.push_back(-1);
        } else {
            runDemods[nRunDemods] = demod;
            demodChannel[nRunDemods] = -1;
        }
        nRunDemods++;
    }
}

void SDRPostThread::updateChannels() {
    // calculate channel center frequencies, todo: cache
    for (int i = 0; i < numChannels/2; i++) {
        int ofs = ((chanBw) * i);
        chanCenters[i] = frequency + ofs;
        chanCenters[i+(numChannels/2)] = frequency - (sampleRate/2) + ofs;
    }
}

void SDRPostThread::run() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();  // ID of this thread
    int priority = sched_get_priority_max( SCHED_FIFO);
    sched_param prio = {priority}; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_FIFO, &prio);
#endif

    std::cout << "SDR post-processing thread started.." << std::endl;

    iqDataInQueue = (SDRThreadIQDataQueue*)getInputQueue("IQDataInput");
    iqDataOutQueue = (DemodulatorThreadInputQueue*)getOutputQueue("IQDataOutput");
    iqVisualQueue = (DemodulatorThreadInputQueue*)getOutputQueue("IQVisualDataOutput");
    iqActiveDemodVisualQueue = (DemodulatorThreadInputQueue*)getOutputQueue("IQActiveDemodVisualDataOutput");

    iqDataInQueue->set_max_num_items(0);

    std::vector<liquid_float_complex> dcBuf;
    
    while (!terminated) {
        SDRThreadIQData *data_in;
        
        iqDataInQueue->pop(data_in);
        //        std::lock_guard < std::mutex > lock(data_in->m_mutex);
        
        if (data_in && data_in->data.size() && data_in->numChannels) {
            if (numChannels != data_in->numChannels || sampleRate != data_in->sampleRate) {
                numChannels = data_in->numChannels;
                sampleRate = data_in->sampleRate;
                initPFBChannelizer();
                doRefresh.store(true);
            }
            
            int dataSize = data_in->data.size();
            int outSize = data_in->data.size();

            if (outSize > dataOut.capacity()) {
                dataOut.reserve(outSize);
            }
            if (outSize != dataOut.size()) {
                dataOut.resize(outSize);
            }

            //            if (swapIQ) {
            //                for (int i = 0; i < dataSize; i++) {
            //                    fpData[i] = _lut_swap[*((uint16_t*)&data_in->data[2*i])];
            //                }
            //            } else {
            //                for (int i = 0; i < dataSize; i++) {
            //                    fpData[i] = _lut[*((uint16_t*)&data_in->data[2*i])];
            //                }
            //            }
            
            if (iqDataOutQueue != NULL && !iqDataOutQueue->full()) {
                DemodulatorThreadIQData *iqDataOut = visualDataBuffers.getBuffer();
                
                bool doVis = false;
                
                if (iqVisualQueue != NULL && !iqVisualQueue->full()) {
                    doVis = true;
                }
                
                iqDataOut->setRefCount(1 + (doVis?1:0));
                
                iqDataOut->frequency = data_in->frequency;
                iqDataOut->sampleRate = data_in->sampleRate;
                iqDataOut->data.assign(data_in->data.begin(), data_in->data.begin() + dataSize);

                iqDataOutQueue->push(iqDataOut);
                if (doVis) {
                    iqVisualQueue->push(iqDataOut);
                }
            }

            busy_demod.lock();
            
            if (frequency != data_in->frequency) {
                frequency = data_in->frequency;
                doRefresh.store(true);
            }

            if (doRefresh.load()) {
                updateActiveDemodulators();
                updateChannels();
                doRefresh.store(false);
            }

            DemodulatorInstance *activeDemod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
            int activeDemodChannel = -1;
            
            // Find active demodulators
            if (nRunDemods) {
                
//                for (int i = 0; i < numChannels; i++) {
//                    firpfbch_crcf_set_channel_state(channelizer, i, (demodChannelActive[i]>0)?1:0);
//                }
                
                // channelize data
                // firpfbch output rate is (input rate / channels)
                for (int i = 0, iMax = dataSize; i < iMax; i+=numChannels) {
                    firpfbch_crcf_analyzer_execute(channelizer, &data_in->data[i], &dataOut[i]);
                }

                for (int i = 0, iMax = numChannels; i < iMax; i++) {
                    demodChannelActive[i] = 0;
                }
                
                // Find nearest channel for each demodulator
                for (int i = 0; i < nRunDemods; i++) {
                    DemodulatorInstance *demod = runDemods[i];
                    long long minDelta = data_in->sampleRate;
                    for (int j = 0, jMax = numChannels; j < jMax; j++) {
                        // Distance from channel center to demod center
                        long long fdelta = abs(demod->getFrequency() - chanCenters[j]);
                        if (fdelta < minDelta) {
                            minDelta = fdelta;
                            demodChannel[i] = j;
                            if (demod == activeDemod) {
                                activeDemodChannel = j;
                            }
                        }
                    }
                }
                
                for (int i = 0; i < nRunDemods; i++) {
                    // cache channel usage refcounts
                    if (demodChannel[i] >= 0) {
                        demodChannelActive[demodChannel[i]]++;
                    }
                }
                
                // Run channels
                for (int i = 0; i < numChannels; i++) {
                    bool doVis = (activeDemodChannel == i) && (iqActiveDemodVisualQueue != NULL) && !iqActiveDemodVisualQueue->full();
                    
                    DemodulatorThreadIQData *demodDataOut = buffers.getBuffer();
                    demodDataOut->setRefCount(demodChannelActive[i] + (doVis?1:0));
                    demodDataOut->frequency = chanCenters[i];
                    demodDataOut->sampleRate = chanBw;

                    // Calculate channel buffer size
                    int chanDataSize = (outSize/numChannels);

                    if (demodDataOut->data.size() != chanDataSize) {
                        if (demodDataOut->data.capacity() < chanDataSize) {
                            demodDataOut->data.reserve(chanDataSize);
                        }
                        demodDataOut->data.resize(chanDataSize);
                    }

                    // prepare channel data buffer
                    if (i == 0) {   // Channel 0 requires DC correction
                        if (dcBuf.size() != chanDataSize) {
                            dcBuf.resize(chanDataSize);
                        }
                        for (int j = 0, idx = i; j < chanDataSize; j++) {
                            idx += numChannels;
                            dcBuf[j] = dataOut[idx];
                        }
                        iirfilt_crcf_execute_block(dcFilter, &dcBuf[0], chanDataSize, &demodDataOut->data[0]);
                    } else {
                        for (int j = 0, idx = i; j < chanDataSize; j++) {
                            idx += numChannels;
                            demodDataOut->data[j] = dataOut[idx];
                        }
                    }

                    if (doVis) {
                        iqActiveDemodVisualQueue->push(demodDataOut);
                    }
                    
                    for (int j = 0; j < nRunDemods; j++) {
                        if (demodChannel[j] == i) {
                            DemodulatorInstance *demod = runDemods[j];
                            demod->getIQInputDataPipe()->push(demodDataOut);
// std::cout << "Demodulator " << j << " in channel #" << i << " ctr: " << chanCenters[i] << " dataSize: " << chanDataSize << std::endl;
                        }
                    }
                }
            }
            
            busy_demod.unlock();
        }
        data_in->decRefCount();
    }

//    buffers.purge();
    
    if (iqVisualQueue && !iqVisualQueue->empty()) {
        DemodulatorThreadIQData *visualDataDummy;
        iqVisualQueue->pop(visualDataDummy);
    }

//    visualDataBuffers.purge();

    std::cout << "SDR post-processing thread done." << std::endl;
}

void SDRPostThread::terminate() {
    terminated = true;
    SDRThreadIQData *dummy = new SDRThreadIQData;
    iqDataInQueue->push(dummy);
}

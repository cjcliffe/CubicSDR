// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SDRPostThread.h"
#include "CubicSDRDefs.h"
#include "CubicSDR.h"

#include <vector>
#include <deque>
#include <memory>

//50 ms
#define HEARTBEAT_CHECK_PERIOD_MICROS (50 * 1000) 

SDRPostThread::SDRPostThread() : IOThread(), buffers("SDRPostThreadBuffers"), visualDataBuffers("SDRPostThreadVisualDataBuffers"), frequency(0) {
    iqDataInQueue = nullptr;
    iqDataOutQueue = nullptr;
    iqVisualQueue = nullptr;

    numChannels = 0;
    channelizer = nullptr;
    channelizer2 = nullptr;
    
    // Channel mode default is PFBCH
    chanMode = (int)SDRPostPFBCH;
    lastChanMode = 0;
    
    sampleRate = 0;
    
    doRefresh.store(false);
    dcFilter = iirfilt_crcf_create_dc_blocker(0.0005f);
}


SDRPostThread::~SDRPostThread() {
    iirfilt_crcf_destroy(dcFilter);
}


void SDRPostThread::notifyDemodulatorsChanged() {
    doRefresh.store(true);
}


// Update the active list of demodulators for handling
void SDRPostThread::updateActiveDemodulators() {
    // In range?
   
    runDemods.clear();
    demodChannel.clear();

    long long centerFreq = wxGetApp().getFrequency();

    //retreive the current list of demodulators:
    auto demodulators = wxGetApp().getDemodMgr().getDemodulators();
   
    for (auto demod : demodulators) {
            
        // not in range?
        if (demod->isDeltaLock()) {
            if (demod->getFrequency() != centerFreq + demod->getDeltaLockOfs()) {
                demod->setFrequency(centerFreq + demod->getDeltaLockOfs());
                demod->updateLabel(demod->getFrequency());
                demod->setFollow(false);
                demod->setTracking(false);
            }
        }
        
        if (abs(frequency - demod->getFrequency()) > (sampleRate / 2)) {
            // deactivate if active
           
            if (wxGetApp().getDemodMgr().getLastActiveDemodulator() == demod) {

                demod->setActive(false);
            }
            else if (demod->isActive() && !demod->isFollow() && !demod->isTracking()) {
                demod->setActive(false);
            } 
            
            // follow if follow mode
            if (demod->isFollow() && centerFreq != demod->getFrequency()) {
                wxGetApp().setFrequency(demod->getFrequency());
                demod->setFollow(false);
            }
        } else if (!demod->isActive()) { // in range, activate if not activated
            demod->setActive(true);
            if (wxGetApp().getDemodMgr().getLastActiveDemodulator() == nullptr) {

                wxGetApp().getDemodMgr().setActiveDemodulator(demod);
            }
        }
        
        if (!demod->isActive()) {
            continue;
        }
        
        // Add active demods to the current run:
        runDemods.push_back(demod);
        demodChannel.push_back(-1);
    }
}


void SDRPostThread::resetAllDemodulators() {
    //retreive the current list of demodulators:
    auto demodulators = wxGetApp().getDemodMgr().getDemodulators();

    for (auto demod : demodulators) {

        demod->setActive(false);
        demod->getIQInputDataPipe()->flush();
    }

    doRefresh.store(true);
}


// Update the channel positions and frequencies
void SDRPostThread::updateChannels() {
    // calculate channel center frequencies, todo: cache
    for (int i = 0; i < numChannels/2; i++) {
        int ofs = ((chanBw) * i);
        chanCenters[i] = frequency + ofs;
        chanCenters[i+(numChannels/2)] = frequency - (sampleRate/2) + ofs;
    }
    chanCenters[numChannels] = frequency + (sampleRate/2);
}


// Find the channelizer channel that corresponds to the given frequency
int SDRPostThread::getChannelAt(long long frequency) {
    int chan = -1;
    long long minDelta = sampleRate;
    for (int i = 0; i < numChannels+1; i++) {
        long long fdelta = abs(frequency - chanCenters[i]);
        if (fdelta < minDelta) {
            minDelta = fdelta;
            chan = i;
        }
    }
    return chan;
}


void SDRPostThread::setChannelizerType(SDRPostThreadChannelizerType chType) {
    chanMode.store((int)chType);
}


SDRPostThreadChannelizerType SDRPostThread::getChannelizerType() {
    return (SDRPostThreadChannelizerType) chanMode.load();
}


void SDRPostThread::run() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();  // ID of this thread
    int priority = sched_get_priority_max( SCHED_FIFO);
    sched_param prio = {priority}; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_FIFO, &prio);
#endif

//    std::cout << "SDR post-processing thread started.." << std::endl;

    iqDataInQueue = std::static_pointer_cast<SDRThreadIQDataQueue>(getInputQueue("IQDataInput"));
    iqDataOutQueue = std::static_pointer_cast<DemodulatorThreadInputQueue>(getOutputQueue("IQDataOutput"));
    iqVisualQueue = std::static_pointer_cast<DemodulatorThreadInputQueue>(getOutputQueue("IQVisualDataOutput"));
    iqActiveDemodVisualQueue = std::static_pointer_cast<DemodulatorThreadInputQueue>(getOutputQueue("IQActiveDemodVisualDataOutput"));
    
    while (!stopping) {
        SDRThreadIQDataPtr data_in;
        
        if (!iqDataInQueue->pop(data_in, HEARTBEAT_CHECK_PERIOD_MICROS)) {
            continue;
        }
         
        bool doUpdate = false;

        if (data_in && data_in->data.size()) {
           
            if(data_in->numChannels > 1) {
                if (chanMode == 1) {
                    runPFBCH(data_in.get());
                } else if (chanMode == 2) {
                    runPFBCH2(data_in.get());
                }
            } else {
                runSingleCH(data_in.get());
            }
        }
        
        for (size_t j = 0; j < runDemods.size(); j++) {
            DemodulatorInstancePtr demod = runDemods[j];
            if (abs(frequency - demod->getFrequency()) > (sampleRate / 2)) {
                doUpdate = true;
            }
        }
        
        //Only update the list of demodulators here
        if (doUpdate || doRefresh.load()) {
            updateActiveDemodulators();
            doRefresh.store(false);
        }
    } //end while
    
    //Be safe, remove as many elements as possible
    iqVisualQueue->flush();   
    iqDataInQueue->flush();
    iqDataOutQueue->flush();
    iqActiveDemodVisualQueue->flush();

//    std::cout << "SDR post-processing thread done." << std::endl;
}

void SDRPostThread::terminate() {
    IOThread::terminate();
    //unblock push()
    iqVisualQueue->flush();
    iqDataInQueue->flush();
    iqDataOutQueue->flush();
    iqActiveDemodVisualQueue->flush();
}

// Copy the full badwidth into a new DemodulatorThreadIQDataPtr.
DemodulatorThreadIQDataPtr SDRPostThread::getFullSampleRateIqData(SDRThreadIQData *data_in) {

    DemodulatorThreadIQDataPtr iqDataOut = visualDataBuffers.getBuffer();

    iqDataOut->frequency = data_in->frequency;
    iqDataOut->sampleRate = data_in->sampleRate;
    iqDataOut->data.assign(data_in->data.begin(), data_in->data.begin() + data_in->data.size());

    return iqDataOut;
}

// Push visual data; i.e. Main Waterfall (all frames) and Spectrum (active frame)
void SDRPostThread::pushVisualData(DemodulatorThreadIQDataPtr iqDataOut) {

    if (iqDataOutQueue != nullptr) {
        
        //non-blocking push here, we can afford to loose some samples for a ever-changing visual display.
        iqDataOutQueue->try_push(iqDataOut);

        if (iqVisualQueue != nullptr) {
            //non-blocking push here, we can afford to loose some samples for a ever-changing visual display.
            iqVisualQueue->try_push(iqDataOut);
        }
    }
}

// Run without any processing; each demod gets the full SDR bandwidth to handle on it's own
void SDRPostThread::runSingleCH(SDRThreadIQData *data_in) {
    bool refreshed = false;
    
    if (sampleRate != data_in->sampleRate || doRefresh.load()) {
        sampleRate = data_in->sampleRate;
        numChannels = 1;
        refreshed = true;
    }

    if (refreshed || frequency != data_in->frequency) {
        frequency = data_in->frequency;
        updateActiveDemodulators();
    }
    
    size_t outSize = data_in->data.size();
    
    if (outSize > dataOut.capacity()) {
        dataOut.reserve(outSize);
    }
    if (outSize != dataOut.size()) {
        dataOut.resize(outSize);
    }
    
    DemodulatorThreadIQDataPtr demodDataOut = buffers.getBuffer();

    demodDataOut->frequency = frequency;
    demodDataOut->sampleRate = sampleRate;
    
    if (demodDataOut->data.size() != outSize) {
        if (demodDataOut->data.capacity() < outSize) {
            demodDataOut->data.reserve(outSize);
        }
        demodDataOut->data.resize(outSize);
    }
    
    //Only 1 channel, apply DC blocker.
    iirfilt_crcf_execute_block(dcFilter, &data_in->data[0], data_in->data.size(), &demodDataOut->data[0]);

    //push the DC-corrected data as Main Spactrum + Waterfall data.
    pushVisualData(demodDataOut);

    if (runDemods.size() > 0 && iqActiveDemodVisualQueue != nullptr) {
        //non-blocking push here, we can afford to loose some samples for a ever-changing visual display.
        iqActiveDemodVisualQueue->try_push(demodDataOut);
    }
    
    for (size_t i = 0; i < runDemods.size(); i++) {
        // try-push() : we do our best to only stimulate active demods, but some could happen to be dead, full, or indeed non-active.
        //so in short never block here no matter what.
        runDemods[i]->getIQInputDataPipe()->try_push(demodDataOut);
    }
}


// Handle active channels, channel 0 offset correction, de-interlacing and push data to demodulators
void SDRPostThread::runDemodChannels(int channelBandwidth) {
    DemodulatorInstancePtr activeDemod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    // Calculate channel data size
    size_t chanDataSize = dataOut.size()/numChannels;

    // Channel for the 'active' demod that's displaying visual data
    int activeDemodChannel = -1;
    
    for (int i = 0, iMax = numChannels+1; i < iMax; i++) {
        demodChannelActive[i] = 0;
    }
    
    // Find nearest channel for each demodulator
    for (size_t i = 0; i < runDemods.size(); i++) {
        DemodulatorInstancePtr demod = runDemods[i];
        demodChannel[i] = getChannelAt(demod->getFrequency());
        if (demod == activeDemod) {
            activeDemodChannel = demodChannel[i];
        }
    }
    
    // Count the demods per-channel
    for (size_t i = 0; i < runDemods.size(); i++) {
        if (demodChannel[i] >= 0) {
            demodChannelActive[demodChannel[i]]++;
        }
    }

    // Run channels
    for (int i = 0; i < numChannels+1; i++) {
        bool doDemodVis = (activeDemodChannel == i) && (iqActiveDemodVisualQueue != nullptr);
        
        if (!doDemodVis && demodChannelActive[i] == 0) {
            // Nothing to do for this channel? continue.
            continue;
        }
        
        // Get a channel buffer
        DemodulatorThreadIQDataPtr demodDataOut = buffers.getBuffer();
        demodDataOut->frequency = chanCenters[i];
        demodDataOut->sampleRate = channelBandwidth;

        // Resize and update capacity of buffer if necessary
        if (demodDataOut->data.size() != chanDataSize) {
            if (demodDataOut->data.capacity() < chanDataSize) {
                demodDataOut->data.reserve(chanDataSize);
            }
            demodDataOut->data.resize(chanDataSize);
        }
        
        // Start copying interleaved data at given channel index
        int idx = i;
        
        // Extra channel wraps left side band of lowest channel
        // to fix frequency gap on right side of spectrum
        if (i == numChannels) {
            idx = (numChannels/2);
        }
        
        // prepare channel data buffer
        if (i == 0) {   // Channel 0 requires DC correction
            // Update DC Buffer size if needed
            if (dcBuf.size() != chanDataSize) {
                dcBuf.resize(chanDataSize);
            }
            // Copy interleaved channel data to dc buffer
            for (size_t j = 0; j < chanDataSize; j++) {
                dcBuf[j] = dataOut[idx];
                idx += numChannels;
            }
            // Run DC Filter from dcBuf to demod output buffer
            iirfilt_crcf_execute_block(dcFilter, &dcBuf[0], chanDataSize, &demodDataOut->data[0]);
        } else {
            // Copy interleaved channel data to demod output buffer
            for (size_t j = 0; j < chanDataSize; j++) {
                demodDataOut->data[j] = dataOut[idx];
                idx += numChannels;
            }
        }
        
        if (doDemodVis) {
            //non-blocking push here, we can afford to loose some samples for a ever-changing visual display.
            iqActiveDemodVisualQueue->try_push(demodDataOut);
        }
        
        for (size_t j = 0; j < runDemods.size(); j++) {
            if (demodChannel[j] == i) {
                
                // try-push() : we do our best to only stimulate active demods, but some could happen to be dead, full, or indeed non-active.
                //so in short never block here no matter what.
                runDemods[j]->getIQInputDataPipe()->try_push(demodDataOut);
            }
        } //end for
    }
}


void SDRPostThread::initPFBCH() {
    //    std::cout << "Initializing post-process FIR polyphase filterbank channelizer with " << numChannels << " channels." << std::endl;
    if (channelizer) {
        firpfbch_crcf_destroy(channelizer);
    }
    channelizer = firpfbch_crcf_create_kaiser(LIQUID_ANALYZER, numChannels, 4, 60);
    
    chanBw = (sampleRate / numChannels);
    
    chanCenters.resize(numChannels+1);
    demodChannelActive.resize(numChannels+1);
    
    //    std::cout << "Channel bandwidth spacing: " << (chanBw) << std::endl;
}

void SDRPostThread::runPFBCH(SDRThreadIQData *data_in) {
    bool refreshed = false;
    if (numChannels != data_in->numChannels || sampleRate != data_in->sampleRate || chanMode != lastChanMode || doRefresh.load()) {
        numChannels = data_in->numChannels;
        sampleRate = data_in->sampleRate;
        initPFBCH();
        lastChanMode = 1;
        refreshed = true;
    }
    
    if (refreshed || frequency != data_in->frequency) {
        frequency = data_in->frequency;
        updateActiveDemodulators();
        updateChannels();
    }

    //push the full data_in into (Main spectrum + waterfall) visual queue:
    DemodulatorThreadIQDataPtr fullSampleRateIQ = getFullSampleRateIqData(data_in);
    pushVisualData(fullSampleRateIQ);
    
    size_t outSize = data_in->data.size();
    
    if (outSize > dataOut.capacity()) {
        dataOut.reserve(outSize);
    }
    if (outSize != dataOut.size()) {
        dataOut.resize(outSize);
    }
    
    // Find active demodulators
    if (runDemods.size() > 0) {
        // Channelize data
        // firpfbch produces [numChannels] interleaved output samples for every [numChannels] samples
        for (int i = 0, iMax = data_in->data.size(); i < iMax; i+=numChannels) {
            firpfbch_crcf_analyzer_execute(channelizer, &data_in->data[i], &dataOut[i]);
        }
        
        runDemodChannels(chanBw);
    }
}


void SDRPostThread::initPFBCH2() {
    //    std::cout << "Initializing post-process FIR polyphase filterbank channelizer with " << numChannels << " channels." << std::endl;
    if (channelizer2) {
        firpfbch2_crcf_destroy(channelizer2);
    }
    channelizer2 = firpfbch2_crcf_create_kaiser(LIQUID_ANALYZER, numChannels, 4, 60);
    
    chanBw = (sampleRate / numChannels);
    
    chanCenters.resize(numChannels+1);
    demodChannelActive.resize(numChannels+1);
    //    std::cout << "Channel bandwidth spacing: " << (chanBw) << std::endl;
}

void SDRPostThread::runPFBCH2(SDRThreadIQData *data_in) {
    bool refreshed = false;
    if (numChannels != data_in->numChannels || sampleRate != data_in->sampleRate || chanMode != lastChanMode || doRefresh.load()) {
        numChannels = data_in->numChannels;
        sampleRate = data_in->sampleRate;
        initPFBCH2();
        lastChanMode = 2;
        refreshed = true;
    }

    if (refreshed || frequency != data_in->frequency) {
        frequency = data_in->frequency;
        updateActiveDemodulators();
        updateChannels();
    }

    //push the full data_in into (Main spectrum + waterfall) visual queue:
    DemodulatorThreadIQDataPtr fullSampleRateIQ = getFullSampleRateIqData(data_in);
    pushVisualData(fullSampleRateIQ);
    
    size_t outSize = data_in->data.size() * 2;
    
    if (outSize > dataOut.capacity()) {
        dataOut.reserve(outSize);
    }
    if (outSize != dataOut.size()) {
        dataOut.resize(outSize);
    }
    
    // Find active demodulators
    if (runDemods.size() > 0) {
        // Channelize data
        // firpfbch2 produces [numChannels] interleaved output samples for every [numChannels/2] input samples
        for (int i = 0, iMax = data_in->data.size(); i < iMax; i += numChannels/2) {
            firpfbch2_crcf_execute(channelizer2, &data_in->data[i], &dataOut[i*2]);
        }
        
        runDemodChannels(chanBw * 2);
    }
}

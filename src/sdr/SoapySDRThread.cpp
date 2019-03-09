// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SoapySDRThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include "CubicSDR.h"
#include <string>
#include <algorithm>
#include <SoapySDR/Logger.h>
#include <chrono>

#define TARGET_DISPLAY_FPS 60

SDRThread::SDRThread() : IOThread(), buffers("SDRThreadBuffers") {
    device = nullptr;

    deviceConfig.store(nullptr);
    deviceInfo.store(nullptr);

    sampleRate.store(DEFAULT_SAMPLE_RATE);
    frequency.store(0);
    offset.store(0);
    ppm.store(0);

    numElems.store(0);
    
    rate_changed.store(false);
    freq_changed.store(false);
    offset_changed.store(false);
    antenna_changed.store(false);
    ppm_changed .store(false);
    device_changed.store(false);

    hasPPM.store(false);
    hasHardwareDC.store(false);
    numChannels.store(8);
    
    agc_mode.store(true);
    agc_mode_changed.store(false);
    gain_value_changed.store(false);
    setting_value_changed.store(false);
    frequency_lock_init.store(false);
    frequency_locked.store(false);
    lock_freq.store(0);
    iq_swap.store(false);
}

SDRThread::~SDRThread() {

}

SoapySDR::Kwargs SDRThread::combineArgs(SoapySDR::Kwargs a, SoapySDR::Kwargs b) {
    SoapySDR::Kwargs c;
    SoapySDR::Kwargs::iterator i;
    for (i = a.begin(); i != a.end(); i++) {
        c[i->first] = i->second;
    }
    for (i = b.begin(); i != b.end(); i++) {
        c[i->first] = i->second;
    }
    return c;
}

bool SDRThread::init() {
//#warning Debug On
//    SoapySDR_setLogLevel(SOAPY_SDR_DEBUG);
    
    SDRDeviceInfo *devInfo = deviceInfo.load();
    deviceConfig.store(wxGetApp().getConfig()->getDevice(devInfo->getDeviceId()));
    DeviceConfig *devConfig = deviceConfig.load();
    
    ppm.store(devConfig->getPPM());
    ppm_changed.store(true);
    
    std::string driverName = devInfo->getDriver();

    offset = devConfig->getOffset();
    
    SoapySDR::Kwargs args = devInfo->getDeviceArgs();
    
    wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("Initializing device."));
    
    device = devInfo->getSoapyDevice();
    
    SoapySDR::Kwargs currentStreamArgs = combineArgs(devInfo->getStreamArgs(),streamArgs);
    
    std::string streamExceptionStr("");
    
    try {
        stream = device->setupStream(SOAPY_SDR_RX,"CF32", std::vector<size_t>(), currentStreamArgs);
    } catch(exception e) {
        streamExceptionStr = e.what();
    }

    if (!stream) {
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_FAILED, std::string("Stream setup failed, stream is null. ") + streamExceptionStr);
        std::cout << "Stream setup failed, stream is null. " << streamExceptionStr << std::endl;
        return false;
    }
    
    int streamMTU = device->getStreamMTU(stream);
    mtuElems.store(streamMTU);
  
    deviceInfo.load()->setStreamArgs(currentStreamArgs);
    deviceConfig.load()->setStreamOpts(currentStreamArgs);
    
    wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("Activating stream."));
    device->setSampleRate(SOAPY_SDR_RX,0,sampleRate.load());
    
    // TODO: explore bandwidth setting option to see if this is necessary for others
    if (device->getDriverKey() == "bladeRF") {
        device->setBandwidth(SOAPY_SDR_RX, 0, sampleRate.load());
    }
        
    device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency - offset.load());
    device->activateStream(stream);
    if (devInfo->hasCORR(SOAPY_SDR_RX, 0)) {
        hasPPM.store(true);
        device->setFrequency(SOAPY_SDR_RX,0,"CORR",ppm.load());
    } else {
        hasPPM.store(false);
    }
    if (device->hasDCOffsetMode(SOAPY_SDR_RX, 0)) {
        hasHardwareDC.store(true);
//        wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("Found hardware DC offset correction support, internal disabled."));
        device->setDCOffsetMode(SOAPY_SDR_RX, 0, true);
    } else {
        hasHardwareDC.store(false);
    }
    
    device->setGainMode(SOAPY_SDR_RX,0,agc_mode.load());
    
    numChannels.store(getOptimalChannelCount(sampleRate.load()));
    numElems.store(getOptimalElementCount(sampleRate.load(), TARGET_DISPLAY_FPS));

    //fallback if  mtuElems was wrong
    if (!mtuElems.load()) {
        mtuElems.store(numElems.load());
        std::cout << "SDRThread::init(): Device Stream MTU is broken, use " << mtuElems.load() << "instead..." << std::endl << std::flush;
    } else {
        std::cout << "SDRThread::init(): Device Stream set to MTU: " << mtuElems.load() << std::endl << std::flush;
    }

    overflowBuffer.data.resize(mtuElems.load());
    
    buffs[0] = malloc(mtuElems.load() * 4 * sizeof(float));
    numOverflow = 0;
    
    SoapySDR::ArgInfoList settingsInfo = device->getSettingInfo();
    SoapySDR::ArgInfoList::const_iterator settings_i;
    
    if (!setting_value_changed.load()) {
        settings.erase(settings.begin(), settings.end());
        settingChanged.erase(settingChanged.begin(), settingChanged.end());
    }
    
	//apply settings.
    { //enter scoped-lock
        std::lock_guard < std::mutex > lock(setting_busy);

        for (settings_i = settingsInfo.begin(); settings_i != settingsInfo.end(); settings_i++) {
            SoapySDR::ArgInfo setting = (*settings_i);
            if ((settingChanged.find(setting.key) != settingChanged.end()) && (settings.find(setting.key) != settings.end())) {              
				device->writeSetting(setting.key, settings[setting.key]);
                settingChanged[setting.key] = false;
            } else {
                settings[setting.key] = device->readSetting(setting.key);
                settingChanged[setting.key] = false;
            }
        }
        setting_value_changed.store(false);

    } //leave lock guard scope
    
    updateSettings();
    
    wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_INITIALIZED, std::string("Device Initialized."));
	
	//rebuild menu now that settings are really been applied.
	wxGetApp().notifyMainUIOfDeviceChange(true);

    return true;
}

void SDRThread::deinit() {
    device->deactivateStream(stream);
    device->closeStream(stream);
    free(buffs[0]);
}

void SDRThread::assureBufferMinSize(SDRThreadIQData * dataOut, size_t minSize) {
    
    if (dataOut->data.size() < minSize) {
        dataOut->data.resize(minSize);
    }
}

//Called in an infinite loop, read SaopySDR device to build 
// a 'this.numElems' sized batch of samples (SDRThreadIQData) and push it into  iqDataOutQueue.
//this batch of samples is built to represent 1 frame / TARGET_DISPLAY_FPS.
int SDRThread::readStream(SDRThreadIQDataQueuePtr iqDataOutQueue) {
    
    int flags(0);
    
    long long timeNs(0);

    // Supply a huge timeout value to neutralize the readStream 'timeout' effect
    // we are not interested in, but some modules may effectively use. 
    //TODO: use something roughly (1 / TARGET_DISPLAY_FPS) seconds * (factor) instead.?
    long timeoutUs = (1 << 30);

    int n_read = 0;
    int nElems = numElems.load();
    int mtElems = mtuElems.load();

    // Warning: if MTU > numElems, i.e if device MTU is too big w.r.t the sample rate, the TARGET_DISPLAY_FPS cannot
    //be reached and the CubicSDR displays "slows down". 
    //To get back a TARGET_DISPLAY_FPS, the user need to adapt 
    //the SoapySDR Device to use smaller buffer sizes, because  
    // readStream() is suited to device MTU and cannot be really adapted dynamically.
    //TODO: Add in doc the need to reduce SoapySDR device buffer length (if available) to restore higher fps.

    //0. Retreive a new batch 
    SDRThreadIQDataPtr dataOut = buffers.getBuffer();

    //resize to the target size immedialetly, to minimize later reallocs:
    assureBufferMinSize(dataOut.get(), nElems);

    //1.If overflow occured on the previous readStream(), transfer it in dataOut directly. 
    if (numOverflow > 0) {
        int n_overflow = std::min(numOverflow, nElems);
        
        //safety
        assureBufferMinSize(dataOut.get(), n_overflow);

        ::memcpy(&dataOut->data[0], &overflowBuffer.data[0], n_overflow * sizeof(liquid_float_complex));
        n_read = n_overflow;

        //is still > 0 if MTU > nElements (low sample rate w.r.t the MTU !)
        numOverflow -= n_overflow;

        // std::cout << "SDRThread::readStream() 1.1 overflowBuffer not empty, collect the remaining " << n_overflow << " samples in it..." << std::endl;
        
        if (numOverflow > 0) { // still some left, shift the remaining samples to the begining..
            ::memmove(&overflowBuffer.data[0], &overflowBuffer.data[n_overflow], numOverflow * sizeof(liquid_float_complex));

        //    std::cout << "SDRThread::readStream() 1.2 overflowBuffer still not empty, compact the remaining " << numOverflow << " samples in it..." << std::endl;
        }
    } //end if numOverflow > 0
    
    int readStreamCode = 0;

    //2. attempt readStream() at most nElems, by mtElems-sized chunks, append in dataOut->data directly.
    while (n_read < nElems && !stopping) {
        
        //Whatever the number of remaining samples needed to reach nElems,  we always try to read a mtElems-size chunk,
        //from which SoapySDR effectively returns n_stream_read.
        int n_stream_read = device->readStream(stream, buffs, mtElems, flags, timeNs, timeoutUs);
        
        readStreamCode = n_stream_read;

        //if the n_stream_read <= 0, bail out from reading. 
        if (n_stream_read == 0) {
             std::cout << "SDRThread::readStream(): 2. SoapySDR read blocking..." << std::endl;
             break;
        }
        else if (n_stream_read < 0) {
            std::cout << "SDRThread::readStream(): 2. SoapySDR read failed with code: " << n_stream_read << std::endl;
            break;
        }
        
        //sucess read beyond nElems, so with overflow:
        if ((n_read + n_stream_read) > nElems) {

            //n_requested is the exact number to reach nElems.
            int n_requested = nElems-n_read;
    
            //Copy at most n_requested CF32 into .data liquid_float_complex,
            //starting at n_read position.
            //inspired from SoapyRTLSDR code, this mysterious void** is indeed an array of CF32(real/imag) samples, indeed an array of 
            //float with the following layout [sample 1 real part , sample 1 imag part,  sample 2 real part , sample 2 imag part,sample 3 real part , sample 3 imag part,...etc]
            //Since there is indeed no garantee that sizeof(liquid_float_complex) = 2 * sizeof (float)
            //nor that the Re/Im layout of fields matches the float array order, assign liquid_float_complex field by field.
            float *pp = (float *)buffs[0];

            //safety
            assureBufferMinSize(dataOut.get(), n_read + n_requested);

            if (iq_swap.load()) {
                for (int i = 0; i < n_requested; i++) {
                    dataOut->data[n_read + i].imag = pp[2 * i];
                    dataOut->data[n_read + i].real = pp[2 * i + 1];
                }
            } else {
                for (int i = 0; i < n_requested; i++) {
                    dataOut->data[n_read + i].real = pp[2 * i];
                    dataOut->data[n_read + i].imag = pp[2 * i + 1];
                }
            }
           
           //shift of n_requested samples, each one made of 2 floats...
            pp += n_requested * 2;

            //numNewOverflow are in exess, they have to be added in the existing overflowBuffer.
            int numNewOverflow = n_stream_read - n_requested;

            //so push the remainder samples to overflowBuffer:
            if (numNewOverflow > 0) {
            //	std::cout << "SDRThread::readStream(): 2. SoapySDR read make nElems overflow by " << numNewOverflow << " samples..." << std::endl;
            }

            //safety
            assureBufferMinSize(&overflowBuffer, numOverflow + numNewOverflow);

            if (iq_swap.load()) {

                for (int i = 0; i < numNewOverflow; i++) {
                    overflowBuffer.data[numOverflow + i].imag = pp[2 * i];
                    overflowBuffer.data[numOverflow + i].real = pp[2 * i + 1];
                }
            }
            else {
                for (int i = 0; i < numNewOverflow; i++) {
                    overflowBuffer.data[numOverflow + i].real = pp[2 * i];
                    overflowBuffer.data[numOverflow + i].imag = pp[2 * i + 1];
                }
            }
            numOverflow += numNewOverflow;
           
            n_read += n_requested;
        } else if (n_stream_read > 0) { // no overflow, read the whole n_stream_read.

            float *pp = (float *)buffs[0];

            //safety
            assureBufferMinSize(dataOut.get(), n_read + n_stream_read);

            if (iq_swap.load()) {
                for (int i = 0; i < n_stream_read; i++) {
                    dataOut->data[n_read + i].imag = pp[2 * i];
                    dataOut->data[n_read + i].real = pp[2 * i + 1];
                }
            }
            else {
                for (int i = 0; i < n_stream_read; i++) {
                    dataOut->data[n_read + i].real = pp[2 * i];
                    dataOut->data[n_read + i].imag = pp[2 * i + 1];
                }
            } 

            n_read += n_stream_read;
        } else {
            break;
        }
    } //end while
    
    //3. At that point, dataOut contains nElems (or less if a read has return an error), try to post in queue, else discard.
    if (n_read > 0 && !stopping && !iqDataOutQueue->full()) {
        
        //clamp result to the actual read size:
        dataOut->data.resize(n_read);

        dataOut->frequency = frequency.load();
        dataOut->sampleRate = sampleRate.load();
        dataOut->dcCorrected = hasHardwareDC.load();
        dataOut->numChannels = numChannels.load();
        
        if (!iqDataOutQueue->try_push(dataOut)) {
            //The rest of the system saturates,
            //finally the push didn't suceeded.
            readStreamCode = -32;
            std::cout << "SDRThread::readStream(): 3.2 iqDataOutQueue output queue is full, discard processing of the batch..." << std::endl;

            //saturation, let a chance to the other threads to consume the existing samples
            std::this_thread::yield();
        }
    }
    else {
        readStreamCode = -31;
        std::cout << "SDRThread::readStream(): 3.1 iqDataOutQueue output queue is full, discard processing of the batch..." << std::endl;
        //saturation, let a chance to the other threads to consume the existing samples
        std::this_thread::yield();
    }

    return readStreamCode;
}


void SDRThread::readLoop() {
  
    SDRThreadIQDataQueuePtr iqDataOutQueue = std::static_pointer_cast<SDRThreadIQDataQueue>( getOutputQueue("IQDataOutput"));
    
    if (iqDataOutQueue == nullptr) {
        return;
    }
    
    updateGains();
 
    while (!stopping.load()) {

        updateSettings();

        readStream(iqDataOutQueue);

    } //End while

    iqDataOutQueue->flush();
}

void SDRThread::updateGains() {
    SDRDeviceInfo *devInfo = deviceInfo.load();
    
    gainValues.erase(gainValues.begin(),gainValues.end());
    gainChanged.erase(gainChanged.begin(),gainChanged.end());
    
    SDRRangeMap gains = devInfo->getGains(SOAPY_SDR_RX, 0);
    for (SDRRangeMap::iterator gi = gains.begin(); gi != gains.end(); gi++) {
        gainValues[gi->first] = device->getGain(SOAPY_SDR_RX, 0, gi->first);
        gainChanged[gi->first] = false;
    }
    
    gain_value_changed.store(false);
}

void SDRThread::updateSettings() {
    bool doUpdate = false;
    
    if (!stream) {
        return;
    }

    if (antenna_changed.load()) {
        
       device->setAntenna(SOAPY_SDR_RX, 0, antennaName);
           
       antenna_changed.store(false);
    }
    
    if (offset_changed.load()) {
        if (!freq_changed.load()) {
            frequency.store(frequency.load());
            freq_changed.store(true);
        }
        offset_changed.store(false);
    }
    
    if (rate_changed.load()) {
        device->setSampleRate(SOAPY_SDR_RX,0,sampleRate.load());
        // TODO: explore bandwidth setting option to see if this is necessary for others
        if (device->getDriverKey() == "bladeRF") {
            device->setBandwidth(SOAPY_SDR_RX, 0, sampleRate.load());
        }
	// Fix for LimeSDR-USB not properly handling samplerate changes while device is 
	// active.
	else if (device->getHardwareKey() == "LimeSDR-USB") {
	    std::cout << "SDRThread::updateSettings(): Force deactivate / activate limeSDR stream" << std::endl << std::flush;
            device->deactivateStream(stream);
	    device->activateStream(stream);
        }
        sampleRate.store(device->getSampleRate(SOAPY_SDR_RX,0));
        numChannels.store(getOptimalChannelCount(sampleRate.load()));
        numElems.store(getOptimalElementCount(sampleRate.load(), TARGET_DISPLAY_FPS));
        int streamMTU = device->getStreamMTU(stream);

        mtuElems.store(streamMTU);
        
        //fallback if  mtuElems was wrong
        if (!mtuElems.load()) {
            mtuElems.store(numElems.load());
            std::cout << "SDRThread::updateSettings(): Device Stream MTU is broken, use " << mtuElems.load() << "instead..." << std::endl << std::flush;
        } else {
            std::cout << "SDRThread::updateSettings(): Device Stream changing to MTU: " << mtuElems.load() << std::endl << std::flush;
        }

        overflowBuffer.data.resize(mtuElems.load());
        free(buffs[0]);
        buffs[0] = malloc(mtuElems.load() * 4 * sizeof(float));
        //clear overflow buffer
        numOverflow = 0;

        rate_changed.store(false);
        doUpdate = true;
    }
    
    if (ppm_changed.load() && hasPPM.load()) {
        device->setFrequency(SOAPY_SDR_RX,0,"CORR",ppm.load());
        ppm_changed.store(false);
    }
    
    if (freq_changed.load()) {
        if (frequency_locked.load() && !frequency_lock_init.load()) {
            device->setFrequency(SOAPY_SDR_RX,0,"RF",lock_freq.load());
            frequency_lock_init.store(true);
        } else if (!frequency_locked.load()) {
            device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency.load() - offset.load());
        }
        freq_changed.store(false);
    }
    
//    double devFreq = device->getFrequency(SOAPY_SDR_RX,0);
//    if (((long long)devFreq + offset.load()) != frequency.load()) {
//        wxGetApp().setFrequency((long long)devFreq + offset.load());
//    }
    
    if (agc_mode_changed.load()) {
        device->setGainMode(SOAPY_SDR_RX, 0, agc_mode.load());
        agc_mode_changed.store(false);
        if (!agc_mode.load()) {
            updateGains();
            
			//re-apply the saved configuration gains:
            DeviceConfig *devConfig = deviceConfig.load();
            ConfigGains gains = devConfig->getGains();
            
            for (ConfigGains::iterator gain_i = gains.begin(); gain_i != gains.end(); gain_i++) {
                setGain(gain_i->first, gain_i->second);
            }
        }
        doUpdate = true;
    }
    
    if (gain_value_changed.load() && !agc_mode.load()) {
        std::lock_guard < std::mutex > lock(gain_busy); 

        for (std::map<std::string,bool>::iterator gci = gainChanged.begin(); gci != gainChanged.end(); gci++) {
            if (gci->second) {
                device->setGain(SOAPY_SDR_RX, 0, gci->first, gainValues[gci->first]);
                gainChanged[gci->first] = false;
            }
        }
        
        gain_value_changed.store(false);
    }
    
    if (setting_value_changed.load()) {

        std::lock_guard < std::mutex > lock(setting_busy);
        
        for (std::map<std::string, bool>::iterator sci = settingChanged.begin(); sci != settingChanged.end(); sci++) {
            if (sci->second) {
                device->writeSetting(sci->first, settings[sci->first]);
                settingChanged[sci->first] = false;
            }
        }
        
        setting_value_changed.store(false);
        
        doUpdate = true;
    }
    
    if (doUpdate) {
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_INITIALIZED, std::string("Settings updated."));
    }
}

void SDRThread::run() {
//#ifdef __APPLE__
//    pthread_t tID = pthread_self();  // ID of this thread
//    int priority = sched_get_priority_max( SCHED_FIFO);
//    sched_param prio = { priority }; // scheduling priority of thread
//    pthread_setschedparam(tID, SCHED_FIFO, &prio);
//#endif

    std::cout << "SDR thread starting." << std::endl;
    
    SDRDeviceInfo *activeDev = deviceInfo.load();
    
    if (activeDev != nullptr) {
        std::cout << "device init()" << std::endl;
        if (!init()) {
            std::cout << "SDR Thread stream init error." << std::endl;
            return;
        }
        std::cout << "starting readLoop()" << std::endl;
        activeDev->setActive(true);
        readLoop();
        activeDev->setActive(false);
        std::cout << "readLoop() ended." << std::endl;
        deinit();
        std::cout << "device deinit()" << std::endl;
    } else {
        std::cout << "SDR Thread started with null device?" << std::endl;
    }
    
    std::cout << "SDR thread done." << std::endl;
}

void SDRThread::terminate() {
    IOThread::terminate();

    SDRThreadIQDataQueuePtr iqDataOutQueue = std::static_pointer_cast<SDRThreadIQDataQueue>(getOutputQueue("IQDataOutput"));

    if (iqDataOutQueue != nullptr) {
        iqDataOutQueue->flush();
    }
}

SDRDeviceInfo *SDRThread::getDevice() {
    return deviceInfo.load();
}

void SDRThread::setDevice(SDRDeviceInfo *dev) {
    deviceInfo.store(dev);
    if (dev) {
        deviceConfig.store(wxGetApp().getConfig()->getDevice(dev->getDeviceId()));
    } else {
        deviceConfig.store(nullptr);
    }
}

int SDRThread::getOptimalElementCount(long long sampleRate, int fps) {
    int elemCount = (int)floor((double)sampleRate/(double)fps);
    int nch = numChannels.load();
    elemCount = int(ceil((double)elemCount/(double)nch))*nch;
//    std::cout << "Calculated optimal " << numChannels.load() << " channel element count of " << elemCount << std::endl;
    return elemCount;
}

int SDRThread::getOptimalChannelCount(long long sampleRate) {
    if (sampleRate <= CHANNELIZER_RATE_MAX) {
        return 1;
    }
    
    int optimal_rate = CHANNELIZER_RATE_MAX;
    int optimal_count = int(ceil(double(sampleRate)/double(optimal_rate)));
    
    if (optimal_count % 2 == 1) {
        optimal_count--;
    }
    
    if (optimal_count < 2) {
        optimal_count = 2;
    }

    return optimal_count;
}


void SDRThread::setFrequency(long long freq) {
    if (freq < sampleRate.load() / 2) {
        freq = sampleRate.load() / 2;
    }
    frequency.store(freq);
    freq_changed.store(true);
}

long long SDRThread::getFrequency() {
    return frequency.load();
}

void SDRThread::lockFrequency(long long freq) {
    lock_freq.store(freq);
    frequency_locked.store(true);
    frequency_lock_init.store(false);
    setFrequency(freq);
}

bool SDRThread::isFrequencyLocked() {
    return frequency_locked.load();
}

void SDRThread::unlockFrequency() {
    frequency_locked.store(false);
    frequency_lock_init.store(false);
    freq_changed.store(true);
}

void SDRThread::setOffset(long long ofs) {
    offset.store(ofs);
    offset_changed.store(true);

    DeviceConfig *devConfig = deviceConfig.load();
    if (devConfig) {
        devConfig->setOffset(ofs);
    }

//    std::cout << "Set offset: " << offset.load() << std::endl;
}

long long SDRThread::getOffset() {
    return offset.load();
}

void SDRThread::setAntenna(const std::string& name) {
    antennaName = name;
    antenna_changed.store(true);

    DeviceConfig *devConfig = deviceConfig.load();
    if (devConfig) {
        devConfig->setAntennaName(antennaName);
    }
}

std::string SDRThread::getAntenna() {
    return antennaName;
}

void SDRThread::setSampleRate(long rate) {
    sampleRate.store(rate);
    rate_changed = true;
    DeviceConfig *devConfig = deviceConfig.load();
    if (devConfig) {
        devConfig->setSampleRate(rate);
    }
//    std::cout << "Set sample rate: " << sampleRate.load() << std::endl;
}
long SDRThread::getSampleRate() {
    return sampleRate.load();
}

void SDRThread::setPPM(int ppm) {
    this->ppm.store(ppm);
    ppm_changed.store(true);

    DeviceConfig *devConfig = deviceConfig.load();
    if (devConfig) {
        devConfig->setPPM(ppm);
    }

//    std::cout << "Set PPM: " << this->ppm.load() << std::endl;
}

int SDRThread::getPPM() {
    return ppm.load();
}

void SDRThread::setAGCMode(bool mode) {
    agc_mode.store(mode);
    agc_mode_changed.store(true);
    DeviceConfig *devConfig = deviceConfig.load();
    if (devConfig) {
        devConfig->setAGCMode(mode);
    }
}

bool SDRThread::getAGCMode() {
    return agc_mode.load();
}

void SDRThread::setIQSwap(bool swap) {
    iq_swap.store(swap);
}

bool SDRThread::getIQSwap() {
    return iq_swap.load();
}

void SDRThread::setGain(std::string name, float value) {
    std::lock_guard < std::mutex > lock(gain_busy);
    gainValues[name] = value;
    gainChanged[name] = true;
    gain_value_changed.store(true);
    
    DeviceConfig *devConfig = deviceConfig.load();
    if (devConfig) {
        devConfig->setGain(name, value);
    }
}

float SDRThread::getGain(std::string name) {
    std::lock_guard < std::mutex > lock(gain_busy);
    float val = gainValues[name];
    
    return val;
}

void SDRThread::writeSetting(std::string name, std::string value) {

    std::lock_guard < std::mutex > lock(setting_busy);

    settings[name] = value;
    settingChanged[name] = true;
    setting_value_changed.store(true);
    if (deviceConfig.load() != nullptr) {
        deviceConfig.load()->setSetting(name, value);
    }
}

std::string SDRThread::readSetting(std::string name) {
    std::string val;
    std::lock_guard < std::mutex > lock(setting_busy);

    val = device->readSetting(name);
   
    return val;
}

void SDRThread::setStreamArgs(SoapySDR::Kwargs streamArgs_in) {
    streamArgs = streamArgs_in;
}

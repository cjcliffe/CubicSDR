// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SoapySDRThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include "CubicSDR.h"
#include <string>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <thread>

#define TARGET_DISPLAY_FPS (60)
#define SDR_DEVICE_LOST (-666)

SDRThread::SDRThread() : IOThread(), buffers("SDRThreadBuffers") {
    device = nullptr;

    deviceConfig.store(nullptr);
    deviceInfo.store(nullptr);

    sampleRate.store(DEFAULT_SAMPLE_RATE);
    pendingSampleRate.store(DEFAULT_SAMPLE_RATE);
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

SDRThread::~SDRThread() = default;

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
    
    try {
        device = devInfo->getSoapyDevice();
    } catch (const std::exception &e) {
        std::cout << "SDRThread: device creation failed: " << e.what() << std::endl << std::flush;
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_FAILED, std::string("Device creation failed: ") + e.what());
        releaseStream();
        return false;
    } catch (...) {
        std::cout << "SDRThread: device creation failed with unknown error." << std::endl << std::flush;
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_FAILED, std::string("Device creation failed."));
        releaseStream();
        return false;
    }
    
    SoapySDR::Kwargs currentStreamArgs = combineArgs(devInfo->getStreamArgs(),streamArgs);
    
    std::string streamExceptionStr;
    
    //1. setup stream for CF32:
    try {
        stream = device->setupStream(SOAPY_SDR_RX,"CF32", std::vector<size_t>(), currentStreamArgs);
    } catch(const exception &e) {
        streamExceptionStr = e.what();
    }

    if (!stream) {
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_FAILED, std::string("Stream setup failed, stream is null. ") + streamExceptionStr);
        std::cout << "Stream setup failed, stream is null. " << streamExceptionStr << std::endl;
        releaseStream();
        return false;
    }

    //2. Set sample rate:
    // Adopt the pending rate first: on a device switch the running rate is
    // stale from the previous device and may be invalid for this one.
    sampleRate.store(pendingSampleRate.load());
    // A throwing driver must fail device start, not kill the app.
    try {
    device->setSampleRate(SOAPY_SDR_RX, 0, sampleRate.load());

    //3. Store Stream-specific parameters to current Device
    deviceInfo.load()->setStreamArgs(currentStreamArgs);
    deviceConfig.load()->setStreamOpts(currentStreamArgs);
  
    //4. Apply other settings: Frequency, PPM correction, Gains,  Device-specific settings:
    device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency - offset.load());

    if (device->hasFrequencyCorrection(SOAPY_SDR_RX, 0)) {
        hasPPM.store(true);
        device->setFrequencyCorrection(SOAPY_SDR_RX, 0, ppm.load());
    } else if (devInfo->hasCORR(SOAPY_SDR_RX, 0)) {
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

    if (device->hasGainMode(SOAPY_SDR_RX, 0)) {
        device->setGainMode(SOAPY_SDR_RX, 0, agc_mode.load());
    }
    
    SoapySDR::ArgInfoList settingsInfo = device->getSettingInfo();
    SoapySDR::ArgInfoList::const_iterator settings_i;
    
    if (!setting_value_changed.load()) {
        settings.clear();
        settingChanged.clear();
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
    } catch (const std::exception &e) {
        std::cout << "SDRThread: device configuration failed: " << e.what() << std::endl << std::flush;
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_FAILED, std::string("Device configuration failed: ") + e.what());
        releaseStream();
        return false;
    } catch (...) {
        std::cout << "SDRThread: device configuration failed with unknown error." << std::endl << std::flush;
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_FAILED, std::string("Device configuration failed."));
        releaseStream();
        return false;
    }
      
    wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_INITIALIZED, std::string("Device Initialized."));

    //5. Activate stream: (through update settings)
    wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("Activating stream."));

    rate_changed.store(true);
    try {
        updateSettings();
    } catch (const std::exception &e) {
        std::cout << "SDRThread: stream activation failed: " << e.what() << std::endl << std::flush;
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_FAILED, std::string("Stream activation failed: ") + e.what());
        releaseStream();
        return false;
    } catch (...) {
        std::cout << "SDRThread: stream activation failed with unknown error." << std::endl << std::flush;
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_FAILED, std::string("Stream activation failed."));
        releaseStream();
        return false;
    }

	//rebuild menu now that settings are really been applied.
	wxGetApp().notifyMainUIOfDeviceChange(true);

    return true;
}

void SDRThread::releaseStream() {
    if (!stream) {
        return;
    }
    // Teardown must complete even if the driver throws: a wedged device
    // must not take the process down with it.
    try {
        device->deactivateStream(stream);
    } catch (const std::exception &e) {
        std::cout << "SDRThread: deactivateStream failed during teardown: " << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cout << "SDRThread: deactivateStream failed during teardown." << std::endl << std::flush;
    }
    try {
        device->closeStream(stream);
    } catch (const std::exception &e) {
        std::cout << "SDRThread: closeStream failed during teardown: " << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cout << "SDRThread: closeStream failed during teardown." << std::endl << std::flush;
    }

    stream = nullptr;
}

void SDRThread::deinit() {
    releaseStream();

    if (buffs[0] != nullptr) {
        ::free(buffs[0]);
        buffs[0] = nullptr;
    }
}

void SDRThread::assureBufferMinSize(SDRThreadIQData * dataOut, size_t minSize) {
    
    if (dataOut->data.size() < minSize) {
        dataOut->data.resize(minSize);
    }
}

//Called in an infinite loop, read SaopySDR device to build 
// a 'this.numElems' sized batch of samples (SDRThreadIQData) and push it into  iqDataOutQueue.
//this batch of samples is built to represent 1 frame / TARGET_DISPLAY_FPS.
int SDRThread::readStream(const SDRThreadIQDataQueuePtr& iqDataOutQueue) {
    
    int flags(0);
    
    long long timeNs(0);

    // Supply a huge timeout value so reads block until the requested slice
    // fills; with frame-sized slices that wait is the frame clock.
    long timeoutUs = (1 << 30);

    int n_read = 0;
    int nElems = numElems.load();
    int mtElems = mtuElems.load();

    // Reads below are sliced to the frame size, so a large device MTU only
    // bounds the request instead of setting the cadence: each read fills in
    // about one frame time and blocks go out evenly at the display rate.

    //0. Retrieve a new batch 
    SDRThreadIQDataPtr dataOut = buffers.getBuffer();

    //resize to the target size immedialetly, to minimize later reallocs:
    assureBufferMinSize(dataOut.get(), nElems);

    //1.If overflow occurred on the previous readStream(), transfer it in dataOut directly. 
    if (numOverflow > 0) {
        int n_overflow = std::min(numOverflow, nElems);
        
        //safety
        assureBufferMinSize(dataOut.get(), n_overflow);

        ::memcpy(&dataOut->data[0], &overflowBuffer.data[0], n_overflow * sizeof(liquid_float_complex));
        n_read = n_overflow;

        //is still > 0 if MTU > nElements (low sample rate w.r.t the MTU !)
        numOverflow -= n_overflow;

        // std::cout << "SDRThread::readStream() 1.1 overflowBuffer not empty, collect the remaining " << n_overflow << " samples in it..." << std::endl;
        
        if (numOverflow > 0) { // still some left, shift the remaining samples to the beginning.
            ::memmove(&overflowBuffer.data[0], &overflowBuffer.data[n_overflow], numOverflow * sizeof(liquid_float_complex));

        //    std::cout << "SDRThread::readStream() 1.2 overflowBuffer still not empty, compact the remaining " << numOverflow << " samples in it..." << std::endl;
        }
    } //end if numOverflow > 0
    
    //default means blocking.
    int readStreamCode = 0;

    //2. attempt readStream() at most nElems, appending in dataOut->data directly.
    while (n_read < nElems && !stopping) {

        //Read only what the frame still needs: a full-MTU request would
        //block for the whole MTU before any block goes out, bursting once
        //per MTU instead of pacing at the frame rate.
        int n_stream_read = device->readStream(stream, buffs, std::min(mtElems, nElems - n_read), flags, timeNs, timeoutUs);
        
        readStreamCode = n_stream_read;

        //if the n_stream_read <= 0, bail out from reading. 
        if (n_stream_read == 0) {
             std::cout << "SDRThread::readStream(): 2. SoapySDR read blocking..." << std::endl;
             break;
        }
        else if (n_stream_read < 0) {

            //trace here interesting error codes from the Soapy side.
            switch (n_stream_read) {

            case SOAPY_SDR_TIMEOUT:
                std::cout << "SDRThread::readStream(): 2. SoapySDR read failed with code SOAPY_SDR_TIMEOUT";
                break;
            case SOAPY_SDR_STREAM_ERROR:
                std::cout << "SDRThread::readStream(): 2. SoapySDR read failed with code SOAPY_SDR_STREAM_ERROR";
                break;

            case SOAPY_SDR_CORRUPTION:
                std::cout << "SDRThread::readStream(): 2. SoapySDR read failed with code SOAPY_SDR_CORRUPTION";
                break;

            case SOAPY_SDR_NOT_SUPPORTED:
                //return a special code to mean that the device has to be stopped entirely:
                std::cout << "SDRThread::readStream(): 2. SoapySDR read failed with code SOAPY_SDR_NOT_SUPPORTED => stopping device.";
                readStreamCode = SDR_DEVICE_LOST;
                break;
            default:
                std::cout << "SDRThread::readStream(): 2. SoapySDR read failed with unknown code: " << n_stream_read << std::endl;
            }
        }
        
        //success read beyond nElems, so with overflow:
        if ((n_read + n_stream_read) > nElems) {

            //n_requested is the exact number to reach nElems.
            int n_requested = nElems-n_read;
    
            //Copy at most n_requested CF32 into .data liquid_float_complex,
            //starting at n_read position.
            //inspired from SoapyRTLSDR code, this mysterious void** is indeed an array of CF32(real/imag) samples, indeed an array of 
            //float with the following layout [sample 1 real part , sample 1 imag part,  sample 2 real part , sample 2 imag part,sample 3 real part , sample 3 imag part,...etc]
            //Since there is indeed no guarantee that sizeof(liquid_float_complex) = 2 * sizeof (float)
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
            //finally the push didn't succeeded.
            readStreamCode = 0;
            std::cout << "SDRThread::readStream(): 3.2 iqDataOutQueue output queue is full, discard processing of the batch..." << std::endl;

            //saturation, let a chance to the other threads to consume the existing samples
            std::this_thread::yield();
        }
    }
    else {
        readStreamCode = 0;
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
    
    try {
        updateGains();
        while (!stopping.load()) {

            updateSettings();

            if (SDR_DEVICE_LOST == readStream(iqDataOutQueue)) {
                //stop reading loop:
                stopping = true;
            }
        } //End while
    }
    catch (...) {
        //notify App of device loss:
        std::cout << "SDRThread::readLoop() exception, stopping device..." << std::endl << std::flush;
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_FAILED, std::string("SDRThread::readLoop() exception, stopping device."));
        stopping = true;
    }

    iqDataOutQueue->flush();
}

void SDRThread::updateGains() {
    SDRDeviceInfo *devInfo = deviceInfo.load();
    
    gainValues.clear();
    gainChanged.clear();
    
    SDRRangeMap gains = devInfo->getGains(SOAPY_SDR_RX, 0);
    for (auto & gain : gains) {
        gainValues[gain.first] = device->getGain(SOAPY_SDR_RX, 0, gain.first);
        gainChanged[gain.first] = false;
    }
    
    gain_value_changed.store(false);
}

// Discard whatever the driver has buffered (non-blocking reads until
// empty, bounded), so the stream resumes from live samples.
void SDRThread::drainStream() {
    if (buffs[0] == nullptr) {
        // Fresh stream, nothing allocated to drain into yet.
        return;
    }
    for (int i = 0; i < 4096; i++) {
        int drainFlags = 0;
        long long drainTimeNs = 0;
        if (device->readStream(stream, buffs, mtuElems.load(), drainFlags, drainTimeNs, 0) <= 0) {
            break;
        }
    }
}

void SDRThread::updateSettings() {
    bool doUpdate = false;
    
    if (!stream) {
        return;
    }

    if (antenna_changed.load()) {
        antenna_changed.store(false);
        
       std::string antennaCopy;
       {
           std::lock_guard < std::mutex > lock(antenna_busy);
           antennaCopy = antennaName;
       }
       device->setAntenna(SOAPY_SDR_RX, 0, antennaCopy);
           
    }
    
    if (offset_changed.load()) {
        offset_changed.store(false);
        if (!freq_changed.load()) {
            frequency.store(frequency.load());
            freq_changed.store(true);
        }
    }
    
    if (rate_changed.load()) {
        rate_changed.store(false);

        //Apply the pending rate here, at a block boundary, so in-flight
        //reads keep the old rate instead of producing new-rate labels
        //with old-rate channel counts.
        sampleRate.store(pendingSampleRate.load());

        //Pre-switch drain: the driver holds a partially-consumed buffer
        //across deactivate/activate, which permanently skews its ring;
        //discard it (non-blocking) so the switch starts from a clean state.
        drainStream();

        //1. Silence the device:
        try {
            device->deactivateStream(stream);
        } catch (const std::exception &e) {
            std::cout << "SDRThread: deactivateStream failed: " << e.what() << std::endl << std::flush;
        } catch (...) {
            std::cout << "SDRThread: deactivateStream failed." << std::endl << std::flush;
        }

        //2. Set the (new) sample rate
        try {
            device->setSampleRate(SOAPY_SDR_RX, 0, sampleRate.load());
        }
        catch (...) {
            std::cout << "SDRThread: Exception while setting the sample rate = " << sampleRate.load() << std::endl << std::flush;
        }

        //2.3 Device-specific workarounds:
        // TODO: explore bandwidth setting option to see if this is necessary for others
        if (device->getDriverKey() == "bladeRF") {
            device->setBandwidth(SOAPY_SDR_RX, 0, sampleRate.load());
        }

        //3. Re-activate stream:
        try {
            device->activateStream(stream);
        } catch (const std::exception &e) {
            std::cout << "SDRThread: activateStream failed: " << e.what() << std::endl << std::flush;
        } catch (...) {
            std::cout << "SDRThread: activateStream failed." << std::endl << std::flush;
        }

        //4. Re-do Cubic buffers :
        //4.1. re-read current sample rate and MTU:
        //4.2. The device MAY force another sample rate than the one requested, or refusing the change altogether,
        // because of limitations of hardware and the SoapySDR river
        double requested_sample_rate = (double)sampleRate.load();
        double applied_sample_rate = device->getSampleRate(SOAPY_SDR_RX, 0);

        if (abs((requested_sample_rate - applied_sample_rate) / requested_sample_rate) > 0.05) {
            //force the applied_sample_rate as the new effective user setting
            DeviceConfig* devConfig = deviceConfig.load();
            if (devConfig) {
                devConfig->setSampleRate(applied_sample_rate);
            }

            std::cout << "SDRThread: WARNING Requested sample rate is " << requested_sample_rate << " but " << applied_sample_rate << " was applied." <<std::endl << std::flush;
        } 

        sampleRate.store(applied_sample_rate);

        numChannels.store(getOptimalChannelCount(sampleRate.load()));
        numElems.store(getOptimalElementCount(sampleRate.load(), TARGET_DISPLAY_FPS));
        //read (new) MTU size:
        int streamMTU = device->getStreamMTU(stream);
        mtuElems.store(streamMTU);

        //fallback if  mtuElems was wrong
        if (!mtuElems.load()) {
            mtuElems.store(numElems.load());
            std::cout << "SDRThread: Device Stream MTU is broken, use " << mtuElems.load() << " instead..." << std::endl << std::flush;
        } else {
            std::cout << "SDRThread : Device Stream set to MTU: " << mtuElems.load() << std::endl << std::flush;
        }

        overflowBuffer.data.resize(mtuElems.load());
        if (buffs[0] != nullptr) {
            ::free(buffs[0]);
        }
        buffs[0] = ::malloc(mtuElems.load() * 4 * sizeof(float));
        //clear overflow buffer
        numOverflow = 0;

        doUpdate = true;
    }
    
    if (ppm_changed.load() && hasPPM.load()) {
        ppm_changed.store(false);
        if (device->hasFrequencyCorrection(SOAPY_SDR_RX, 0)) {
            device->setFrequencyCorrection(SOAPY_SDR_RX, 0, ppm.load());
        } else {
            device->setFrequency(SOAPY_SDR_RX, 0, "CORR", ppm.load());
        }
    }
    
    if (freq_changed.load()) {
        // Clear before the slow driver call: tunes requested while it
        // runs stay pending for the next turn instead of being
        // acknowledged here. One tune per turn keeps the stream live
        // during drags while still converging on the latest request.
        freq_changed.store(false);
        bool tuned = false;
        if (frequency_locked.load() && !frequency_lock_init.load()) {
            device->setFrequency(SOAPY_SDR_RX,0,"RF",lock_freq.load());
            frequency_lock_init.store(true);
            tuned = true;
        } else if (!frequency_locked.load()) {
            device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency.load() - offset.load());
            tuned = true;
        }
        // Post-tune drain: samples buffered while the call ran were
        // captured at the old frequency; discard them (non-blocking)
        // so the stream resumes live instead of lagging behind them.
        if (tuned) {
            drainStream();
        }
    }
   
    
    if (agc_mode_changed.load()) {
        agc_mode_changed.store(false);
        if (device->hasGainMode(SOAPY_SDR_RX, 0)) {
            device->setGainMode(SOAPY_SDR_RX, 0, agc_mode.load());
        }
        if (!agc_mode.load()) {
            updateGains();
            
			//re-apply the saved configuration gains:
            DeviceConfig *devConfig = deviceConfig.load();
            ConfigGains gains = devConfig->getGains();
            
            for (auto & gain : gains) {
                setGain(gain.first, gain.second);
            }
        }
        doUpdate = true;
    }
    
    if (gain_value_changed.load() && !agc_mode.load()) {
        std::lock_guard < std::mutex > lock(gain_busy); 

        gain_value_changed.store(false);
        for (auto & gci : gainChanged) {
            if (gci.second) {
                device->setGain(SOAPY_SDR_RX, 0, gci.first, gainValues[gci.first]);
                gainChanged[gci.first] = false;
            }
        }
        
    }
    
    if (setting_value_changed.load()) {

        std::lock_guard < std::mutex > lock(setting_busy);
        
        setting_value_changed.store(false);
        for (auto & sci : settingChanged) {
            if (sci.second) {
                device->writeSetting(sci.first, settings[sci.first]);
                settingChanged[sci.first] = false;
            }
        }
        
        
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

int SDRThread::getOptimalElementCount(long long sampleRate_in, int fps) {
    int elemCount = (int)floor((double)sampleRate_in / (double)fps);
    int nch = numChannels.load();
    elemCount = int(ceil((double)elemCount/(double)nch))*nch;
//    std::cout << "Calculated optimal " << numChannels.load() << " channel element count of " << elemCount << std::endl;
    return elemCount;
}

int SDRThread::getOptimalChannelCount(long long sampleRate_in) {
    if (sampleRate_in <= CHANNELIZER_RATE_MAX) {
        return 1;
    }
    
    int optimal_rate = CHANNELIZER_RATE_MAX;
    int optimal_count = int(ceil(double(sampleRate_in) / double(optimal_rate)));
    
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
    std::lock_guard < std::mutex > lock(antenna_busy);
    antennaName = name;
    antenna_changed.store(true);

    DeviceConfig *devConfig = deviceConfig.load();
    if (devConfig) {
        devConfig->setAntennaName(antennaName);
    }
}

std::string SDRThread::getAntenna() {
    std::lock_guard < std::mutex > lock(antenna_busy);
    return antennaName;
}

void SDRThread::setSampleRate(long rate) {
    pendingSampleRate.store(rate);
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

void SDRThread::setPPM(int ppm_in) {
    ppm.store(ppm_in);
    ppm_changed.store(true);

    DeviceConfig *devConfig = deviceConfig.load();
    if (devConfig) {
        devConfig->setPPM(ppm_in);
    }

//    std::cout << "Set PPM: " << this->ppm_in.load() << std::endl;
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

void SDRThread::setGain(const std::string& name, float value) {
    std::lock_guard < std::mutex > lock(gain_busy);
    gainValues[name] = value;
    gainChanged[name] = true;
    gain_value_changed.store(true);
    
    DeviceConfig *devConfig = deviceConfig.load();
    if (devConfig) {
        devConfig->setGain(name, value);
    }
}

float SDRThread::getGain(const std::string& name) {
    std::lock_guard < std::mutex > lock(gain_busy);
    float val = gainValues[name];
    
    return val;
}

void SDRThread::writeSetting(const std::string& name, std::string value) {

    std::lock_guard < std::mutex > lock(setting_busy);

    settings[name] = value;
    settingChanged[name] = true;
    setting_value_changed.store(true);
    if (deviceConfig.load() != nullptr) {
        deviceConfig.load()->setSetting(name, value);
    }
}

std::string SDRThread::readSetting(const std::string& name) {
    std::string val;
    std::lock_guard < std::mutex > lock(setting_busy);

    val = device->readSetting(name);
   
    return val;
}

void SDRThread::setStreamArgs(SoapySDR::Kwargs streamArgs_in) {
    streamArgs = streamArgs_in;
}

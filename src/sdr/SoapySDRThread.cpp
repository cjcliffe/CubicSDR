#include "SoapySDRThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include "CubicSDR.h"
#include <string>


SDRThread::SDRThread() : IOThread() {
    device = NULL;

    deviceConfig.store(NULL);
    deviceInfo.store(NULL);

    sampleRate.store(0);
    frequency.store(0);
    offset.store(0);
    ppm.store(0);
    direct_sampling_mode.store(0);

    numElems.store(0);
    
    rate_changed.store(false);
    freq_changed.store(false);
    offset_changed.store(false);
    ppm_changed .store(false);
    direct_sampling_changed.store(false);
    device_changed.store(false);

    hasPPM.store(false);
    hasHardwareDC.store(false);
    numChannels.store(8);

//    dcFilter = iirfilt_crcf_create_dc_blocker(0.0005);
}

SDRThread::~SDRThread() {

}

void SDRThread::init() {
    SDRDeviceInfo *devInfo = deviceInfo.load();
    deviceConfig.store(wxGetApp().getConfig()->getDevice(devInfo->getDeviceId()));
    DeviceConfig *devConfig = deviceConfig.load();
    
    ppm.store(devConfig->getPPM());
    direct_sampling_mode.store(devConfig->getDirectSampling());

    std::string driverName = devInfo->getDriver();

    offset = devConfig->getOffset();
    wxGetApp().setSwapIQ(devConfig->getIQSwap());
    
    SoapySDR::Kwargs args = devInfo->getDeviceArgs();
    
    args["direct_samp"] = std::to_string(devConfig->getDirectSampling());
    
    if (driverName == "rtl" || driverName == "rtlsdr") {
        args["iq_swap"] = std::to_string(devConfig->getIQSwap()?1:0);
        args["buffers"] = "6";
        args["buflen"] = "16384";
        hasPPM = true;
    } else {
        hasPPM = false;
    }
    
    wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("Initializing device."));
    device = SoapySDR::Device::make(args);
    stream = device->setupStream(SOAPY_SDR_RX,"CF32", std::vector<size_t>(), devInfo->getStreamArgs());
    
    wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("Activating stream."));
    device->setSampleRate(SOAPY_SDR_RX,0,sampleRate.load());
    device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency - offset.load());
    device->activateStream(stream);
    SDRDeviceChannel *chan = devInfo->getRxChannel();
    if (chan->hasCORR()) {
        hasPPM.store(true);
        device->setFrequency(SOAPY_SDR_RX,0,"CORR",ppm.load());
    } else {
        hasPPM.store(false);
    }
    if (chan->hasHardwareDC()) {
        hasHardwareDC.store(true);
//        wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("Found hardware DC offset correction support, internal disabled."));
        device->setDCOffsetMode(SOAPY_SDR_RX, chan->getChannel(), true);
    } else {
        hasHardwareDC.store(false);
    }

    device->setGainMode(SOAPY_SDR_RX,0,true);
    
    numChannels.store(getOptimalChannelCount(sampleRate.load()));
    numElems.store(getOptimalElementCount(sampleRate.load(), 30));
    inpBuffer.data.resize(numElems.load());
    
    buffs[0] = malloc(numElems * 2 * sizeof(float));
}

void SDRThread::deinit() {
    device->deactivateStream(stream);
    device->closeStream(stream);
    SoapySDR::Device::unmake(device);
    free(buffs[0]);
}

void SDRThread::readStream(SDRThreadIQDataQueue* iqDataOutQueue) {
    int flags;
    long long timeNs;


    int n_read = 0;
    while (n_read != numElems && !terminated) {
        int n_stream_read = device->readStream(stream, buffs, numElems-n_read, flags, timeNs);
        if (n_stream_read > 0) {
            memcpy(&inpBuffer.data[n_read], buffs[0], n_stream_read * sizeof(float) * 2);
            n_read += n_stream_read;
        } else {
            break;
        }
    }
    
    if (n_read > 0 && !terminated) {
        SDRThreadIQData *dataOut = buffers.getBuffer();

//        if (hasHardwareDC) {
            dataOut->data.assign(inpBuffer.data.begin(), inpBuffer.data.begin()+n_read);
//        } else {
//            if (dataOut->data.size() != n_read) {
//                dataOut->data.resize(n_read);
//            }
//            iirfilt_crcf_execute_block(dcFilter, &inpBuffer.data[0], n_read, &dataOut->data[0]);
//        }

        
        dataOut->setRefCount(1);
        dataOut->frequency = frequency.load();
        dataOut->sampleRate = sampleRate.load();
        dataOut->dcCorrected = hasHardwareDC.load();
        dataOut->numChannels = numChannels.load();
        
        iqDataOutQueue->push(dataOut);
    }
}

void SDRThread::readLoop() {
    SDRThreadIQDataQueue* iqDataOutQueue = (SDRThreadIQDataQueue*) getOutputQueue("IQDataOutput");
    
    if (iqDataOutQueue == NULL) {
        return;
    }
    
    while (!terminated.load()) {
        if (offset_changed.load()) {
            if (!freq_changed.load()) {
                frequency.store(frequency.load());
                freq_changed.store(true);
            }
            offset_changed.store(false);
        }
        if (rate_changed.load()) {
            device->setSampleRate(SOAPY_SDR_RX,0,sampleRate.load());
            sampleRate.store(device->getSampleRate(SOAPY_SDR_RX,0));
            numChannels.store(getOptimalChannelCount(sampleRate.load()));
            numElems.store(getOptimalElementCount(sampleRate.load(), 60));
            inpBuffer.data.resize(numElems.load());
            free(buffs[0]);
            buffs[0] = malloc(numElems.load() * 2 * sizeof(float));
            rate_changed.store(false);
        }
        if (ppm_changed.load() && hasPPM.load()) {
            device->setFrequency(SOAPY_SDR_RX,0,"CORR",ppm.load());
            direct_sampling_changed.store(false);
        }
        if (freq_changed.load()) {
            device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency.load() - offset.load());
            freq_changed.store(false);
        }
        if (direct_sampling_changed.load()) {
            //                rtlsdr_set_direct_sampling(dev, direct_sampling_mode);
        }
        
        readStream(iqDataOutQueue);
    }
    buffers.purge();
}


void SDRThread::run() {
//#ifdef __APPLE__
//    pthread_t tID = pthread_self();  // ID of this thread
//    int priority = sched_get_priority_max( SCHED_FIFO);
//    sched_param prio = { priority }; // scheduling priority of thread
//    pthread_setschedparam(tID, SCHED_FIFO, &prio);
//#endif

    std::cout << "SDR thread starting." << std::endl;
    terminated.store(false);
    
    if (deviceInfo.load() != NULL) {
        std::cout << "device init()" << std::endl;
        init();
        std::cout << "starting readLoop()" << std::endl;
        readLoop();
        std::cout << "readLoop() ended." << std::endl;
        deinit();
        std::cout << "device deinit()" << std::endl;
    } else {
        std::cout << "SDR Thread started with null device?" << std::endl;
    }
    
    std::cout << "SDR thread done." << std::endl;
    
    if (!terminated.load()) {
        terminated.store(true);
        wxGetApp().sdrThreadNotify(SDRThread::SDR_THREAD_TERMINATED, "Done.");
    }
}


SDRDeviceInfo *SDRThread::getDevice() {
    return deviceInfo.load();
}

void SDRThread::setDevice(SDRDeviceInfo *dev) {
    deviceInfo.store(dev);
    deviceConfig.store(wxGetApp().getConfig()->getDevice(dev->getDeviceId()));
}

int SDRThread::getOptimalElementCount(long long sampleRate, int fps) {
    int elemCount = (int)floor((double)sampleRate/(double)fps);
    int nch = numChannels.load();
    elemCount = int(ceil((double)elemCount/(double)nch))*nch;
    std::cout << "Calculated optimal " << numChannels.load() << " channel element count of " << elemCount << std::endl;
    return elemCount;
}

int SDRThread::getOptimalChannelCount(long long sampleRate) {
    int optimal_rate = CHANNELIZER_RATE_MAX;
    int optimal_count = int(ceil(double(sampleRate)/double(optimal_rate)));
    
    if (optimal_count % 2 == 1) {
        optimal_count--;
    }
    
    if (optimal_count < 4) {
        optimal_count = 4;
    }
    
//    if (optimal_count > 16) {
//        optimal_count = 16;
//    }
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

void SDRThread::setOffset(long long ofs) {
    offset.store(ofs);
    offset_changed.store(true);
    std::cout << "Set offset: " << offset.load() << std::endl;
}

long long SDRThread::getOffset() {
    return offset.load();
}

void SDRThread::setSampleRate(int rate) {
    sampleRate.store(rate);
    rate_changed = true;
    std::cout << "Set sample rate: " << sampleRate.load() << std::endl;
}
int SDRThread::getSampleRate() {
    return sampleRate.load();
}

void SDRThread::setPPM(int ppm) {
    this->ppm.store(ppm);
    ppm_changed.store(true);
    std::cout << "Set PPM: " << this->ppm.load() << std::endl;
}

int SDRThread::getPPM() {
    return ppm.load();
}

void SDRThread::setDirectSampling(int dsMode) {
    direct_sampling_mode.store(dsMode);
    direct_sampling_changed.store(true);
    std::cout << "Set direct sampling mode: " << this->direct_sampling_mode.load() << std::endl;
}

int SDRThread::getDirectSampling() {
    return direct_sampling_mode.load();
}

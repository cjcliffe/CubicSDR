#include "SoapySDRThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include "CubicSDR.h"
#include <string>

#include <SoapySDR/Version.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Device.hpp>

std::vector<std::string> SDRThread::factories;
std::vector<std::string> SDRThread::modules;
std::vector<SDRDeviceInfo *> SDRThread::devs;


SDRThread::SDRThread() : IOThread() {
	offset.store(0);
	deviceId.store(-1);
//    dev = NULL;
    sampleRate.store(DEFAULT_SAMPLE_RATE);
}

SDRThread::~SDRThread() {
//    rtlsdr_close(dev);
}


std::vector<SDRDeviceInfo *> *SDRThread::enumerate_devices() {

    if (SDRThread::devs.size()) {
        return &SDRThread::devs;
    }
    
    std::cout << "SoapySDR init.." << std::endl;
    std::cout << "\tAPI Version: v" << SoapySDR::getAPIVersion() << std::endl;
    std::cout << "\tABI Version: v" << SoapySDR::getABIVersion() << std::endl;
    std::cout << "\tInstall root: " << SoapySDR::getRootPath() << std::endl;

    modules = SoapySDR::listModules();
    for (size_t i = 0; i < modules.size(); i++) {
        std::cout << "\tModule found: " << modules[i] << std::endl;
    }
    if (modules.empty()) {
        std::cout << "No modules found!" << std::endl;
    }
    
    std::cout << "\tLoading modules... " << std::flush;
    SoapySDR::loadModules();
    std::cout << "done" << std::endl;
    
    if (SDRThread::factories.size()) {
        SDRThread::factories.erase(SDRThread::factories.begin(), SDRThread::factories.end());
    }
    
    std::cout << "\tAvailable factories...";
    SoapySDR::FindFunctions factories = SoapySDR::Registry::listFindFunctions();
    for (SoapySDR::FindFunctions::const_iterator it = factories.begin(); it != factories.end(); ++it) {
        if (it != factories.begin()) {
            std::cout << ", ";
        }
        std::cout << it->first;
        SDRThread::factories.push_back(it->first);
    }
    if (factories.empty()) {
        std::cout << "No factories found!" << std::endl;
    }
    std::cout << std::endl;

    std::vector<SoapySDR::Kwargs> results = SoapySDR::Device::enumerate();

    // Remote driver test..
/* * /
    SDRDeviceInfo *remoteDev = new SDRDeviceInfo();
    remoteDev->setDriver("remote");
    remoteDev->setName("SoapySDR Remote Test");
    
    SoapySDR::Kwargs remoteArgs;
    remoteArgs["driver"] = "remote";
//    remoteArgs["remote"] = "127.0.0.1";
    remoteArgs["remote"] = "192.168.1.103";
    remoteArgs["remote:driver"] = "rtlsdr";
    remoteArgs["buffers"] = "6";
    remoteArgs["buflen"] = "16384";
    remoteDev->setDeviceArgs(remoteArgs);

    SoapySDR::Kwargs streamArgs;
    streamArgs["remote:mtu"] = "8192";
    streamArgs["remote:format"] = "CS8";
    streamArgs["remote:window"] = "16384000";
    remoteDev->setStreamArgs(streamArgs);
    
    SDRThread::devs.push_back(remoteDev);
//  */
    
    for (size_t i = 0; i < results.size(); i++) {
        std::cout << "Found device " << i << std::endl;
        SDRDeviceInfo *dev = new SDRDeviceInfo();
        for (SoapySDR::Kwargs::const_iterator it = results[i].begin(); it != results[i].end(); ++it) {
            std::cout << "  " << it->first << " = " << it->second << std::endl;
            if (it->first == "driver") {
                dev->setDriver(it->second);
            } else if (it->first == "label") {
                dev->setName(it->second);
            }
        }
        
        dev->setDeviceArgs(results[i]);
        
        std::cout << "Make device " << i << std::endl;
        try {
            SoapySDR::Device *device = SoapySDR::Device::make(dev->getDeviceArgs());
            SoapySDR::Kwargs info = device->getHardwareInfo();
            for (SoapySDR::Kwargs::const_iterator it = info.begin(); it != info.end(); ++it) {
                std::cout << "  " << it->first << "=" << it->second << std::endl;
                if (it->first == "hardware") {
                    dev->setHardware(it->second);
                }
            }
            
            if (device->hasDCOffsetMode(SOAPY_SDR_RX, 0)) {
                device->setDCOffsetMode(SOAPY_SDR_RX, 0, true);
                std::cout << "Hardware DC offset support detected; internal DC offset correction will be disabled." << std::endl;
                dev->setHardwareDC(true);
            } else {
                dev->setHardwareDC(false);
            }

            SoapySDR::Device::unmake(device);
            
            dev->setAvailable(true);
        } catch (const std::exception &ex) {
            std::cerr << "Error making device: " << ex.what() << std::endl;
            dev->setAvailable(false);
        }
        std::cout << std::endl;

        SDRThread::devs.push_back(dev);
    }
    if (results.empty()) {
        std::cout << "No devices found!" << std::endl;
    }
    std::cout << std::endl;

    return &SDRThread::devs;
}

void SDRThread::run() {
//#ifdef __APPLE__
//    pthread_t tID = pthread_self();  // ID of this thread
//    int priority = sched_get_priority_max( SCHED_FIFO) - 1;
//    sched_param prio = { priority }; // scheduling priority of thread
//    pthread_setschedparam(tID, SCHED_FIFO, &prio);
//#endif

    std::cout << "SDR thread initializing.." << std::endl;
    
    if (deviceId == -1 && devs.size() == 0) {
        std::cout << "No devices found.. SDR Thread exiting.." << std::endl;
        return;
    } else {
        if (deviceId == -1) {
            deviceId = 0;
        }
        std::cout << "Using device #" << deviceId << std::endl;
    }

    DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(devs[deviceId]->getDeviceId());

    long long frequency = wxGetApp().getConfig()->getCenterFreq();
    int ppm = devConfig->getPPM();
    int direct_sampling_mode = devConfig->getDirectSampling();
    int numElems = 0;
    bool hasPPM = false;
    bool hasHardwareDC = false;
    
    offset.store(devConfig->getOffset());
    wxGetApp().setSwapIQ(devConfig->getIQSwap());

    
    SDRDeviceInfo *dev = devs[deviceId];
    SoapySDR::Kwargs args = dev->getDeviceArgs();
    
    std::string driverName = dev->getDriver();
    
    if (driverName == "rtl" || driverName == "rtlsdr") {
        hasPPM = true;
    }
    
    args["direct_samp"] = std::to_string(devConfig->getDirectSampling());
    args["buffers"] = "6";
    args["buflen"] = "16384";
    SoapySDR::Device *device = SoapySDR::Device::make(args);
    
    SoapySDR::Stream *stream = device->setupStream(SOAPY_SDR_RX,"CF32", std::vector<size_t>(), dev->getStreamArgs());
    
    device->activateStream(stream);
    device->setSampleRate(SOAPY_SDR_RX,0,sampleRate.load());
    device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency - offset.load());
    if (hasPPM) {
        device->setFrequency(SOAPY_SDR_RX,0,"CORR",ppm);
    }
    
    device->setGainMode(SOAPY_SDR_RX,0,true);
    hasHardwareDC = dev->hasHardwareDC();
    
    numElems = getOptimalElementCount(sampleRate.load(), 60);
    
    void *buffs[1];
    buffs[0] = malloc(numElems * 2 * sizeof(float));

    int flags;
    long long timeNs;

    ReBuffer<SDRThreadIQData> buffers;

    SDRThreadIQDataQueue* iqDataOutQueue = (SDRThreadIQDataQueue*) getOutputQueue("IQDataOutput");
    SDRThreadCommandQueue* cmdQueue = (SDRThreadCommandQueue*) getInputQueue("SDRCommandQueue");

    while (!terminated) {
        if (!cmdQueue->empty()) {
            bool freq_changed = false;
            bool offset_changed = false;
            bool rate_changed = false;
            bool device_changed = false;
            bool ppm_changed = false;
            bool direct_sampling_changed = false;
            long long new_freq = frequency;
            long long new_offset = offset.load();
            long long new_rate = sampleRate.load();
            int new_device = deviceId;
            int new_ppm = ppm;
            
            while (!cmdQueue->empty()) {
                SDRThreadCommand command;
                cmdQueue->pop(command);
                
                switch (command.cmd) {
                    case SDRThreadCommand::SDR_THREAD_CMD_TUNE:
                        freq_changed = true;
                        new_freq = command.llong_value;
                        if (new_freq < sampleRate.load() / 2) {
                            new_freq = sampleRate.load() / 2;
                        }
                        break;
                    case SDRThreadCommand::SDR_THREAD_CMD_SET_OFFSET:
                        offset_changed = true;
                        new_offset = command.llong_value;
                        std::cout << "Set offset: " << new_offset << std::endl;
                        break;
                    case SDRThreadCommand::SDR_THREAD_CMD_SET_SAMPLERATE:
                        rate_changed = true;
                        new_rate = command.llong_value;
                        std::cout << "Set sample rate: " << new_rate << std::endl;
                        break;
                    case SDRThreadCommand::SDR_THREAD_CMD_SET_DEVICE:
                        device_changed = true;
                        new_device = (int) command.llong_value;
                        std::cout << "Set device: " << new_device << std::endl;
                        break;
                    case SDRThreadCommand::SDR_THREAD_CMD_SET_PPM:
                        ppm_changed = true;
                        new_ppm = (int) command.llong_value;
                        //std::cout << "Set PPM: " << new_ppm << std::endl;
                        break;
                    case SDRThreadCommand::SDR_THREAD_CMD_SET_DIRECT_SAMPLING:
                        direct_sampling_mode = (int)command.llong_value;
                        direct_sampling_changed = true;
                        break;
                    default:
                        break;
                }
            }
            
            if (device_changed) {
                device->deactivateStream(stream);
                device->closeStream(stream);
                SoapySDR::Device::unmake(device);
                
                deviceId = new_device;
                dev = devs[deviceId];
                device_changed = false;
                
                SoapySDR::Kwargs args = dev->getDeviceArgs();
                args["direct_samp"] = std::to_string(devConfig->getDirectSampling());
                args["buffers"] = "6";
                args["buflen"] = "16384";

                device = SoapySDR::Device::make(args);
                
                device->setSampleRate(SOAPY_SDR_RX,0,sampleRate.load());
                device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency - offset.load());
                if (hasPPM) {
                    device->setFrequency(SOAPY_SDR_RX,0,"CORR",ppm);
                }
                device->setGainMode(SOAPY_SDR_RX,0, true);
                hasHardwareDC = dev->hasHardwareDC();
                if (hasHardwareDC) {
                    device->setDCOffsetMode(SOAPY_SDR_RX, 0, true);
                    std::cout << "Hardware DC offset support detected; internal DC offset compensation disabled." << std::endl;
                }

                SoapySDR::Stream *stream = device->setupStream(SOAPY_SDR_RX,"CF32", std::vector<size_t>(), dev->getStreamArgs());
                device->activateStream(stream);
                
            }

            if (offset_changed) {
                if (!freq_changed) {
                    new_freq = frequency;
                    freq_changed = true;
                }
                offset.store(new_offset);
            }
            if (rate_changed) {
                device->setSampleRate(SOAPY_SDR_RX,0,new_rate);
                sampleRate.store(device->getSampleRate(SOAPY_SDR_RX,0));
                
                numElems = getOptimalElementCount(sampleRate.load(), 60);
                free(buffs[0]);
                buffs[0] = malloc(numElems * 2 * sizeof(float));
            }
            if (freq_changed) {
                frequency = new_freq;
                device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency - offset.load());
            }
            if (ppm_changed && hasPPM) {
                ppm = new_ppm;
                device->setFrequency(SOAPY_SDR_RX,0,"CORR",ppm);
            }
            if (direct_sampling_changed) {
//                rtlsdr_set_direct_sampling(dev, direct_sampling_mode);
            }
        }


        SDRThreadIQData *dataOut = buffers.getBuffer();
        if (dataOut->data.size() != numElems * 2) {
            dataOut->data.resize(numElems * 2);
        }

        int n_read = 0;
        while (n_read != numElems) {
            int n_stream_read = device->readStream(stream, buffs, numElems-n_read, flags, timeNs);
            if (n_stream_read > 0) {
                memcpy(&dataOut->data[n_read * 2], buffs[0], n_stream_read * sizeof(float) * 2);
                n_read += n_stream_read;
            } else {
                dataOut->data.resize(n_read);
                break;
            }
        }
        
//        std::cout << n_read << std::endl;

        if (n_read > 0) {
            dataOut->setRefCount(1);
            dataOut->frequency = frequency;
            dataOut->sampleRate = sampleRate.load();
            dataOut->dcCorrected = hasHardwareDC;
        
            if (iqDataOutQueue != NULL) {
                iqDataOutQueue->push(dataOut);
            } else {
                dataOut->setRefCount(0);
            }
        } else {
            dataOut->setRefCount(0);
        }
    }
    device->deactivateStream(stream);
    device->closeStream(stream);
    SoapySDR::Device::unmake(device);
    free(buffs[0]);

    buffers.purge();
    std::cout << "SDR thread done." << std::endl;
}


int SDRThread::getDeviceId() const {
    return deviceId.load();
}

void SDRThread::setDeviceId(int deviceId) {
    this->deviceId.store(deviceId);
}

int SDRThread::getOptimalElementCount(long long sampleRate, int fps) {
    int elemCount = (int)floor((double)sampleRate/(double)fps);
    elemCount = int(ceil((double)elemCount/512.0)*512.0);
    std::cout << "calculated optimal element count of " << elemCount << std::endl;
    return elemCount;
}

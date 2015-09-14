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
    std::map<std::string, int> deviceIndexes;
    
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
//            else if (it->first == dev->getDriver()) {
//                dev->setIndex(std::stoi(it->second.c_str()));
//            }
        }
        
        dev->setIndex(deviceIndexes[dev->getDriver()]);
        deviceIndexes[dev->getDriver()]++;
        
        std::cout << "Make device " << dev->getDeviceArgs() << std::endl;
        try {
            SoapySDR::Device *device = SoapySDR::Device::make(dev->getDeviceArgs());
            SoapySDR::Kwargs info = device->getHardwareInfo();
            for (SoapySDR::Kwargs::const_iterator it = info.begin(); it != info.end(); ++it) {
                std::cout << "  " << it->first << "=" << it->second << std::endl;
                if (it->first == "hardware") {
                    dev->setHardware(it->second);
                }
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
    int direct_sampling_mode = devConfig->getDirectSampling();;
//    int buf_size = BUF_SIZE;
    offset.store(devConfig->getOffset());
    wxGetApp().setSwapIQ(devConfig->getIQSwap());

    
    SDRDeviceInfo *dev = devs[deviceId];
    std::vector<SoapySDR::Kwargs> dev_results = SoapySDR::Device::enumerate(dev->getDeviceArgs());
    SoapySDR::Device *device = SoapySDR::Device::make(dev_results[dev->getIndex()]);

    device->setSampleRate(SOAPY_SDR_RX,0,sampleRate.load());
    device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency - offset.load());
    device->setFrequency(SOAPY_SDR_RX,0,"CORR",ppm);
    device->setGainMode(SOAPY_SDR_RX,0, true);
    
    SoapySDR::Stream *stream = device->setupStream(SOAPY_SDR_RX,"CF32");
    device->activateStream(stream);
    
    void *buffs[1];

    buffs[0] = malloc(BUF_SIZE * sizeof(float));

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
                
                SDRDeviceInfo *dev = devs[new_device];
                std::vector<SoapySDR::Kwargs> dev_results = SoapySDR::Device::enumerate(dev->getDeviceArgs());
                device = SoapySDR::Device::make(dev_results[dev->getIndex()]);
                
                device->setSampleRate(SOAPY_SDR_RX,0,sampleRate.load());
                device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency - offset.load());
                device->setFrequency(SOAPY_SDR_RX,0,"CORR",ppm);
                device->setGainMode(SOAPY_SDR_RX,0, true);

                SoapySDR::Stream *stream = device->setupStream(SOAPY_SDR_RX,"CF32");
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
            }
            if (freq_changed) {
                frequency = new_freq;
                device->setFrequency(SOAPY_SDR_RX,0,"RF",frequency - offset.load());
            }
            if (ppm_changed) {
                ppm = new_ppm;
                device->setFrequency(SOAPY_SDR_RX,0,"CORR",ppm);
            }
            if (direct_sampling_changed) {
//                rtlsdr_set_direct_sampling(dev, direct_sampling_mode);
            }
        }

        
        int n_read = device->readStream(stream, buffs, BUF_SIZE/2, flags, timeNs, 99999999);
        
//        std::cout << n_read << ", " << timeNs << std::endl;

        SDRThreadIQData *dataOut = buffers.getBuffer();
        
        dataOut->setRefCount(1);
        dataOut->frequency = frequency;
        dataOut->sampleRate = sampleRate.load();
        
        dataOut->data.resize(n_read * 2);
        memcpy(&dataOut->data[0],buffs[0],n_read * sizeof(float) * 2);
        
        if (iqDataOutQueue != NULL) {
            iqDataOutQueue->push(dataOut);
        } else {
            dataOut->setRefCount(0);
        }

    }
    device->deactivateStream(stream);
    device->closeStream(stream);
    SoapySDR::Device::unmake(device);
    
/*

 
    rtlsdr_open(&dev, deviceId);
    rtlsdr_set_sample_rate(dev, sampleRate.load());
    rtlsdr_set_center_freq(dev, frequency - offset.load());
    rtlsdr_set_freq_correction(dev, ppm);
    rtlsdr_set_agc_mode(dev, 1);
    rtlsdr_set_offset_tuning(dev, 0);
    rtlsdr_reset_buffer(dev);

//    sampleRate = rtlsdr_get_sample_rate(dev);

    std::cout << "Sample Rate is: " << sampleRate.load() << std::endl;

    int n_read;
    double seconds = 0.0;

    std::cout << "SDR thread started.." << std::endl;

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
//                    std::cout << "Set frequency: " << new_freq << std::endl;
                    break;
                case SDRThreadCommand::SDR_THREAD_CMD_SET_OFFSET:
                    offset_changed = true;
                    new_offset = command.llong_value;
                    std::cout << "Set offset: " << new_offset << std::endl;
                    break;
                case SDRThreadCommand::SDR_THREAD_CMD_SET_SAMPLERATE:
                    rate_changed = true;
                    new_rate = command.llong_value;
                    if (new_rate <= 250000) {
                        buf_size = BUF_SIZE/4;
                    } else if (new_rate < 1500000) {
                        buf_size = BUF_SIZE/2;
                    } else {
                        buf_size = BUF_SIZE;
                    }
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
                rtlsdr_close(dev);
                rtlsdr_open(&dev, new_device);
                rtlsdr_set_sample_rate(dev, sampleRate.load());
                rtlsdr_set_center_freq(dev, frequency - offset.load());
                rtlsdr_set_freq_correction(dev, ppm);
                rtlsdr_set_agc_mode(dev, 1);
                rtlsdr_set_offset_tuning(dev, 0);
                rtlsdr_set_direct_sampling(dev, direct_sampling_mode);
                rtlsdr_reset_buffer(dev);
            }
            if (offset_changed) {
                if (!freq_changed) {
                    new_freq = frequency;
                    freq_changed = true;
                }
                offset.store(new_offset);
            }
            if (rate_changed) {
                rtlsdr_set_sample_rate(dev, new_rate);
                rtlsdr_reset_buffer(dev);
                sampleRate.store(rtlsdr_get_sample_rate(dev));
            }
            if (freq_changed) {
                frequency = new_freq;
                rtlsdr_set_center_freq(dev, frequency - offset.load());
            }
            if (ppm_changed) {
                ppm = new_ppm;
                rtlsdr_set_freq_correction(dev, ppm);
            }
            if (direct_sampling_changed) {
                rtlsdr_set_direct_sampling(dev, direct_sampling_mode);
            }
        }

        rtlsdr_read_sync(dev, buf, buf_size, &n_read);

        SDRThreadIQData *dataOut = buffers.getBuffer();

//        std::lock_guard < std::mutex > lock(dataOut->m_mutex);
        dataOut->setRefCount(1);
        dataOut->frequency = frequency;
        dataOut->sampleRate = sampleRate.load();

        if (dataOut->data.capacity() < n_read) {
            dataOut->data.reserve(n_read);
        }

        if (dataOut->data.size() != n_read) {
            dataOut->data.resize(n_read);
        }

        memcpy(&dataOut->data[0], buf, n_read);

        double time_slice = (double) n_read / (double) sampleRate.load();
        seconds += time_slice;

        if (iqDataOutQueue != NULL) {
            iqDataOutQueue->push(dataOut);
        }
    }

//     buffers.purge();
    */
    std::cout << "SDR thread done." << std::endl;
}


int SDRThread::getDeviceId() const {
    return deviceId.load();
}

void SDRThread::setDeviceId(int deviceId) {
    this->deviceId.store(deviceId);
}

#include "SDREnumerator.h"
#include "CubicSDRDefs.h"
#include <vector>
#include "CubicSDR.h"
#include <string>


std::vector<std::string> SDREnumerator::factories;
std::vector<std::string> SDREnumerator::modules;
std::vector<SDRDeviceInfo *> SDREnumerator::devs;


SDREnumerator::SDREnumerator() : IOThread() {
  
}

SDREnumerator::~SDREnumerator() {

}


std::vector<SDRDeviceInfo *> *SDREnumerator::enumerate_devices() {

    if (SDREnumerator::devs.size()) {
        return &SDREnumerator::devs;
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
    
    if (SDREnumerator::factories.size()) {
        SDREnumerator::factories.erase(SDREnumerator::factories.begin(), SDREnumerator::factories.end());
    }
    
    std::cout << "\tAvailable factories...";
    SoapySDR::FindFunctions factories = SoapySDR::Registry::listFindFunctions();
    for (SoapySDR::FindFunctions::const_iterator it = factories.begin(); it != factories.end(); ++it) {
        if (it != factories.begin()) {
            std::cout << ", ";
        }
        std::cout << it->first;
        SDREnumerator::factories.push_back(it->first);
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

        SDREnumerator::devs.push_back(dev);
    }
    if (results.empty()) {
        std::cout << "No devices found!" << std::endl;
    }
    std::cout << std::endl;

    return &SDREnumerator::devs;
}


void SDREnumerator::run() {

    std::cout << "SDR enumerator starting." << std::endl;
    terminated.store(false);
    
    std::cout << "Enumerator devices." << std::endl;
    SDREnumerator::enumerate_devices();

    std::cout << "Reporting enumeration complete." << std::endl;
    terminated.store(true);
    wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_DEVICES_READY, "Devices Ready.");
    
    std::cout << "SDR enumerator done." << std::endl;

}



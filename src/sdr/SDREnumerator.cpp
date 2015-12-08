#include "SDREnumerator.h"
#include "CubicSDRDefs.h"
#include <vector>
#include "CubicSDR.h"
#include <string>


std::vector<std::string> SDREnumerator::factories;
std::vector<std::string> SDREnumerator::modules;
std::vector<std::string> SDREnumerator::remotes;
std::map< std::string, std::vector<SDRDeviceInfo *> > SDREnumerator::devs;
bool SDREnumerator::soapy_initialized = false;
bool SDREnumerator::has_remote = false;

SDREnumerator::SDREnumerator() : IOThread() {
  
}

SDREnumerator::~SDREnumerator() {

}


std::vector<SDRDeviceInfo *> *SDREnumerator::enumerate_devices(std::string remoteAddr, bool noInit) {

    if (SDREnumerator::devs[remoteAddr].size()) {
        return &SDREnumerator::devs[remoteAddr];
    }
    
    if (noInit) {
        return NULL;
    }
    
    if (!soapy_initialized) {
        std::cout << "SoapySDR init.." << std::endl;
        std::cout << "\tAPI Version: v" << SoapySDR::getAPIVersion() << std::endl;
        std::cout << "\tABI Version: v" << SoapySDR::getABIVersion() << std::endl;
        std::cout << "\tInstall root: " << SoapySDR::getRootPath() << std::endl;
        
        std::cout << "\tLoading modules... " << std::endl;
        
        std::string userModPath = wxGetApp().getModulePath();
        
        if (userModPath != "") {
            wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, "Loading SoapySDR modules from " + userModPath + "..");
            std::vector<std::string> localMods = SoapySDR::listModules(userModPath);
            for (std::vector<std::string>::iterator mods_i = localMods.begin(); mods_i != localMods.end(); mods_i++) {
                wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, "Initializing user specified SoapySDR module " + (*mods_i) + "..");
                std::cout << "Initializing user specified SoapySDR module " << (*mods_i) <<  ".." << std::endl;
                SoapySDR::loadModule(*mods_i);
            }
        } else {
            #ifdef BUNDLE_SOAPY_MODS
            bool localModPref = wxGetApp().getUseLocalMod();
            if (localModPref) {
                wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, "Loading SoapySDR modules..");
                std::cout << "Checking local system SoapySDR modules.." << std::flush;
                SoapySDR::loadModules();
            }

            wxFileName exePath = wxFileName(wxStandardPaths::Get().GetExecutablePath());
            std::vector<std::string> localMods = SoapySDR::listModules(exePath.GetPath().ToStdString() + "/modules/");
            for (std::vector<std::string>::iterator mods_i = localMods.begin(); mods_i != localMods.end(); mods_i++) {
                wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, "Initializing bundled SoapySDR module " + (*mods_i) + "..");
                std::cout << "Loading bundled SoapySDR module " << (*mods_i) <<  ".." << std::endl;
                SoapySDR::loadModule(*mods_i);
            }
        
            if (!localModPref) {
                wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, "Loading SoapySDR modules..");
                std::cout << "Checking system SoapySDR modules.." << std::flush;
                SoapySDR::loadModules();
            }
            #else
            wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, "Loading SoapySDR modules..");
            SoapySDR::loadModules();
            #endif

        }
//        modules = SoapySDR::listModules();
//        for (size_t i = 0; i < modules.size(); i++) {
//            std::cout << "\tModule found: " << modules[i] << std::endl;
//        }
//        if (modules.empty()) {
//            std::cout << "No modules found!" << std::endl;
//        }

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
            
            if (it->first == "remote") {
                has_remote = true;
            }
            SDREnumerator::factories.push_back(it->first);
        }
        if (factories.empty()) {
            std::cout << "No factories found!" << std::endl;
        }
        if ((factories.size() == 1) && factories.find("null") != factories.end()) {
            std::cout << "Just 'null' factory found." << std::endl;
            wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_FAILED, std::string("No modules available."));
        }
        std::cout << std::endl;
        soapy_initialized = true;
    }
    
    std::vector<SoapySDR::Kwargs> results;
    SoapySDR::Kwargs enumArgs;
    bool isRemote = false;
    
    if (remoteAddr.length()) {
        std::cout << "Enumerating remote address: " << remoteAddr << std::endl;
        enumArgs["driver"] = "remote";
        enumArgs["remote"] = remoteAddr;
        isRemote = true;
        
        results = SoapySDR::Device::enumerate(enumArgs);
    } else {
        results = SoapySDR::Device::enumerate();
    }
    
    if (isRemote) {
        wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("Opening remote server ") + remoteAddr + "..");
    }
    for (size_t i = 0; i < results.size(); i++) {
        SDRDeviceInfo *dev = new SDRDeviceInfo();
        
        SoapySDR::Kwargs deviceArgs = results[i];

        for (SoapySDR::Kwargs::const_iterator it = deviceArgs.begin(); it != deviceArgs.end(); ++it) {
            std::cout << "  " << it->first << " = " << it->second << std::endl;
            if (it->first == "driver") {
                dev->setDriver(it->second);
            } else if (it->first == "label" || it->first == "device") {
                dev->setName(it->second);
			}
        }
        
        DeviceConfig *cfg = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
        
        if (deviceArgs.count("remote")) {
            isRemote = true;
        } else {
            isRemote = false;
        }
        
        dev->setRemote(isRemote);
        
        std::cout << "Make device " << i << std::endl;
        try {
            SoapySDR::Device *device = SoapySDR::Device::make(deviceArgs);
            SoapySDR::Kwargs info = device->getHardwareInfo();
            for (SoapySDR::Kwargs::const_iterator it = info.begin(); it != info.end(); ++it) {
                std::cout << "  " << it->first << "=" << it->second << std::endl;
                if (it->first == "hardware") {
                    dev->setHardware(it->second);
                }
            }
            
            if (isRemote) {
                wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, "Querying remote " + remoteAddr + " device #" + std::to_string(i) + ": " + dev-> getName());
            } else {
                wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("Querying device #") + std::to_string(i) + ": " + dev->getName());
            }

            SoapySDR::ArgInfoList settingsInfo = device->getSettingInfo();
            
            ConfigSettings devSettings = cfg->getSettings();
            if (devSettings.size()) {
                for (ConfigSettings::const_iterator set_i = devSettings.begin(); set_i != devSettings.end(); set_i++) {
                    deviceArgs[set_i->first] = set_i->second;
                }
                for (int j = 0; j < settingsInfo.size(); j++) {
                    if (deviceArgs.find(settingsInfo[j].key) != deviceArgs.end()) {
                        settingsInfo[j].value = deviceArgs[settingsInfo[j].key];
                    }
                }
            }
            
            dev->setDeviceArgs(deviceArgs);
            dev->setSettingsInfo(settingsInfo);

            int numChan = device->getNumChannels(SOAPY_SDR_RX);
            for (int i = 0; i < numChan; i++) {
                SDRDeviceChannel *chan = new SDRDeviceChannel();

                SoapySDR::RangeList rfRange = device->getFrequencyRange(SOAPY_SDR_RX, i);
                double rfMin = rfRange[0].minimum();
                double rfMax = rfRange[rfRange.size()-1].maximum();
                chan->setChannel(i);
                chan->setFullDuplex(device->getFullDuplex(SOAPY_SDR_RX, i));
                chan->setRx(true);
                chan->setTx(false);
                chan->getRFRange().setLow(rfMin);
                chan->getRFRange().setHigh(rfMax);

                std::vector<std::string> freqs = device->listFrequencies(SOAPY_SDR_RX,i);
                if (std::find(freqs.begin(), freqs.end(), "CORR") != freqs.end()) {
                    chan->setCORR(true);
                } else {
                    chan->setCORR(false);
                }
                
                if (device->hasDCOffsetMode(SOAPY_SDR_RX, i)) {
                    chan->setHardwareDC(true);
                } else {
                    chan->setHardwareDC(false);
                }
                
                std::vector<double> rates = device->listSampleRates(SOAPY_SDR_RX, i);
                for (std::vector<double>::iterator i = rates.begin(); i != rates.end(); i++) {
                    chan->getSampleRates().push_back((long)(*i));
                }
                
                ConfigSettings devStreamOpts = cfg->getStreamOpts();
                if (devStreamOpts.size()) {
                    dev->setStreamArgs(devStreamOpts);
                }
                
                SoapySDR::ArgInfoList optArgs = device->getStreamArgsInfo(SOAPY_SDR_RX, i);

                if (devStreamOpts.size()) {
                    for (int j = 0, jMax = optArgs.size(); j < jMax; j++) {
                        if (devStreamOpts.find(optArgs[j].key) != devStreamOpts.end()) {
                            optArgs[j].value = devStreamOpts[optArgs[j].key];
                        }
                    }
                }
                chan->setStreamArgsInfo(optArgs);
                
                std::vector<std::string> gainNames = device->listGains(SOAPY_SDR_RX, i);
                
                for (std::vector<std::string>::iterator gname = gainNames.begin(); gname!= gainNames.end(); gname++) {
                    chan->addGain((*gname),device->getGainRange(SOAPY_SDR_RX, i, (*gname)));
                }
                
                dev->addChannel(chan);
            }
            
            SoapySDR::Device::unmake(device);
            
            dev->setAvailable(true);
        } catch (const std::exception &ex) {
            std::cerr << "Error making device: " << ex.what() << std::endl;
            wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("Error querying device #") + std::to_string(i));
            dev->setAvailable(false);
        }
        std::cout << std::endl;

        SDREnumerator::devs[remoteAddr].push_back(dev);
    }
    if (SDREnumerator::devs[remoteAddr].empty()) {
        wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, std::string("No devices found!"));
    }
    std::cout << std::endl;

    return &SDREnumerator::devs[remoteAddr];
}


void SDREnumerator::run() {

    std::cout << "SDR enumerator starting." << std::endl;
    terminated.store(false);

    wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, "Scanning local devices, please wait..");
    SDREnumerator::enumerate_devices("");

    if (remotes.size()) {
        std::vector<std::string>::iterator remote_i;
        for (remote_i = remotes.begin(); remote_i != remotes.end(); remote_i++) {
            wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_MESSAGE, "Scanning devices at " + (*remote_i) + ", please wait..");
            SDREnumerator::enumerate_devices(*remote_i);
        }
    }
    
    std::cout << "Reporting enumeration complete." << std::endl;
    terminated.store(true);
    wxGetApp().sdrEnumThreadNotify(SDREnumerator::SDR_ENUM_DEVICES_READY, "Finished scanning devices.");
    std::cout << "SDR enumerator done." << std::endl;

}



void SDREnumerator::addRemote(std::string remoteAddr) {
    std::vector<std::string>::iterator remote_i = std::find(remotes.begin(), remotes.end(), remoteAddr);
    
    if (remote_i != remotes.end()) {
        return;
    } else {
        remotes.push_back(remoteAddr);
    }
}

void SDREnumerator::removeRemote(std::string remoteAddr) {
    std::vector<std::string>::iterator remote_i = std::find(remotes.begin(), remotes.end(), remoteAddr);
    
    if (remote_i != remotes.end()) {
        if (devs.find(*remote_i) != devs.end()) {
            while (devs[*remote_i].size()) {
                SDRDeviceInfo *devRemove = devs[*remote_i].back();
                devs[*remote_i].pop_back();
                delete devRemove;
            }
        }
        remotes.erase(remote_i);
    } else {
        return;
    }
}

std::vector<std::string> &SDREnumerator::getRemotes() {
    return remotes;
}

bool SDREnumerator::hasRemoteModule() {
    return SDREnumerator::has_remote;
}

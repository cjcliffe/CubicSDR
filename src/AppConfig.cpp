#include "AppConfig.h"
#include "CubicSDR.h"

DeviceConfig::DeviceConfig() : deviceId("") {
	ppm.store(0);
	directSampling.store(false);
	offset.store(0);
}

DeviceConfig::DeviceConfig(std::string deviceId) : DeviceConfig() {
    this->deviceId = deviceId;
}

void DeviceConfig::setPPM(int ppm) {
    this->ppm.store(ppm);
}

int DeviceConfig::getPPM() {
    return ppm.load();
}

void DeviceConfig::setDirectSampling(int mode) {
    directSampling.store(mode);
}

int DeviceConfig::getDirectSampling() {
    return directSampling.load();
}

void DeviceConfig::setOffset(long long offset) {
    this->offset.store(offset);
}

long long DeviceConfig::getOffset() {
    return offset.load();
}

void DeviceConfig::setIQSwap(bool iqSwap) {
    this->iqSwap.store(iqSwap);
}

bool DeviceConfig::getIQSwap() {
    return iqSwap.load();
}

void DeviceConfig::setDeviceId(std::string deviceId) {
    busy_lock.lock();
    this->deviceId = deviceId;
    busy_lock.unlock();
}

std::string DeviceConfig::getDeviceId() {
    std::string tmp;

    busy_lock.lock();
    tmp = deviceId;
    busy_lock.unlock();

    return tmp;
}

void DeviceConfig::save(DataNode *node) {
    busy_lock.lock();
    *node->newChild("id") = deviceId;
    *node->newChild("ppm") = (int)ppm;
    *node->newChild("iq_swap") = iqSwap;
    *node->newChild("direct_sampling") = directSampling;
    *node->newChild("offset") = offset;
    busy_lock.unlock();
}

void DeviceConfig::load(DataNode *node) {
    busy_lock.lock();
    if (node->hasAnother("ppm")) {
        DataNode *ppm_node = node->getNext("ppm");
        int ppmValue = 0;
        ppm_node->element()->get(ppmValue);
        setPPM(ppmValue);
        std::cout << "Loaded PPM for device '" << deviceId << "' at " << ppmValue << "ppm" << std::endl;
    }
    if (node->hasAnother("iq_swap")) {
        DataNode *iq_swap_node = node->getNext("iq_swap");
        int iqSwapValue = 0;
        iq_swap_node->element()->get(iqSwapValue);
        setIQSwap(iqSwapValue?true:false);
        std::cout << "Loaded I/Q Swap for device '" << deviceId << "' as " << (iqSwapValue?"swapped":"not swapped") << std::endl;
    }
    if (node->hasAnother("direct_sampling")) {
        DataNode *direct_sampling_node = node->getNext("direct_sampling");
        int directSamplingValue = 0;
        direct_sampling_node->element()->get(directSamplingValue);
        setDirectSampling(directSamplingValue);
        std::cout << "Loaded Direct Sampling Mode for device '" << deviceId << "':  ";
        switch (directSamplingValue) {
            case 0:
                std::cout << "off" << std::endl;
                break;
            case 1:
                std::cout << "I-ADC" << std::endl;
                break;
            case 2:
                std::cout << "Q-ADC" << std::endl;
                break;
                
        }
    }
    if (node->hasAnother("offset")) {
        DataNode *offset_node = node->getNext("offset");
        long long offsetValue = 0;
        offset_node->element()->get(offsetValue);
        setOffset(offsetValue);
        std::cout << "Loaded offset for device '" << deviceId << "' at " << offsetValue << "Hz" << std::endl;
    }
    busy_lock.unlock();
}


DeviceConfig *AppConfig::getDevice(std::string deviceId) {
	if (deviceConfig.find(deviceId) == deviceConfig.end()) {
		deviceConfig[deviceId] = new DeviceConfig();
	}
    DeviceConfig *conf = deviceConfig[deviceId];
    conf->setDeviceId(deviceId);
    return conf;
}

std::string AppConfig::getConfigDir() {
    std::string dataDir = wxStandardPaths::Get().GetUserDataDir().ToStdString();

    bool mkStatus = false;

    if (!wxDir::Exists(dataDir)) {
        mkStatus = wxDir::Make(dataDir);
    } else {
        mkStatus = true;
    }

    if (!mkStatus) {
        std::cout << "Warning, unable to initialize user data directory." << std::endl;
    }

    return dataDir;
}


void AppConfig::setWindow(wxPoint winXY, wxSize winWH) {
    winX.store(winXY.x);
    winY.store(winXY.y);
    winW.store(winWH.x);
    winH.store(winWH.y);
}

wxRect *AppConfig::getWindow() {
    wxRect *r = NULL;
    if (winH.load() && winW.load()) {
        r = new wxRect(winX.load(),winY.load(),winW.load(),winH.load());
    }
    return r;
}


void AppConfig::setTheme(int themeId) {
    this->themeId.store(themeId);
}

int AppConfig::getTheme() {
    return themeId.load();
}


bool AppConfig::save() {
    DataTree cfg;

    cfg.rootNode()->setName("cubicsdr_config");
    
    if (winW.load() && winH.load()) {
        DataNode *window_node = cfg.rootNode()->newChild("window");
        
        *window_node->newChild("x") = winX.load();
        *window_node->newChild("y") = winY.load();
        *window_node->newChild("w") = winW.load();
        *window_node->newChild("h") = winH.load();

        *window_node->newChild("theme") = themeId.load();
    }
    
    DataNode *devices_node = cfg.rootNode()->newChild("devices");

    std::map<std::string, DeviceConfig *>::iterator device_config_i;
    for (device_config_i = deviceConfig.begin(); device_config_i != deviceConfig.end(); device_config_i++) {
        DataNode *device_node = devices_node->newChild("device");
        device_config_i->second->save(device_node);
    }

    
    std::string cfgFileDir = getConfigDir();

    wxFileName cfgFile = wxFileName(cfgFileDir, "config.xml");
    std::string cfgFileName = cfgFile.GetFullPath(wxPATH_NATIVE).ToStdString();

    if (!cfg.SaveToFileXML(cfgFileName)) {
        std::cout << "Error saving :: configuration file '" << cfgFileName << "' is not writable!" << std::endl;
        return false;
    }

    return true;
}

bool AppConfig::load() {
    DataTree cfg;
    std::string cfgFileDir = getConfigDir();

    wxFileName cfgFile = wxFileName(cfgFileDir, "config.xml");
    std::string cfgFileName = cfgFile.GetFullPath(wxPATH_NATIVE).ToStdString();

    if (!cfgFile.Exists()) {
        return true;
    }

    if (cfgFile.IsFileReadable()) {
        std::cout << "Loading:: configuration file '" << cfgFileName << "'" << std::endl;

        cfg.LoadFromFileXML(cfgFileName);
    } else {
        std::cout << "Error loading:: configuration file '" << cfgFileName << "' is not readable!" << std::endl;
        return false;
    }

    if (cfg.rootNode()->hasAnother("window")) {
        int x,y,w,h;
        
        DataNode *win_node = cfg.rootNode()->getNext("window");
        
        if (win_node->hasAnother("w") && win_node->hasAnother("h") && win_node->hasAnother("x") && win_node->hasAnother("y")) {
            win_node->getNext("x")->element()->get(x);
            win_node->getNext("y")->element()->get(y);
            win_node->getNext("w")->element()->get(w);
            win_node->getNext("h")->element()->get(h);
            
            winX.store(x);
            winY.store(y);
            winW.store(w);
            winH.store(h);
        }
        
        if (win_node->hasAnother("theme")) {
            int theme;
            win_node->getNext("theme")->element()->get(theme);
            themeId.store(theme);
        }

    }
    
    if (cfg.rootNode()->hasAnother("devices")) {
        DataNode *devices_node = cfg.rootNode()->getNext("devices");

        while (devices_node->hasAnother("device")) {
            DataNode *device_node = devices_node->getNext("device");
            if (device_node->hasAnother("id")) {
                std::string deviceId;
                device_node->getNext("id")->element()->get(deviceId);

                getDevice(deviceId)->load(device_node);
            }
        }
    }

    return true;
}

bool AppConfig::reset() {

    return true;
}

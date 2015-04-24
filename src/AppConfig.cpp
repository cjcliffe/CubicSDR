#include "AppConfig.h"

DeviceConfig::DeviceConfig() : ppm(0), deviceId("") {

}

DeviceConfig::DeviceConfig(std::string deviceId) : ppm(0) {
    this->deviceId = deviceId;
}

void DeviceConfig::setPPM(int ppm) {
    this->ppm = ppm;
}

int DeviceConfig::getPPM() {
    return ppm;
}

void DeviceConfig::setDeviceId(std::string deviceId) {
    this->deviceId = deviceId;
}

std::string DeviceConfig::getDeviceId() {
    return deviceId;
}

void DeviceConfig::save(DataNode *node) {
    node->newChild("id")->element()->set(deviceId);
    DataNode *ppm_node = node->newChild("ppm");
    ppm_node->element()->set((int)ppm);
}

void DeviceConfig::load(DataNode *node) {
    if (node->hasAnother("ppm")) {
        DataNode *ppm_node = node->getNext("ppm");
        int ppmValue = 0;
        ppm_node->element()->get(ppmValue);
        setPPM(ppmValue);
        std::cout << "Loaded PPM for device '" << deviceId << "' at " << ppmValue << "ppm" << std::endl;
    }
}


DeviceConfig *AppConfig::getDevice(std::string deviceId) {
    DeviceConfig *conf = &deviceConfig[deviceId];
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

bool AppConfig::save() {
    DataTree cfg;

    cfg.rootNode()->setName("cubicsdr_config");
    DataNode *devices_node = cfg.rootNode()->newChild("devices");

    std::map<std::string, DeviceConfig>::iterator device_config_i;
    for (device_config_i = deviceConfig.begin(); device_config_i != deviceConfig.end(); device_config_i++) {
        DataNode *device_node = devices_node->newChild("device");
        device_config_i->second.save(device_node);
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

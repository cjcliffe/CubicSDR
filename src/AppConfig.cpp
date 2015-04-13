#include "AppConfig.h"

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

void AppConfig::setPPM(std::string device_serial, int ppm) {
    device_ppm[device_serial] = ppm;
}

int AppConfig::getPPM(std::string device_serial) {
    if (device_ppm.find(device_serial) == device_ppm.end()) {
        return 0;
    }
    return device_ppm[device_serial];
}

bool AppConfig::save() {
    DataTree cfg;

    cfg.rootNode()->setName("cubicsdr_config");
    DataNode *ppm_data = cfg.rootNode()->newChild("ppm");

    std::map<std::string, int>::iterator device_ppm_i;
    for (device_ppm_i = device_ppm.begin(); device_ppm_i != device_ppm.end(); device_ppm_i++) {
        DataNode *ppm_ent = ppm_data->newChild("device");
        ppm_ent->newChild("id")->element()->set(device_ppm_i->first);
        ppm_ent->newChild("value")->element()->set((int)device_ppm_i->second);
    }

    std::string cfgFileDir = getConfigDir();

    wxFileName cfgFile = wxFileName(cfgFileDir, "cubicsdr.xml");
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

    wxFileName cfgFile = wxFileName(cfgFileDir, "cubicsdr.xml");
    std::string cfgFileName = cfgFile.GetFullPath(wxPATH_NATIVE).ToStdString();

    if (!cfgFile.Exists()) {
        return true;
    }

    if (cfgFile.IsFileReadable()) {
        cfg.LoadFromFileXML(cfgFileName);
    } else {
        std::cout << "Error loading:: configuration file '" << cfgFileName << "' is not readable!" << std::endl;
        return false;
    }

    if (cfg.rootNode()->hasAnother("ppm")) {
        device_ppm.clear();

        DataNode *ppm_data = cfg.rootNode()->getNext("ppm");

        while (ppm_data->hasAnother("device")) {
            DataNode *ppm_ent = ppm_data->getNext("device");

            if (ppm_ent->hasAnother("id") && ppm_ent->hasAnother("value")) {
                std::string deviceId(*ppm_ent->getNext("id"));
                int ppmValue = *ppm_ent->getNext("value");
                setPPM(deviceId, ppmValue);
                std::cout << "Loaded PPM for device '" << deviceId << "' at " << ppmValue << "ppm" << std::endl;
            }
        }
    }

    return true;
}

bool AppConfig::reset() {

    return true;
}

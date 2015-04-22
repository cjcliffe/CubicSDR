#pragma once

#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/filename.h>

#include "DataTree.h"


class DeviceConfig {
public:
    DeviceConfig();
    DeviceConfig(std::string deviceId);

    void setPPM(int ppm);
    int getPPM();

    void setDeviceId(std::string deviceId);
    std::string getDeviceId();

    void save(DataNode *node);
    void load(DataNode *node);

private:
    std::string deviceId;
    int ppm;
};

class AppConfig {
public:
    std::string getConfigDir();
    DeviceConfig *getDevice(std::string deviceId);

    bool save();
    bool load();
    bool reset();

private:
    std::map<std::string, DeviceConfig> deviceConfig;
};

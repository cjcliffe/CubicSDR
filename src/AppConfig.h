#pragma once

#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <atomic>
#include <mutex>

#include "DataTree.h"

class DeviceConfig {
public:
    DeviceConfig();
    DeviceConfig(std::string deviceId);

    void setPPM(int ppm);
    int getPPM();
    
    void setDirectSampling(int mode);
    int getDirectSampling();
    
    void setOffset(long long offset);
    long long getOffset();

    void setIQSwap(bool iqSwap);
    bool getIQSwap();
    
    void setDeviceId(std::string deviceId);
    std::string getDeviceId();

    void save(DataNode *node);
    void load(DataNode *node);

private:
    std::string deviceId;
    std::mutex busy_lock;

    std::atomic<int> ppm, directSampling;
    std::atomic<bool> iqSwap;
    std::atomic<long long> offset;
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

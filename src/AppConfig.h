#pragma once

#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/panel.h>
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

    std::atomic_int ppm, directSampling;
    std::atomic_bool iqSwap;
    std::atomic_llong offset;
};

class AppConfig {
public:
    AppConfig();
    std::string getConfigDir();
    DeviceConfig *getDevice(std::string deviceId);

    void setWindow(wxPoint winXY, wxSize winWH);
    wxRect *getWindow();
    
    void setWindowMaximized(bool max);
    bool getWindowMaximized();

    void setTheme(int themeId);
    int getTheme();

    void setSnap(long long snapVal);
    long long getSnap();

    void setConfigName(std::string configName);
    std::string getConfigFileName(bool ignoreName=false);
    bool save();
    bool load();
    bool reset();

private:
    std::string configName;
    std::map<std::string, DeviceConfig *> deviceConfig;
    std::atomic_int winX,winY,winW,winH;
    std::atomic_bool winMax;
    std::atomic_int themeId;
    std::atomic_llong snap;
};

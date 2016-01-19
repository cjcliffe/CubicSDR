#pragma once

#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/panel.h>
#include <atomic>
#include <mutex>

#include "DataTree.h"

typedef std::map<std::string, std::string> ConfigSettings;

class DeviceConfig {
public:
    DeviceConfig();
    DeviceConfig(std::string deviceId);

    void setPPM(int ppm);
    int getPPM();
    
    void setOffset(long long offset);
    long long getOffset();

    void setDeviceId(std::string deviceId);
    std::string getDeviceId();

    void setDeviceName(std::string deviceName);
    std::string getDeviceName();

    void setStreamOpts(ConfigSettings opts);
    ConfigSettings getStreamOpts();
    void setStreamOpt(std::string key, std::string value);
    std::string getStreamOpt(std::string key, std::string defaultValue);
    
    void setSettings(ConfigSettings settings);
    ConfigSettings getSettings();
    void setSetting(std::string key, std::string value);
    std::string getSetting(std::string key, std::string defaultValue);
    
    void setRigIF(int rigType, long long freq);
    long long getRigIF(int rigType);
    
    void save(DataNode *node);
    void load(DataNode *node);

private:
    std::string deviceId;
    std::string deviceName;
    std::mutex busy_lock;

    std::atomic_int ppm;
    std::atomic_llong offset;
    ConfigSettings streamOpts;
    std::map<std::string, std::string> settings;
    std::map<int, long long> rigIF;
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
    
    void setCenterFreq(long long freqVal);
    long long getCenterFreq();
    
    void setWaterfallLinesPerSec(int lps);
    int getWaterfallLinesPerSec();
    
    void setSpectrumAvgSpeed(float avgSpeed);
    float getSpectrumAvgSpeed();
    
#if USE_HAMLIB
    int getRigModel();
    void setRigModel(int rigModel);

    int getRigRate();
    void setRigRate(int rigRate);
    
    std::string getRigPort();
    void setRigPort(std::string rigPort);
#endif
    
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
    std::atomic_llong centerFreq;
    std::atomic_int waterfallLinesPerSec;
    std::atomic<float> spectrumAvgSpeed;
#if USE_HAMLIB
    std::atomic_int rigModel, rigRate;
    std::string rigPort;
#endif
};

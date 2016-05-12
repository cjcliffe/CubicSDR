#pragma once

#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/panel.h>
#include <atomic>
#include <mutex>

#include "DataTree.h"
#include "CubicSDRDefs.h"
#include "SDRDeviceInfo.h"

typedef std::map<std::string, std::string> ConfigSettings;
typedef std::map<std::string, float> ConfigGains;

class DeviceConfig {
public:
    DeviceConfig();
    DeviceConfig(std::string deviceId);

    void setPPM(int ppm);
    int getPPM();
    
    void setOffset(long long offset);
    long long getOffset();

    void setSampleRate(long srate);
    long getSampleRate();

    void setAGCMode(bool agcMode);
    bool getAGCMode();
    
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

    void setGains(ConfigGains gains);
    ConfigGains getGains();
    void setGain(std::string key, float value);
    float getGain(std::string key, float defaultValue);

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
    std::atomic_bool agcMode;
    std::atomic_long sampleRate;
    ConfigSettings streamOpts;
    ConfigGains gains;
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

    void setShowTips(bool show);
    bool getShowTips();

    void setLowPerfMode(bool lpm);
    bool getLowPerfMode();
    
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
    
    void setManualDevices(std::vector<SDRManualDef> manuals);
    std::vector<SDRManualDef> getManualDevices();
    
#if USE_HAMLIB
    int getRigModel();
    void setRigModel(int rigModel);

    int getRigRate();
    void setRigRate(int rigRate);
    
    std::string getRigPort();
    void setRigPort(std::string rigPort);
    
    void setRigControlMode(bool cMode);
    bool getRigControlMode();

    void setRigFollowMode(bool fMode);
    bool getRigFollowMode();
    
    void setRigCenterLock(bool cLock);
    bool getRigCenterLock();
    
    void setRigFollowModem(bool fMode);
    bool getRigFollowModem();

    void setRigEnabled(bool enabled);
    bool getRigEnabled();
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
    std::atomic_bool winMax, showTips, lowPerfMode;
    std::atomic_int themeId;
    std::atomic_llong snap;
    std::atomic_llong centerFreq;
    std::atomic_int waterfallLinesPerSec;
    std::atomic<float> spectrumAvgSpeed;
    std::vector<SDRManualDef> manualDevices;
#if USE_HAMLIB
    std::atomic_int rigModel, rigRate;
    std::string rigPort;
    std::atomic_bool rigEnabled, rigFollowMode, rigControlMode, rigCenterLock, rigFollowModem;
#endif
};

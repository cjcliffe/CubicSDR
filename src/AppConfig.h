// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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
    explicit DeviceConfig(std::string deviceId_in);

    void setPPM(int ppm_in);
    int getPPM();
    
    void setOffset(long long offset_in);
    long long getOffset();

    void setSampleRate(long sampleRate_in);
    long getSampleRate();

    void setAntennaName(const std::string& name);
    const std::string& getAntennaName();

    void setAGCMode(bool agcMode_in);
    bool getAGCMode();
    
    void setDeviceId(std::string deviceId_in);
    std::string getDeviceId();

    void setDeviceName(std::string deviceName_in);
    std::string getDeviceName();

    void setStreamOpts(ConfigSettings opts);
    ConfigSettings getStreamOpts();
    void setStreamOpt(const std::string& key, std::string value);
    std::string getStreamOpt(const std::string& key, std::string defaultValue);
    
    void setSettings(ConfigSettings settings_in);
    ConfigSettings getSettings();
    void setSetting(const std::string& key, std::string value);
    std::string getSetting(const std::string& key, std::string defaultValue);

    void setGains(ConfigGains gains_in);
    ConfigGains getGains();
    void setGain(const std::string& key, float value);
    float getGain(const std::string& key, float defaultValue);

    void setRigIF(int rigType, long long freq);
    long long getRigIF(int rigType);

    void save(DataNode *node);
    void load(DataNode *node);

private:
    std::string deviceId;
    std::string deviceName;

    std::mutex busy_lock;

    std::atomic_int ppm{};
    std::atomic_llong offset{};
    std::atomic_bool agcMode{};
    std::atomic_long sampleRate{};
    std::string antennaName;
    ConfigSettings streamOpts;
    ConfigGains gains;
    std::map<std::string, std::string> settings;
    std::map<int, long long> rigIF;
};

class AppConfig {
public:

    enum PerfModeEnum {
        PERF_LOW = 0,
        PERF_NORMAL = 1,
        PERF_HIGH = 2
    };


    AppConfig();
    std::string getConfigDir();
    DeviceConfig *getDevice(const std::string& deviceId);

    void setWindow(wxPoint winXY, wxSize winWH);
    wxRect *getWindow();
    
    void setWindowMaximized(bool max);
    bool getWindowMaximized();

    void setModemPropsCollapsed(bool collapse);
    bool getModemPropsCollapsed();

    void setShowTips(bool show);
    bool getShowTips();

    void setPerfMode(PerfModeEnum mode);
    PerfModeEnum getPerfMode();
    
    void setTheme(int themeId_in);
    int getTheme();

    void setFontScale(int fontScale_in);
    int getFontScale();

    void setSnap(long long snapVal);
    long long getSnap();
    
    void setCenterFreq(long long freqVal);
    long long getCenterFreq();
    
    void setWaterfallLinesPerSec(int lps);
    int getWaterfallLinesPerSec();
    
    void setSpectrumAvgSpeed(float avgSpeed);
    float getSpectrumAvgSpeed();
    
    void setDBOffset(int offset);
    int getDBOffset();
    
    void setManualDevices(std::vector<SDRManualDef> manuals);
    std::vector<SDRManualDef> getManualDevices();
    
    void setMainSplit(float value);
    float getMainSplit();
    
    void setVisSplit(float value);
    float getVisSplit();
    
    void setBookmarkSplit(float value);
    float getBookmarkSplit();
    
    void setBookmarksVisible(bool state);
    bool getBookmarksVisible();
    
	//Recording settings:
    void setRecordingPath(std::string recPath);
    std::string getRecordingPath();
	bool verifyRecordingPath();

	void setRecordingSquelchOption(int enumChoice);
	int getRecordingSquelchOption() const;
    
	void setRecordingFileTimeLimit(int nbSeconds);
	int getRecordingFileTimeLimit() const;
    
#if USE_HAMLIB
    int getRigModel();
    void setRigModel(int rigModel_in);

    int getRigRate();
    void setRigRate(int rigRate_in);
    
    std::string getRigPort();
    void setRigPort(std::string rigPort_in);
    
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
    
    void setConfigName(std::string configName_in);
    std::string getConfigFileName(bool ignoreName=false);
    bool save();
    bool load();
    bool reset();

private:
    std::string configName;
    std::map<std::string, DeviceConfig *> deviceConfig;
    std::atomic_int winX{},winY{},winW{},winH{};
    std::atomic_bool winMax{}, showTips{}, modemPropsCollapsed{};
    std::atomic_int themeId{};
    std::atomic_int fontScale{};
    std::atomic_llong snap{};
    std::atomic_llong centerFreq{};
    std::atomic_int waterfallLinesPerSec{};
    std::atomic<float> spectrumAvgSpeed{}, mainSplit{}, visSplit{}, bookmarkSplit{};
    std::atomic_int dbOffset{};
    std::vector<SDRManualDef> manualDevices;
    std::atomic_bool bookmarksVisible{};

    std::atomic<PerfModeEnum> perfMode{};

    std::string recordingPath;
	int recordingSquelchOption = 0;
	int recordingFileTimeLimitSeconds = 0;
#if USE_HAMLIB
    std::atomic_int rigModel{}, rigRate{};
    std::string rigPort;
    std::atomic_bool rigEnabled{}, rigFollowMode{}, rigControlMode{}, rigCenterLock{}, rigFollowModem{};
#endif
};

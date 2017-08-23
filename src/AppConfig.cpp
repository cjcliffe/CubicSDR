// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AppConfig.h"
#include "CubicSDR.h"

DeviceConfig::DeviceConfig() : deviceId("") {
	ppm.store(0);
	offset.store(0);
    agcMode.store(true);
    sampleRate.store(0);
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

void DeviceConfig::setOffset(long long offset) {
    this->offset.store(offset);
}

long long DeviceConfig::getOffset() {
    return offset.load();
}

void DeviceConfig::setSampleRate(long srate) {
    sampleRate.store(srate);
}

long DeviceConfig::getSampleRate() {
    return sampleRate.load();
}

void DeviceConfig::setAntennaName(const std::string& name) {
    antennaName = name;
}

const std::string& DeviceConfig::getAntennaName() {
    return antennaName;
}

void DeviceConfig::setAGCMode(bool agcMode) {
    this->agcMode.store(agcMode);
}

bool DeviceConfig::getAGCMode() {
    return agcMode.load();
}


void DeviceConfig::setDeviceId(std::string deviceId) {
    std::lock_guard < std::mutex > lock(busy_lock);
    this->deviceId = deviceId;
    
}

std::string DeviceConfig::getDeviceId() {
    std::string tmp;

    std::lock_guard < std::mutex > lock(busy_lock);
    tmp = deviceId;
    

    return tmp;
}

void DeviceConfig::setDeviceName(std::string deviceName) {
    std::lock_guard < std::mutex > lock(busy_lock);
    this->deviceName = deviceName;
   
}

std::string DeviceConfig::getDeviceName() {
    std::string tmp;
    
    std::lock_guard < std::mutex > lock(busy_lock);
    tmp = (deviceName=="")?deviceId:deviceName;
   
    
    return tmp;
}

void DeviceConfig::save(DataNode *node) {

    std::lock_guard < std::mutex > lock(busy_lock);
    
    *node->newChild("id") = deviceId;
    *node->newChild("name") = deviceName;
    *node->newChild("ppm") = ppm.load();
    *node->newChild("offset") = offset.load();
    *node->newChild("sample_rate") = sampleRate.load();
    *node->newChild("agc_mode") = agcMode.load()?1:0;

    if (!antennaName.empty()) {
        *node->newChild("antenna") = antennaName;
    }

    if (streamOpts.size()) {
        DataNode *streamOptsNode = node->newChild("streamOpts");
        for (ConfigSettings::const_iterator opt_i = streamOpts.begin(); opt_i != streamOpts.end(); opt_i++) {
            *streamOptsNode->newChild(opt_i->first.c_str()) = opt_i->second;
        }
    }
    if (settings.size()) {
        DataNode *settingsNode = node->newChild("settings");
        for (ConfigSettings::const_iterator set_i = settings.begin(); set_i != settings.end(); set_i++) {
            *settingsNode->newChild(set_i->first.c_str()) = set_i->second;
        }
    }
    if (rigIF.size()) {
        DataNode *rigIFs = node->newChild("rig_ifs");
        for (std::map<int, long long>::const_iterator rigIF_i = rigIF.begin(); rigIF_i != rigIF.end(); rigIF_i++) {
            DataNode *ifNode = rigIFs->newChild("rig_if");
            *ifNode->newChild("model") = rigIF_i->first;
            *ifNode->newChild("sdr_if") = rigIF_i->second;
        }
    }
    if (gains.size()) {
        DataNode *gainsNode = node->newChild("gains");
        for (ConfigGains::const_iterator gain_i = gains.begin(); gain_i != gains.end(); gain_i++) {
            DataNode *gainNode = gainsNode->newChild("gain");
            *gainNode->newChild("id") = gain_i->first;
            *gainNode->newChild("value") = gain_i->second;
        }
    }
   
}

void DeviceConfig::load(DataNode *node) {
    std::lock_guard < std::mutex > lock(busy_lock);
    if (node->hasAnother("name")) {
        deviceName = node->getNext("name")->element()->toString();
    }
    if (node->hasAnother("ppm")) {
        DataNode *ppm_node = node->getNext("ppm");
        int ppmValue = 0;
        ppm_node->element()->get(ppmValue);
        setPPM(ppmValue);
    }
    if (node->hasAnother("offset")) {
        DataNode *offset_node = node->getNext("offset");
        long long offsetValue = 0;
        offset_node->element()->get(offsetValue);
        setOffset(offsetValue);
    }
    if (node->hasAnother("agc_mode")) {
        DataNode *agc_node = node->getNext("agc_mode");
        int agcModeValue = 0;
        agc_node->element()->get(agcModeValue);
        setAGCMode(agcModeValue?true:false);
    }
    if (node->hasAnother("sample_rate")) {
        DataNode *sample_rate_node = node->getNext("sample_rate");
        long sampleRateValue = 0;
        sample_rate_node->element()->get(sampleRateValue);
        setSampleRate(sampleRateValue);
    }
    if (node->hasAnother("antenna")) {
        DataNode *antenna_node = node->getNext("antenna");
        std::string antennaNameValue;
        antenna_node->element()->get(antennaNameValue);
        setAntennaName(antennaNameValue);
    }
    if (node->hasAnother("streamOpts")) {
        DataNode *streamOptsNode = node->getNext("streamOpts");
        for (int i = 0, iMax = streamOptsNode->numChildren(); i<iMax; i++) {
            DataNode *streamOptNode = streamOptsNode->child(i);
            std::string keyName = streamOptNode->getName();
            std::string strSettingValue = streamOptNode->element()->toString();
            
            if (keyName != "") {
                setStreamOpt(keyName, strSettingValue);
            }
        }
    }
    if (node->hasAnother("settings")) {
        DataNode *settingsNode = node->getNext("settings");
        for (int i = 0, iMax = settingsNode->numChildren(); i<iMax; i++) {
            DataNode *settingNode = settingsNode->child(i);
            std::string keyName = settingNode->getName();
            std::string strSettingValue = settingNode->element()->toString();
            
            if (keyName != "") {
                setSetting(keyName, strSettingValue);
            }
        }
    }
    if (node->hasAnother("rig_ifs")) {
        DataNode *rigIFNodes = node->getNext("rig_ifs");
        while (rigIFNodes->hasAnother("rig_if")) {
            DataNode *rigIFNode = rigIFNodes->getNext("rig_if");
            if (rigIFNode->hasAnother("model") && rigIFNode->hasAnother("sdr_if")) {
                int load_model;
                long long load_freq;
                
                rigIFNode->getNext("model")->element()->get(load_model);
                rigIFNode->getNext("sdr_if")->element()->get(load_freq);
                
                rigIF[load_model] = load_freq;
            }
        }
    }
    if (node->hasAnother("gains")) {
        DataNode *gainsNode = node->getNext("gains");
        while (gainsNode->hasAnother("gain")) {
            DataNode *gainNode = gainsNode->getNext("gain");
            std::string keyName;
            float fltSettingValue;
            
            gainNode->getNext("id")->element()->get(keyName);
            gainNode->getNext("value")->element()->get(fltSettingValue);

            if (keyName != "" && !(fltSettingValue!=fltSettingValue)) {
                setGain(keyName, fltSettingValue);
            }
        }
    }
   
}

void DeviceConfig::setStreamOpts(ConfigSettings opts) {
    streamOpts = opts;
}

ConfigSettings DeviceConfig::getStreamOpts() {
    return streamOpts;
}

void DeviceConfig::setStreamOpt(std::string key, std::string value) {
    streamOpts[key] = value;
}

std::string DeviceConfig::getStreamOpt(std::string key, std::string defaultValue) {
    if (streamOpts.find(key) == streamOpts.end()) {
        return defaultValue;
    }
    
    return streamOpts[key];
}

void DeviceConfig::setSettings(ConfigSettings settings) {
    this->settings = settings;
}

void DeviceConfig::setSetting(std::string key, std::string value) {
    this->settings[key] = value;
}

std::string DeviceConfig::getSetting(std::string key, std::string defaultValue) {
    if (settings.find(key) == settings.end()) {
        return defaultValue;
    }
    return settings[key];
}

ConfigSettings DeviceConfig::getSettings() {
    return settings;
}


void DeviceConfig::setGains(ConfigGains gains) {
    this->gains = gains;
}

ConfigGains DeviceConfig::getGains() {
    return gains;
}

void DeviceConfig::setGain(std::string key, float value) {
    gains[key] = value;
}

float DeviceConfig::getGain(std::string key, float defaultValue) {
    if (gains.find(key) != gains.end()) {
        return gains[key];
    }
    return defaultValue;
}


void DeviceConfig::setRigIF(int rigType, long long freq) {
    rigIF[rigType] = freq;
}

long long DeviceConfig::getRigIF(int rigType) {
    if (rigIF.find(rigType) != rigIF.end()) {
        return rigIF[rigType];
    }
    return 0;
}

AppConfig::AppConfig() : configName("") {
    winX.store(0);
    winY.store(0);
    winW.store(0);
    winH.store(0);
    winMax.store(false);
    showTips.store(true);
    lowPerfMode.store(false);
    themeId.store(0);
    fontScale.store(0);
    snap.store(1);
    centerFreq.store(100000000);
    waterfallLinesPerSec.store(DEFAULT_WATERFALL_LPS);
    spectrumAvgSpeed.store(0.65f);
    dbOffset.store(0);
    modemPropsCollapsed.store(false);
    mainSplit = -1;
    visSplit = -1;
    bookmarkSplit = 200;
#ifdef CUBICSDR_DEFAULT_HIDE_BOOKMARKS
    bookmarksVisible.store(false);
#else
    bookmarksVisible.store(true);
#endif
    
#ifdef USE_HAMLIB
    rigEnabled.store(false);
    rigModel.store(1);
    rigRate.store(57600);
    rigPort = "/dev/ttyUSB0";
    rigControlMode.store(true);
    rigFollowMode.store(true);
#endif
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

void AppConfig::setWindowMaximized(bool max) {
    winMax.store(max);
}

bool AppConfig::getWindowMaximized() {
    return winMax.load();
}

void AppConfig::setModemPropsCollapsed(bool collapse) {
    modemPropsCollapsed.store(collapse);
}

bool AppConfig::getModemPropsCollapsed() {
    return modemPropsCollapsed.load();
}

void AppConfig::setShowTips(bool show) {
    showTips.store(show);
}

bool AppConfig::getShowTips() {
    return showTips.load();
}

void AppConfig::setLowPerfMode(bool show) {
    lowPerfMode.store(show);
}

bool AppConfig::getLowPerfMode() {
    return lowPerfMode.load();
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

void AppConfig::setFontScale(int fontScale) {
    this->fontScale.store(fontScale);
}

int AppConfig::getFontScale() {
    return fontScale.load();
}


void AppConfig::setSnap(long long snapVal) {
    this->snap.store(snapVal);
}

long long AppConfig::getSnap() {
    return snap.load();
}

void AppConfig::setCenterFreq(long long freqVal) {
    centerFreq.store(freqVal);
}

long long AppConfig::getCenterFreq() {
    return centerFreq.load();
}


void AppConfig::setWaterfallLinesPerSec(int lps) {
    waterfallLinesPerSec.store(lps);
}

int AppConfig::getWaterfallLinesPerSec() {
    return waterfallLinesPerSec.load();
}

void AppConfig::setSpectrumAvgSpeed(float avgSpeed) {
    spectrumAvgSpeed.store(avgSpeed);
}

float AppConfig::getSpectrumAvgSpeed() {
    return spectrumAvgSpeed.load();
}

void AppConfig::setDBOffset(int offset) {
    this->dbOffset.store(offset);
}

int AppConfig::getDBOffset() {
    return dbOffset.load();
}

void AppConfig::setManualDevices(std::vector<SDRManualDef> manuals) {
    manualDevices = manuals;
}

std::vector<SDRManualDef> AppConfig::getManualDevices() {
    return manualDevices;
}

void AppConfig::setMainSplit(float value) {
    mainSplit.store(value);
}

float AppConfig::getMainSplit() {
    return mainSplit.load();
}

void AppConfig::setVisSplit(float value) {
    visSplit.store(value);
}

float AppConfig::getVisSplit() {
    return visSplit.load();
}

void AppConfig::setBookmarkSplit(float value) {
    bookmarkSplit.store(value);
}

float AppConfig::getBookmarkSplit() {
    return bookmarkSplit.load();
}

void AppConfig::setBookmarksVisible(bool state) {
    bookmarksVisible.store(state);
}

bool AppConfig::getBookmarksVisible() {
    return bookmarksVisible.load();
}


void AppConfig::setConfigName(std::string configName) {
    this->configName = configName;
}

std::string AppConfig::getConfigFileName(bool ignoreName) {
    std::string cfgFileDir = getConfigDir();
    
    wxFileName cfgFile;
    if (configName.length() && !ignoreName) {
        std::string tempFn("config-");
        tempFn.append(configName);
        tempFn.append(".xml");
        cfgFile = wxFileName(cfgFileDir, tempFn);
    } else {
        cfgFile = wxFileName(cfgFileDir, "config.xml");
    }
    
    std::string cfgFileName = cfgFile.GetFullPath(wxPATH_NATIVE).ToStdString();
    
    return cfgFileName;
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

        *window_node->newChild("max") = winMax.load();
        *window_node->newChild("tips") = showTips.load();
        *window_node->newChild("low_perf_mode") = lowPerfMode.load();
        *window_node->newChild("theme") = themeId.load();
        *window_node->newChild("font_scale") = fontScale.load();
        *window_node->newChild("snap") = snap.load();
        *window_node->newChild("center_freq") = centerFreq.load();
        *window_node->newChild("waterfall_lps") = waterfallLinesPerSec.load();
        *window_node->newChild("spectrum_avg") = spectrumAvgSpeed.load();
        *window_node->newChild("modemprops_collapsed") = modemPropsCollapsed.load();;
        *window_node->newChild("db_offset") = dbOffset.load();

        *window_node->newChild("main_split") = mainSplit.load();
        *window_node->newChild("vis_split") = visSplit.load();
        *window_node->newChild("bookmark_split") = bookmarkSplit.load();
        *window_node->newChild("bookmark_visible") = bookmarksVisible.load();
    }
    
    DataNode *devices_node = cfg.rootNode()->newChild("devices");

    std::map<std::string, DeviceConfig *>::iterator device_config_i;
    for (device_config_i = deviceConfig.begin(); device_config_i != deviceConfig.end(); device_config_i++) {
        DataNode *device_node = devices_node->newChild("device");
        device_config_i->second->save(device_node);
    }

    if (manualDevices.size()) {
        DataNode *manual_node = cfg.rootNode()->newChild("manual_devices");
        for (std::vector<SDRManualDef>::const_iterator i = manualDevices.begin(); i != manualDevices.end(); i++) {
            DataNode *rig_node = manual_node->newChild("device");
            *rig_node->newChild("factory") = i->factory;
            *rig_node->newChild("params") = i->params;
        }
    }
    
#ifdef USE_HAMLIB
    DataNode *rig_node = cfg.rootNode()->newChild("rig");
    *rig_node->newChild("enabled") = rigEnabled.load()?1:0;
    *rig_node->newChild("model") = rigModel.load();
    *rig_node->newChild("rate") = rigRate.load();
    *rig_node->newChild("port") = rigPort;
    *rig_node->newChild("control") = rigControlMode.load()?1:0;
    *rig_node->newChild("follow") = rigFollowMode.load()?1:0;
    *rig_node->newChild("center_lock") = rigCenterLock.load()?1:0;
    *rig_node->newChild("follow_modem") = rigFollowModem.load()?1:0;
#endif
    
    std::string cfgFileName = getConfigFileName();
    
    if (!cfg.SaveToFileXML(cfgFileName)) {
        std::cout << "Error saving :: configuration file '" << cfgFileName << "' is not writable!" << std::endl;
        return false;
    }

    return true;
}

bool AppConfig::load() {
    DataTree cfg;
    std::string cfgFileDir = getConfigDir();

    std::string cfgFileName = getConfigFileName();
    wxFileName cfgFile = wxFileName(cfgFileName);

    if (!cfgFile.Exists()) {
        if (configName.length()) {
            wxFileName baseConfig = wxFileName(getConfigFileName(true));
            if (baseConfig.Exists()) {
                std::string baseConfigFileName = baseConfig.GetFullPath(wxPATH_NATIVE).ToStdString();
                std::cout << "Creating new configuration file '" << cfgFileName << "' by copying '" << baseConfigFileName << "'..";
                wxCopyFile(baseConfigFileName, cfgFileName);
                if (!cfgFile.Exists()) {
                    std::cout << "failed." << std::endl;
                    return true;
                }
                std::cout << "ok." << std::endl;
            } else {
                return true;
            }
        } else {
            return true;
        }
    }

    if (cfgFile.IsFileReadable()) {
        std::cout << "Loading:: configuration file '" << cfgFileName << "'" << std::endl;

        cfg.LoadFromFileXML(cfgFileName);
    } else {
        std::cout << "Error loading:: configuration file '" << cfgFileName << "' is not readable!" << std::endl;
        return false;
    }

    if (cfg.rootNode()->hasAnother("window")) {
        int x = 0 ,y = 0 ,w = 0 ,h = 0;
        int max = 0 ,tips = 0 ,lpm = 0 ,mpc = 0;
        
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
        
        if (win_node->hasAnother("max")) {
            win_node->getNext("max")->element()->get(max);
            winMax.store(max?true:false);
        }

        if (win_node->hasAnother("tips")) {
            win_node->getNext("tips")->element()->get(tips);
            showTips.store(tips?true:false);
        }

        if (win_node->hasAnother("low_perf_mode")) {
            win_node->getNext("low_perf_mode")->element()->get(lpm);
            lowPerfMode.store(lpm?true:false);
        }

        if (win_node->hasAnother("theme")) {
            int theme;
            win_node->getNext("theme")->element()->get(theme);
            themeId.store(theme);
        }

        if (win_node->hasAnother("font_scale")) {
            int fscale;
            win_node->getNext("font_scale")->element()->get(fscale);
            fontScale.store(fscale);
        }

        if (win_node->hasAnother("snap")) {
			long long snapVal;
			win_node->getNext("snap")->element()->get(snapVal);
			snap.store(snapVal);
		}

        if (win_node->hasAnother("center_freq")) {
            long long freqVal;
            win_node->getNext("center_freq")->element()->get(freqVal);
            centerFreq.store(freqVal);
        }

        if (win_node->hasAnother("waterfall_lps")) {
            int lpsVal;
            win_node->getNext("waterfall_lps")->element()->get(lpsVal);
            waterfallLinesPerSec.store(lpsVal);
        }
        
        if (win_node->hasAnother("spectrum_avg")) {
            float avgVal;
            win_node->getNext("spectrum_avg")->element()->get(avgVal);
            spectrumAvgSpeed.store(avgVal);
        }

        if (win_node->hasAnother("modemprops_collapsed")) {
            win_node->getNext("modemprops_collapsed")->element()->get(mpc);
            modemPropsCollapsed.store(mpc?true:false);
        }
        
        if (win_node->hasAnother("db_offset")) {
            DataNode *offset_node = win_node->getNext("db_offset");
            int offsetValue = 0;
            offset_node->element()->get(offsetValue);
            setDBOffset(offsetValue);
        }

        if (win_node->hasAnother("main_split")) {
            float gVal;
            win_node->getNext("main_split")->element()->get(gVal);
            mainSplit.store(gVal);
        }
        
        if (win_node->hasAnother("vis_split")) {
            float gVal;
            win_node->getNext("vis_split")->element()->get(gVal);
            visSplit.store(gVal);
        }
        
        if (win_node->hasAnother("bookmark_split")) {
            float gVal;
            win_node->getNext("bookmark_split")->element()->get(gVal);
            bookmarkSplit.store(gVal);
        }

        if (win_node->hasAnother("bookmark_visible")) {
            int bVal;
            win_node->getNext("bookmark_visible")->element()->get(bVal);
            bookmarksVisible.store(bVal);
        }
    }
    
    if (cfg.rootNode()->hasAnother("devices")) {
        DataNode *devices_node = cfg.rootNode()->getNext("devices");

        while (devices_node->hasAnother("device")) {
            DataNode *device_node = devices_node->getNext("device");
            if (device_node->hasAnother("id")) {
                std::string deviceId = device_node->getNext("id")->element()->toString();

                getDevice(deviceId)->load(device_node);
            }
        }
    }
    
    if (cfg.rootNode()->hasAnother("manual_devices")) {
        DataNode *manuals_node = cfg.rootNode()->getNext("manual_devices");
        
        while (manuals_node->hasAnother("device")) {
            DataNode *manual_node = manuals_node->getNext("device");
            if (manual_node->hasAnother("factory") && manual_node->hasAnother("params")) {
                SDRManualDef mdef;
                
                mdef.factory = manual_node->getNext("factory")->element()->toString();
                mdef.params = manual_node->getNext("params")->element()->toString();

                manualDevices.push_back(mdef);
            }
        }
    }
    
#ifdef USE_HAMLIB
    if (cfg.rootNode()->hasAnother("rig")) {
        DataNode *rig_node = cfg.rootNode()->getNext("rig");

        if (rig_node->hasAnother("enabled")) {
            int loadEnabled;
            rig_node->getNext("enabled")->element()->get(loadEnabled);
            rigEnabled.store(loadEnabled?true:false);
        }
        if (rig_node->hasAnother("model")) {
            int loadModel;
            rig_node->getNext("model")->element()->get(loadModel);
            rigModel.store(loadModel?loadModel:1);
        }
        if (rig_node->hasAnother("rate")) {
            int loadRate;
            rig_node->getNext("rate")->element()->get(loadRate);
            rigRate.store(loadRate?loadRate:57600);
        }
        if (rig_node->hasAnother("port")) {
            rigPort = rig_node->getNext("port")->element()->toString();
        }
        if (rig_node->hasAnother("control")) {
            int loadControl;
            rig_node->getNext("control")->element()->get(loadControl);
            rigControlMode.store(loadControl?true:false);
        }
        if (rig_node->hasAnother("follow")) {
            int loadFollow;
            rig_node->getNext("follow")->element()->get(loadFollow);
            rigFollowMode.store(loadFollow?true:false);
        }
        if (rig_node->hasAnother("center_lock")) {
            int loadCenterLock;
            rig_node->getNext("center_lock")->element()->get(loadCenterLock);
            rigCenterLock.store(loadCenterLock?true:false);
        }
        if (rig_node->hasAnother("follow_modem")) {
            int loadFollow;
            rig_node->getNext("follow_modem")->element()->get(loadFollow);
            rigFollowModem.store(loadFollow?true:false);
        }
    }
#endif


    return true;
}

bool AppConfig::reset() {

    return true;
}


#if USE_HAMLIB

int AppConfig::getRigModel() {
    return rigModel.load();
}

void AppConfig::setRigModel(int rigModel) {
    this->rigModel.store(rigModel);
}

int AppConfig::getRigRate() {
    return rigRate.load();
}

void AppConfig::setRigRate(int rigRate) {
    this->rigRate.store(rigRate);
}

std::string AppConfig::getRigPort() {
    return rigPort;
}

void AppConfig::setRigPort(std::string rigPort) {
    this->rigPort = rigPort;
}

void AppConfig::setRigControlMode(bool cMode) {
    rigControlMode.store(cMode);
}

bool AppConfig::getRigControlMode() {
    return rigControlMode.load();
}

void AppConfig::setRigFollowMode(bool fMode) {
    rigFollowMode.store(fMode);
}

bool AppConfig::getRigFollowMode() {
    return rigFollowMode.load();
}

void AppConfig::setRigCenterLock(bool cLock) {
    rigCenterLock.store(cLock);
}

bool AppConfig::getRigCenterLock() {
    return rigCenterLock.load();
}

void AppConfig::setRigFollowModem(bool fMode) {
    rigFollowModem.store(fMode);
}

bool AppConfig::getRigFollowModem() {
    return rigFollowModem.load();
}

void AppConfig::setRigEnabled(bool enabled) {
    rigEnabled.store(enabled);
}

bool AppConfig::getRigEnabled() {
    return rigEnabled.load();
}

#endif

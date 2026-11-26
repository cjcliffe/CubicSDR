// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <string>

#include "AudioThreadInput.h"
#include "SDREnumerator.h"
#include "SoapySDRThread.h"

class AppConfig;
class DemodulatorMgr;
class BookmarkMgr;
#ifdef USE_HAMLIB
class RigThread;
#endif

// UI-shell services consumed by the portable core (SDR, demod, DSP, audio).
// Implemented by the application object; the wx shell provides the
// implementation today so core code no longer depends on wxApp.
class AppServices {
public:
    virtual ~AppServices() = default;

    virtual AppConfig *getConfig() = 0;
    virtual DemodulatorMgr &getDemodMgr() = 0;
    virtual BookmarkMgr &getBookmarkMgr() = 0;

    virtual long long getFrequency() = 0;
    virtual void setFrequency(long long freq) = 0;
    virtual long long getSampleRate() = 0;
    virtual bool getSoloMode() = 0;

    virtual DemodulatorThreadOutputQueuePtr getAudioVisualQueue() = 0;

    virtual void sdrThreadNotify(SDRThread::SDRThreadState state, const std::string &message) = 0;
    virtual void sdrEnumThreadNotify(SDREnumerator::SDREnumState state, std::string message) = 0;
    virtual void notifyMainUIOfDeviceChange(bool forceRefreshOfGains) = 0;

    virtual std::string getModulePath() = 0;
    virtual bool getUseLocalMod() = 0;
    virtual std::string getExecutableDir() = 0;

    virtual bool getMainViewWindow(long long &centerHz, long long &spanHz) = 0;
    virtual bool isUserDemodBusy() = 0;
    virtual void notifyUpdateModemProperties() = 0;
#ifdef USE_HAMLIB
    virtual bool rigIsActive() = 0;
    virtual RigThread *getRigThread() = 0;
#endif

    virtual std::string getUserDataDir() = 0;
    virtual void showMessage(const std::string &title, const std::string &message) = 0;
    virtual void showStatusText(const std::string &text) = 0;
    virtual bool isShuttingDown() = 0;

    virtual void setSampleRate(long long rate) = 0;
    virtual void setSoloMode(bool solo) = 0;
    virtual void notifyDemodulatorsChanged() = 0;
    virtual SDRThread *getSDRThread() = 0;

    virtual bool getBookmarkBranchExpandState(const std::string &branch) = 0;
    virtual void setBookmarkBranchExpandState(const std::string &branch, bool expanded) = 0;
    virtual void refreshBookmarkActiveList() = 0;
    virtual void refreshBookmarkActiveLabels() = 0;
    virtual void refreshBookmarks() = 0;
    virtual void refreshBookmarks(const std::string &group) = 0;

    virtual bool getMainZoomedView(long long &centerFreq, long long &bandwidth) = 0;
    virtual void setMainZoomedView(long long centerFreq, long long bandwidth) = 0;
    virtual void resetMainZoomedView() = 0;
};

void SetAppServices(AppServices *services);
AppServices &GetAppServices();

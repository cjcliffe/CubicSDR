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
};

void SetAppServices(AppServices *services);
AppServices &GetAppServices();

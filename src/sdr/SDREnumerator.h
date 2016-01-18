#pragma once

#include <atomic>
#include <map>
#include <string>
#include "IOThread.h"
#include "SDRDeviceInfo.h"
#include "AppConfig.h"

#include <SoapySDR/Version.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Device.hpp>


class SDREnumerator: public IOThread {
private:

public:
    SDREnumerator();
    ~SDREnumerator();
    enum SDREnumState { SDR_ENUM_DEVICES_READY, SDR_ENUM_MESSAGE, SDR_ENUM_TERMINATED, SDR_ENUM_FAILED };
    
    static std::vector<SDRDeviceInfo *> *enumerate_devices(std::string remoteAddr = "", bool noInit=false);

    void run();

    static void addRemote(std::string remoteAddr);
    static void removeRemote(std::string remoteAddr);
    static std::vector<std::string> &getRemotes();
    static bool hasRemoteModule();
    static void reset();
    
protected:
    static bool soapy_initialized, has_remote;
    static std::vector<std::string> factories;
    static std::vector<std::string> modules;
    static std::vector<std::string> remotes;
    static std::map< std::string, std::vector<SDRDeviceInfo *> > devs;
};

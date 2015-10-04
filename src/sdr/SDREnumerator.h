#pragma once

#include <atomic>
#include "ThreadQueue.h"
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
    enum SDREnumState { SDR_ENUM_DEVICES_READY, SDR_ENUM_TERMINATED, SDR_ENUM_FAILED };
    
    static std::vector<SDRDeviceInfo *> *enumerate_devices();

    void run();

protected:
    static std::vector<std::string> factories;
    static std::vector<std::string> modules;
    static std::vector<SDRDeviceInfo *> devs;
};

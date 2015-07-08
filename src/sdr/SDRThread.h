#pragma once

#include <atomic>

#include "rtl-sdr.h"

#include "ThreadQueue.h"
#include "DemodulatorMgr.h"

class SDRDeviceInfo {
public:
    SDRDeviceInfo() : name(""), serial(""), available(false) { }

    std::string getDeviceId() {
        std::string deviceId;

        deviceId.append(getName());
        deviceId.append(" :: ");
        deviceId.append(getSerial());

        return deviceId;
    }

    bool isAvailable() const {
        return available;
    }

    void setAvailable(bool available) {
        this->available = available;
    }

    const std::string& getName() const {
        return name;
    }

    void setName(const std::string& name) {
        this->name = name;
    }

    const std::string& getSerial() const {
        return serial;
    }

    void setSerial(const std::string& serial) {
        this->serial = serial;
    }

    const std::string& getTuner() const {
        return tuner;
    }

    void setTuner(const std::string& tuner) {
        this->tuner = tuner;
    }

    const std::string& getManufacturer() const {
        return manufacturer;
    }

    void setManufacturer(const std::string& manufacturer) {
        this->manufacturer = manufacturer;
    }

    const std::string& getProduct() const {
        return product;
    }

    void setProduct(const std::string& product) {
        this->product = product;
    }

private:
    std::string name;
    std::string serial;
    std::string product;
    std::string manufacturer;
    std::string tuner;
    bool available;
};

class SDRThreadCommand {
public:
    enum SDRThreadCommandEnum {
        SDR_THREAD_CMD_NULL, SDR_THREAD_CMD_TUNE, SDR_THREAD_CMD_SET_OFFSET, SDR_THREAD_CMD_SET_SAMPLERATE, SDR_THREAD_CMD_SET_PPM, SDR_THREAD_CMD_SET_DEVICE, SDR_THREAD_CMD_SET_DIRECT_SAMPLING
    };

    SDRThreadCommand() :
            cmd(SDR_THREAD_CMD_NULL), llong_value(0) {

    }

    SDRThreadCommand(SDRThreadCommandEnum cmd) :
            cmd(cmd), llong_value(0) {

    }

    SDRThreadCommandEnum cmd;
    long long llong_value;
};

class SDRThreadIQData: public ReferenceCounter {
public:
    long long frequency;
    long long sampleRate;
    std::vector<unsigned char> data;

    SDRThreadIQData() :
            frequency(0), sampleRate(DEFAULT_SAMPLE_RATE) {

    }

    SDRThreadIQData(long long bandwidth, long long frequency, std::vector<signed char> *data) :
            frequency(frequency), sampleRate(bandwidth) {

    }

    ~SDRThreadIQData() {

    }
};

typedef ThreadQueue<SDRThreadCommand> SDRThreadCommandQueue;
typedef ThreadQueue<SDRThreadIQData *> SDRThreadIQDataQueue;

class SDRThread {
public:
    rtlsdr_dev_t *dev;

    SDRThread(SDRThreadCommandQueue* pQueue);
    ~SDRThread();

    static int enumerate_rtl(std::vector<SDRDeviceInfo *> *devs);

    void threadMain();

    void setIQDataOutQueue(SDRThreadIQDataQueue* iqDataQueue) {
        iqDataOutQueue = iqDataQueue;
    }

    void terminate();

    int getDeviceId() const {
        return deviceId.load();
    }

    void setDeviceId(int deviceId) {
        this->deviceId.store(deviceId);
    }

protected:
    std::atomic<uint32_t> sampleRate;
    std::atomic<long long> offset;
    std::atomic<SDRThreadCommandQueue*> commandQueue;
    std::atomic<SDRThreadIQDataQueue*> iqDataOutQueue;

    std::atomic<bool> terminated;
    std::atomic<int> deviceId;
};

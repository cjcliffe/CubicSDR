// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "ThreadBlockingQueue.h"
#include "CubicSDRDefs.h"
#include "liquid/liquid.h"
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>

#include "IOThread.h"

class DemodulatorThread;

class DemodulatorThreadControlCommand {
public:
    enum DemodulatorThreadControlCommandEnum {
        DEMOD_THREAD_CMD_CTL_NULL, DEMOD_THREAD_CMD_CTL_SQUELCH_ON, DEMOD_THREAD_CMD_CTL_SQUELCH_OFF
    };

    DemodulatorThreadControlCommand() :
            cmd(DEMOD_THREAD_CMD_CTL_NULL) {
    }

    DemodulatorThreadControlCommandEnum cmd;
};

class DemodulatorThreadIQData {
public:
    long long frequency;
    long long sampleRate;
    std::vector<liquid_float_complex> data;
   

    DemodulatorThreadIQData() :
            frequency(0), sampleRate(0) {

    }

    DemodulatorThreadIQData & operator=(const DemodulatorThreadIQData &other) {
        frequency = other.frequency;
        sampleRate = other.sampleRate;
        data.assign(other.data.begin(), other.data.end());
        return *this;
    }

    virtual ~DemodulatorThreadIQData() = default;
};

class Modem;
class ModemKit;

class DemodulatorThreadPostIQData {
public:
    std::vector<liquid_float_complex> data;

    long long sampleRate;
    std::string modemName;
    std::string modemType;
    Modem *modem;
    ModemKit *modemKit;

    DemodulatorThreadPostIQData() :
            sampleRate(0), modem(nullptr), modemKit(nullptr) {

    }

    virtual ~DemodulatorThreadPostIQData() = default;
};

typedef std::shared_ptr<DemodulatorThreadIQData> DemodulatorThreadIQDataPtr;
typedef std::shared_ptr<DemodulatorThreadPostIQData> DemodulatorThreadPostIQDataPtr;

typedef ThreadBlockingQueue<DemodulatorThreadIQDataPtr> DemodulatorThreadInputQueue;
typedef ThreadBlockingQueue<DemodulatorThreadPostIQDataPtr> DemodulatorThreadPostInputQueue;
typedef ThreadBlockingQueue<DemodulatorThreadControlCommand> DemodulatorThreadControlCommandQueue;

typedef std::shared_ptr<DemodulatorThreadInputQueue> DemodulatorThreadInputQueuePtr;
typedef std::shared_ptr<DemodulatorThreadPostInputQueue> DemodulatorThreadPostInputQueuePtr;
typedef std::shared_ptr<DemodulatorThreadControlCommandQueue> DemodulatorThreadControlCommandQueuePtr;

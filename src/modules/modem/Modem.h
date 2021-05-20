// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "liquid/liquid.h"
#include "IOThread.h"
#include "AudioThread.h"
#include <cmath>
#include <atomic>
#include <memory>

#define MIN_BANDWIDTH 500

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

class ModemKit {
public:
    ModemKit() : sampleRate(0), audioSampleRate(0) {

    }

    long long sampleRate;
    int audioSampleRate;
};

class ModemIQData {
public:
    std::vector<liquid_float_complex> data;
    long long sampleRate;

    ModemIQData() : sampleRate(0) {

    }

    virtual ~ModemIQData() {

    }
};

typedef std::shared_ptr<ModemIQData> ModemIQDataPtr;

// Copy of SoapySDR::Range, original comments
class ModemRange
{
public:

    //! Create an empty range (0.0, 0.0)
    ModemRange(void);

    //! Create a min/max range
    ModemRange(const double minimum, const double maximum);

    //! Get the range minimum
    double minimum(void) const;

    //! Get the range maximum
    double maximum(void) const;

private:
    double _min, _max;
};

// Modified version of SoapySDR::ArgInfo, original comments
class ModemArgInfo
{
public:
    //! Default constructor
    ModemArgInfo(void);

    //! The key used to identify the argument (required)
    std::string key;

    /*!
     * The default value of the argument when not specified (required)
     * Numbers should use standard floating point and integer formats.
     * Boolean values should be represented as "true" and  "false".
     */
    std::string value;

    //! The displayable name of the argument (optional, use key if empty)
    std::string name;

    //! A brief description about the argument (optional)
    std::string description;

    //! The units of the argument: dB, Hz, etc (optional)
    std::string units;

    //! The data type of the argument (required)
    enum Type { BOOL, INT, FLOAT, STRING, PATH_DIR, PATH_FILE, COLOR } type;

    /*!
     * The range of possible numeric values (optional)
     * When specified, the argument should be restricted to this range.
     * The range is only applicable to numeric argument types.
     */
    ModemRange range;

    /*!
     * A discrete list of possible values (optional)
     * When specified, the argument should be restricted to this options set.
     */
    std::vector<std::string> options;

    /*!
     * A discrete list of displayable names for the enumerated options (optional)
     * When not specified, the option value itself can be used as a display name.
     */
    std::vector<std::string> optionNames;
};

typedef std::vector<ModemArgInfo> ModemArgInfoList;

class ModemBase {

};

typedef ModemBase *(*ModemFactoryFn)();


typedef std::map<std::string, ModemFactoryFn> ModemFactoryList;
typedef std::map<std::string, int> DefaultRatesList;

typedef std::map<std::string, std::string> ModemSettings;

class Modem : public ModemBase  {
public:
    static void addModemFactory(ModemFactoryFn, std::string modemName, int defaultRate);
    static ModemFactoryList getFactories();

    static Modem *makeModem(std::string modemName);
    static int getModemDefaultSampleRate(std::string modemName);

    virtual std::string getType() = 0;
    virtual std::string getName() = 0;

    Modem();
    virtual ~Modem();

    virtual ModemArgInfoList getSettings();
    virtual int getDefaultSampleRate();
    virtual void writeSetting(std::string setting, std::string value);
            void writeSettings(ModemSettings settings);
    virtual std::string readSetting(std::string setting);
            ModemSettings readSettings();

    virtual int checkSampleRate(long long sampleRate, int audioSampleRate) = 0;

    virtual ModemKit *buildKit(long long sampleRate, int audioSampleRate) = 0;
    virtual void disposeKit(ModemKit *kit) = 0;

    virtual void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) = 0;

    bool shouldRebuildKit();
    void rebuildKit();
    void clearRebuildKit();

    bool useSignalOutput();
    void useSignalOutput(bool useOutput);

protected:
    // We would like Modems to be able to inherit and add to common settings.
    ModemArgInfoList mArgInfoList;
private:
    static ModemFactoryList modemFactories;
    static DefaultRatesList modemDefaultRates;
    std::atomic_bool refreshKit, _useSignalOutput;
};

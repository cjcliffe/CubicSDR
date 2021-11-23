// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "Modem.h"

class ModemKitAnalog : public ModemKit {
public:
    ModemKitAnalog() : ModemKit(), audioResampler(nullptr), audioResampleRatio(0) {

    };

    msresamp_rrrf audioResampler;
    double audioResampleRatio;
};


class ModemAnalog : public Modem {
public:
    ModemAnalog();
    std::string getType() override;
    int checkSampleRate(long long sampleRate, int audioSampleRate) override;
    ModemKit *buildKit(long long sampleRate, int audioSampleRate) override;
    void disposeKit(ModemKit *kit) override;
    virtual void initOutputBuffers(ModemKitAnalog *akit, ModemIQData *input);
    virtual void buildAudioOutput(ModemKitAnalog *akit, AudioThreadInput *audioOut, bool autoGain);
    virtual std::vector<float> *getDemodOutputData();
    virtual std::vector<float> *getResampledOutputData();
protected:
    size_t             bufSize;
    std::vector<float> demodOutputData;
    std::vector<float> resampledOutputData;
};

// ModemAnalogVC provides a base class which comes with user settable volume
// control. The user may opt for the prior auto gain adjust or select a fixed
// user supplied audio gain in dB. The issue I have with the current auto gain
// is that it assures the noise will dominate if there is no signal.
//
// By all rights, mArgList should be a protected member of Modem.
class ModemAnalogVC : public ModemAnalog
{
public:
  ModemAnalogVC ();
  // Allow for volume control on any analog modem.
  ModemArgInfoList getSettings() { return mArgInfoList; }
  void writeSetting (std::string setting, std::string value);
  std::string readSetting (std::string setting);
protected:
  void applyGain (std::vector<float> &adata);
  bool               mAutoGain;
  float              mGain;
  float              aOutputCeil;
  float              aOutputCeilMA;
  float              aOutputCeilMAA;
};

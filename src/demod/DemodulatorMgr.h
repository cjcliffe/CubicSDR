// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>
#include <map>
#include <thread>

#include "DemodulatorInstance.h"

class DataNode;

class DemodulatorMgr {
public:
    DemodulatorMgr();
    ~DemodulatorMgr();

    DemodulatorInstancePtr newThread();
   
    //return snapshot-copy of the list purposefully
    std::vector<DemodulatorInstancePtr> getDemodulators();

    std::vector<DemodulatorInstancePtr> getOrderedDemodulators(bool actives = true);
    std::vector<DemodulatorInstancePtr> getDemodulatorsAt(long long freq, int bandwidth);
    
    DemodulatorInstancePtr getPreviousDemodulator(const DemodulatorInstancePtr& demod, bool actives = true);
    DemodulatorInstancePtr getNextDemodulator(const DemodulatorInstancePtr& demod, bool actives = true);
    DemodulatorInstancePtr getLastDemodulator();
    DemodulatorInstancePtr getFirstDemodulator();
    bool anyDemodulatorsAt(long long freq, int bandwidth);
    void deleteThread(const DemodulatorInstancePtr&);

    void terminateAll();

    void setActiveDemodulator(const DemodulatorInstancePtr& demod, bool temporary = true);

    //Dangerous: this is only intended by some internal classes,
    // and only set a pre-existing demod
    void setActiveDemodulatorByRawPointer(DemodulatorInstance* demod, bool temporary = true);

    DemodulatorInstancePtr getActiveContextModem();
    DemodulatorInstancePtr getCurrentModem();
    DemodulatorInstancePtr getLastDemodulatorWith(const std::string& type,
												const std::wstring& userLabel,
												long long frequency,
												int bandwidth);

    int getLastBandwidth() const;
    void setLastBandwidth(int lastBandwidth_in);

    std::string getLastDemodulatorType() const;
    void setLastDemodulatorType(std::string lastDemodType_in);

    float getLastGain() const;
    void setLastGain(float lastGain_in);

    bool getLastDeltaLock() const;
    void setLastDeltaLock(bool lock);

    float getLastSquelchLevel() const;
    void setLastSquelchLevel(float lastSquelch_in);

    bool isLastSquelchEnabled() const;
    void setLastSquelchEnabled(bool lastSquelchEnabled_in);
    
    bool isLastMuted() const;
    void setLastMuted(bool lastMuted_in);

    ModemSettings getLastModemSettings(const std::string&);
    void setLastModemSettings(const std::string&, ModemSettings);

    void updateLastState();
    
    void setOutputDevices(std::map<int,RtAudio::DeviceInfo> devs);
    std::map<int, RtAudio::DeviceInfo> getOutputDevices();
    void saveInstance(DataNode *node, const DemodulatorInstancePtr& inst);
	
    DemodulatorInstancePtr loadInstance(DataNode *node);


private:

    //utility method that attempts to decode node value as std::wstring, else as std::string, else 
    //return an empty string.
    static std::wstring getSafeWstringValue(DataNode* node);

    std::vector<DemodulatorInstancePtr> demods;
    
    DemodulatorInstancePtr activeContextModem;
    DemodulatorInstancePtr currentModem;
    DemodulatorInstancePtr activeVisualDemodulator;

    int lastBandwidth;
    std::string lastDemodType;
    bool lastDemodLock;
    bool lastSquelchEnabled;
    float lastSquelch;
    float lastGain;
    bool lastMuted;
    bool lastDeltaLock;
    
    //protects access to demods lists and such, need to be recursive
    //because of the usage of public re-entrant methods 
    std::recursive_mutex demods_busy;
   
    std::map<std::string, ModemSettings> lastModemSettings;
    std::map<int,RtAudio::DeviceInfo> outputDevices;
};

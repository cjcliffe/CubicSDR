// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <DemodulatorMgr.h>
#include <sstream>
#include <algorithm>
#include <string>
#include <sstream>
#include <algorithm>

#include "DemodulatorMgr.h"
#include "CubicSDR.h"

#if USE_HAMLIB
#include "RigThread.h"
#endif

#include "DataTree.h"
#include <wx/string.h>

bool demodFreqCompare (DemodulatorInstancePtr i, DemodulatorInstancePtr j) { return (i->getFrequency() < j->getFrequency()); }
bool inactiveCompare (DemodulatorInstancePtr i, DemodulatorInstancePtr j) { return (i->isActive() < j->isActive()); }

DemodulatorMgr::DemodulatorMgr() {

    lastBandwidth = DEFAULT_DEMOD_BW;
    lastDemodType = DEFAULT_DEMOD_TYPE;
    lastSquelchEnabled = false;
    lastSquelch = -100;
    lastGain = 1.0;
    lastMuted = false;
    lastDeltaLock = false;
}

DemodulatorMgr::~DemodulatorMgr() {
    terminateAll();
}

DemodulatorInstancePtr DemodulatorMgr::newThread() {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    
    //create a new instance of DemodulatorInstance here.
    DemodulatorInstancePtr newDemod = std::make_shared<DemodulatorInstance>();

    std::stringstream label;
    label << demods.size();
    newDemod->setLabel(label.str());
    
    demods.push_back(newDemod);
    
    return newDemod;
}

void DemodulatorMgr::terminateAll() {

    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    
    while (demods.size()) {

        DemodulatorInstancePtr d = demods.back();
        demods.pop_back();
        deleteThread(d);
    }
}

std::vector<DemodulatorInstancePtr> DemodulatorMgr::getDemodulators() {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    return demods;
}

std::vector<DemodulatorInstancePtr> DemodulatorMgr::getOrderedDemodulators(bool actives) {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    
    auto demods_ordered = demods;
    
    if (actives) {
        
        std::sort(demods_ordered.begin(), demods_ordered.end(), inactiveCompare);
        
        std::vector<DemodulatorInstancePtr>::iterator i;
        
        for (i = demods_ordered.begin(); i != demods_ordered.end(); i++) {
            if ((*i)->isActive()) {
                break;
            }
        }
        if (i == demods_ordered.end()) {
            demods_ordered.erase(demods_ordered.begin(), demods_ordered.end());
        } else if ((*i) != demods_ordered.front()) {
            demods_ordered.erase(demods_ordered.begin(), i);
        }
    }
    //if by chance they have the same frequency, keep their relative order
    std::stable_sort(demods_ordered.begin(), demods_ordered.end(), demodFreqCompare);
    return demods_ordered;
}

DemodulatorInstancePtr DemodulatorMgr::getPreviousDemodulator(DemodulatorInstancePtr demod, bool actives) {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    if (!getLastActiveDemodulator()) {
        return nullptr;
    }
    auto demods_ordered = getOrderedDemodulators(actives);
    auto p = std::find(demods_ordered.begin(), demods_ordered.end(), demod);

    if (p == demods_ordered.end()) {
        return nullptr;
    }
    if (*p == demods_ordered.front()) {
        return demods_ordered.back();
    }
    return *(--p);
}

DemodulatorInstancePtr DemodulatorMgr::getNextDemodulator(DemodulatorInstancePtr demod, bool actives) {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    if (!getLastActiveDemodulator()) {
        return nullptr;
    }
    auto demods_ordered = getOrderedDemodulators(actives);
    auto p = std::find(demods_ordered.begin(), demods_ordered.end(), demod);

    if (actives) {
        
    }
    if (p == demods_ordered.end()) {
        return nullptr;
    }
    if (*p == demods_ordered.back()) {
        return demods_ordered.front();
    }
    return *(++p);
}

DemodulatorInstancePtr DemodulatorMgr::getLastDemodulator() {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    
    return getOrderedDemodulators().back();
}

DemodulatorInstancePtr DemodulatorMgr::getFirstDemodulator() {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    
    return getOrderedDemodulators().front();
}

void DemodulatorMgr::deleteThread(DemodulatorInstancePtr demod) {
    
    std::lock_guard < std::recursive_mutex > lock(demods_busy);

    wxGetApp().getBookmarkMgr().addRecent(demod);
  
    auto i = std::find(demods.begin(), demods.end(), demod);

    if (activeDemodulator == demod) {
        activeDemodulator = nullptr;
    }
    if (lastActiveDemodulator == demod) {
        lastActiveDemodulator = nullptr;
    }
    if (activeVisualDemodulator == demod) {
        activeVisualDemodulator = nullptr;
    }

    if (i != demods.end()) {
        demods.erase(i);
    }

    //Ask for termination
    demod->setActive(false);
    demod->terminate();
}

std::vector<DemodulatorInstancePtr> DemodulatorMgr::getDemodulatorsAt(long long freq, int bandwidth) {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    
    std::vector<DemodulatorInstancePtr> foundDemods;

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        DemodulatorInstancePtr testDemod = demods[i];

        long long freqTest = testDemod->getFrequency();
        long long bandwidthTest = testDemod->getBandwidth();
        long long halfBandwidthTest = bandwidthTest / 2;

        long long halfBuffer = bandwidth / 2;

        if ((freq <= (freqTest + ((testDemod->getDemodulatorType() != "LSB")?halfBandwidthTest:0) + halfBuffer)) && (freq >= (freqTest - ((testDemod->getDemodulatorType() != "USB")?halfBandwidthTest:0) - halfBuffer))) {
            foundDemods.push_back(testDemod);
        }
    }

    return foundDemods;
}

bool DemodulatorMgr::anyDemodulatorsAt(long long freq, int bandwidth) {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        DemodulatorInstancePtr testDemod = demods[i];

        long long freqTest = testDemod->getFrequency();
        long long bandwidthTest = testDemod->getBandwidth();
        long long halfBandwidthTest = bandwidthTest / 2;
        
        long long halfBuffer = bandwidth / 2;
        
        if ((freq <= (freqTest + ((testDemod->getDemodulatorType() != "LSB")?halfBandwidthTest:0) + halfBuffer)) && (freq >= (freqTest - ((testDemod->getDemodulatorType() != "USB")?halfBandwidthTest:0) - halfBuffer))) {
           
            return true;
        }
    }

    return false;
}


void DemodulatorMgr::setActiveDemodulator(DemodulatorInstancePtr demod, bool temporary) {
    
    std::lock_guard < std::recursive_mutex > lock(demods_busy);
    
    if (!temporary) {
        if (activeDemodulator != nullptr) {
            lastActiveDemodulator = activeDemodulator;
            updateLastState();
        } else {
            lastActiveDemodulator = demod;
        }
        updateLastState();
#if USE_HAMLIB
        if (wxGetApp().rigIsActive() && wxGetApp().getRigThread()->getFollowModem() && lastActiveDemodulator) {
            wxGetApp().getRigThread()->setFrequency(lastActiveDemodulator->getFrequency(),true);
        }
#endif
        wxGetApp().getBookmarkMgr().updateActiveList();
    } 

    if (activeVisualDemodulator) {
        activeVisualDemodulator->setVisualOutputQueue(nullptr);
    }
    if (demod) {
        demod->setVisualOutputQueue(wxGetApp().getAudioVisualQueue());
        activeVisualDemodulator = demod;
    } else {
        DemodulatorInstancePtr last = getLastActiveDemodulator();
        if (last) {
            last->setVisualOutputQueue(wxGetApp().getAudioVisualQueue());
        }
        activeVisualDemodulator = last;
    }

    activeDemodulator = demod;

}

//Dangerous: this is only intended by some internal classes
void DemodulatorMgr::setActiveDemodulatorByRawPointer(DemodulatorInstance* demod, bool temporary) {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);

    for (auto existing_demod : demods) {

        if (existing_demod.get() == demod) {

            setActiveDemodulator(existing_demod, temporary);
            break;
        }
    }
}

DemodulatorInstancePtr DemodulatorMgr::getActiveDemodulator() {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);

    if (activeDemodulator && !activeDemodulator->isActive()) {
        activeDemodulator = getLastActiveDemodulator();
    }
    return activeDemodulator;
}

DemodulatorInstancePtr DemodulatorMgr::getLastActiveDemodulator() {
    return lastActiveDemodulator;
}

DemodulatorInstancePtr DemodulatorMgr::getLastDemodulatorWith(const std::string& type,
															const std::wstring& userLabel,
															long long frequency,
															int bandwidth) {
	std::lock_guard < std::recursive_mutex > lock(demods_busy);

	//backwards search: 
	for (auto it = demods.rbegin(); it != demods.rend(); it++) {

		if ((*it)->getDemodulatorType() == type &&
			(*it)->getDemodulatorUserLabel() == userLabel &&
			(*it)->getFrequency() == frequency &&
			(*it)->getBandwidth() == bandwidth) {

			return (*it);
		}
	}

	return nullptr;
}


void DemodulatorMgr::updateLastState() {
    std::lock_guard < std::recursive_mutex > lock(demods_busy);

    if (std::find(demods.begin(), demods.end(), lastActiveDemodulator) == demods.end()) {
        if (activeDemodulator && activeDemodulator->isActive()) {
            lastActiveDemodulator = activeDemodulator;
        } else if (activeDemodulator && !activeDemodulator->isActive()){
            activeDemodulator = nullptr;
            lastActiveDemodulator = nullptr;
        }
    }

    if (lastActiveDemodulator && !lastActiveDemodulator->isActive()) {
        lastActiveDemodulator = nullptr;
    }

    if (lastActiveDemodulator) {
        lastBandwidth = lastActiveDemodulator->getBandwidth();
        lastDemodType = lastActiveDemodulator->getDemodulatorType();
        lastDemodLock = lastActiveDemodulator->getDemodulatorLock()?true:false;
        lastSquelchEnabled = lastActiveDemodulator->isSquelchEnabled();
        lastSquelch = lastActiveDemodulator->getSquelchLevel();
        lastGain = lastActiveDemodulator->getGain();
        lastModemSettings[lastDemodType] = lastActiveDemodulator->readModemSettings();
    }

}

int DemodulatorMgr::getLastBandwidth() const {
    return lastBandwidth;
}

void DemodulatorMgr::setLastBandwidth(int lastBandwidth) {
    if (lastBandwidth < MIN_BANDWIDTH) {
        lastBandwidth = MIN_BANDWIDTH;
    } else  if (lastBandwidth > wxGetApp().getSampleRate()) {
        lastBandwidth = wxGetApp().getSampleRate();
    }
    this->lastBandwidth = lastBandwidth;
}

std::string DemodulatorMgr::getLastDemodulatorType() const {
    return lastDemodType;
}

void DemodulatorMgr::setLastDemodulatorType(std::string lastDemodType) {
    this->lastDemodType = lastDemodType;
}

float DemodulatorMgr::getLastGain() const {
    return lastGain;
}

void DemodulatorMgr::setLastGain(float lastGain) {
    this->lastGain = lastGain;
}


bool DemodulatorMgr::getLastDeltaLock() const {
    return lastDeltaLock;
}

void DemodulatorMgr::setLastDeltaLock(bool lock) {
    lastDeltaLock = lock;
}

float DemodulatorMgr::getLastSquelchLevel() const {
    return lastSquelch;
}

void DemodulatorMgr::setLastSquelchLevel(float lastSquelch) {
    this->lastSquelch = lastSquelch;
}

bool DemodulatorMgr::isLastSquelchEnabled() const {
    return lastSquelchEnabled;
}

void DemodulatorMgr::setLastSquelchEnabled(bool lastSquelchEnabled) {
    this->lastSquelchEnabled = lastSquelchEnabled;
}

bool DemodulatorMgr::isLastMuted() const {
    return lastMuted;
}

void DemodulatorMgr::setLastMuted(bool lastMuted) {
    this->lastMuted = lastMuted;
}

ModemSettings DemodulatorMgr::getLastModemSettings(std::string modemType) {
    return lastModemSettings[modemType];
}

void DemodulatorMgr::setLastModemSettings(std::string modemType, ModemSettings settings) {
    lastModemSettings[modemType] = settings;
}

void DemodulatorMgr::setOutputDevices(std::map<int,RtAudio::DeviceInfo> devs) {
    outputDevices = devs;
}

void DemodulatorMgr::saveInstance(DataNode *node, DemodulatorInstancePtr inst) {

    *node->newChild("bandwidth") = inst->getBandwidth();
    *node->newChild("frequency") = inst->getFrequency();  
    *node->newChild("type") = inst->getDemodulatorType();
    
    node->newChild("user_label")->element()->set(inst->getDemodulatorUserLabel());
    
    *node->newChild("squelch_level") = inst->getSquelchLevel();
    *node->newChild("squelch_enabled") = inst->isSquelchEnabled() ? 1 : 0;
    *node->newChild("output_device") = outputDevices[inst->getOutputDevice()].name;
    *node->newChild("gain") = inst->getGain();
    *node->newChild("muted") = inst->isMuted() ? 1 : 0;
    if (inst->isDeltaLock()) {
        *node->newChild("delta_lock") = inst->isDeltaLock() ? 1 : 0;
        *node->newChild("delta_ofs") = inst->getDeltaLockOfs();
    }
    if (inst == getLastActiveDemodulator()) {
        *node->newChild("active") = 1;
    }
    
    ModemSettings saveSettings = inst->readModemSettings();
    if (saveSettings.size()) {
        DataNode *settingsNode = node->newChild("settings");
        for (ModemSettings::const_iterator msi = saveSettings.begin(); msi != saveSettings.end(); msi++) {
            *settingsNode->newChild(msi->first.c_str()) = msi->second;
        }
    }
}

std::wstring DemodulatorMgr::getSafeWstringValue(DataNode* node) {

    std::wstring decodedWString = L"";

    if (node != nullptr) {

        //1) decode as encoded wstring:
        try {
            node->element()->get(decodedWString);

        } catch (DataTypeMismatchException* e) {
            //2) wstring decode fail, try simple std::string
            delete e;
            std::string decodedStdString;
            try {

                node->element()->get(decodedStdString);

                //use wxString for a clean conversion to a wstring:
                decodedWString = wxString(decodedStdString).ToStdWstring();

            } catch (DataTypeMismatchException* e) {
                delete e;
                //nothing works, return an empty string.
                decodedWString = L"";
            }
        }
    }

    return decodedWString;
}

DemodulatorInstancePtr DemodulatorMgr::loadInstance(DataNode *node) {

	std::lock_guard < std::recursive_mutex > lock(demods_busy);

    DemodulatorInstancePtr newDemod = nullptr;
	 
    node->rewindAll();
    
    long bandwidth = *node->getNext("bandwidth");
    long long freq = *node->getNext("frequency");
    float squelch_level = node->hasAnother("squelch_level") ? (float) *node->getNext("squelch_level") : 0;
    int squelch_enabled = node->hasAnother("squelch_enabled") ? (int) *node->getNext("squelch_enabled") : 0;
    int muted = node->hasAnother("muted") ? (int) *node->getNext("muted") : 0;
    int delta_locked = node->hasAnother("delta_lock") ? (int) *node->getNext("delta_lock") : 0;
    int delta_ofs = node->hasAnother("delta_ofs") ? (int) *node->getNext("delta_ofs") : 0;
    std::string output_device = node->hasAnother("output_device") ? string(*(node->getNext("output_device"))) : "";
    float gain = node->hasAnother("gain") ? (float) *node->getNext("gain") : 1.0;
    
    std::string type = "FM";
    
    DataNode *demodTypeNode = node->hasAnother("type")?node->getNext("type"):nullptr;
    
    if (demodTypeNode && demodTypeNode->element()->getDataType() == DATA_INT) {
        int legacyType = *demodTypeNode;
        int legacyStereo = node->hasAnother("stereo") ? (int) *node->getNext("stereo") : 0;
        switch (legacyType) {   // legacy demod ID
            case 1: type = legacyStereo?"FMS":"FM"; break;
            case 2: type = "AM"; break;
            case 3: type = "LSB"; break;
            case 4: type = "USB"; break;
            case 5: type = "DSB"; break;
            case 6: type = "ASK"; break;
            case 7: type = "APSK"; break;
            case 8: type = "BPSK"; break;
            case 9: type = "DPSK"; break;
            case 10: type = "PSK"; break;
            case 11: type = "OOK"; break;
            case 12: type = "ST"; break;
            case 13: type = "SQAM"; break;
            case 14: type = "QAM"; break;
            case 15: type = "QPSK"; break;
            case 16: type = "I/Q"; break;
            default: type = "FM"; break;
        }
    } else if (demodTypeNode && demodTypeNode->element()->getDataType() == DATA_STRING) {
        demodTypeNode->element()->get(type);
    }
    
    //read the user label associated with the demodulator
    std::wstring user_label = L"";
    
    DataNode *demodUserLabel = node->hasAnother("user_label") ? node->getNext("user_label") : nullptr;
    
    if (demodUserLabel) {

        user_label = DemodulatorMgr::getSafeWstringValue(demodUserLabel);
    }
    
    ModemSettings mSettings;
    
    if (node->hasAnother("settings")) {
        DataNode *modemSettings = node->getNext("settings");
        for (int msi = 0, numSettings = modemSettings->numChildren(); msi < numSettings; msi++) {
            DataNode *settingNode = modemSettings->child(msi);
            std::string keyName = settingNode->getName();
            std::string strSettingValue = settingNode->element()->toString();
            
            if (keyName != "" && strSettingValue != "") {
                mSettings[keyName] = strSettingValue;
            }
        }
    }
    
    newDemod = newThread();

    newDemod->setDemodulatorType(type);
    newDemod->setDemodulatorUserLabel(user_label);
    newDemod->writeModemSettings(mSettings);
    newDemod->setBandwidth(bandwidth);
    newDemod->setFrequency(freq);
    newDemod->setGain(gain);
    newDemod->updateLabel(freq);
    newDemod->setMuted(muted?true:false);
    if (delta_locked) {
        newDemod->setDeltaLock(true);
        newDemod->setDeltaLockOfs(delta_ofs);
    }
    if (squelch_enabled) {
        newDemod->setSquelchEnabled(true);
        newDemod->setSquelchLevel(squelch_level);
    }
    
	//Attach to sound output:
    std::map<int, RtAudio::DeviceInfo>::iterator i;

	bool matching_device_found = false;

    for (i = outputDevices.begin(); i != outputDevices.end(); i++) {
        if (i->second.name == output_device) {
            newDemod->setOutputDevice(i->first);
			matching_device_found = true;
			break;
        }
    }
	//if no device is found, choose the first of the list anyway.
	if (!matching_device_found) {
		newDemod->setOutputDevice(outputDevices.begin()->first);
	}
    
    return newDemod;
}


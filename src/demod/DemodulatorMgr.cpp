#include <DemodulatorMgr.h>
#include <sstream>
#include <algorithm>
#include "CubicSDR.h"
#include <string>
#include <sstream>
#include <algorithm>

bool demodFreqCompare (DemodulatorInstance *i, DemodulatorInstance *j) { return (i->getFrequency()<j->getFrequency()); }
bool inactiveCompare (DemodulatorInstance *i, DemodulatorInstance *j) { return (i->isActive()<j->isActive()); }

DemodulatorMgr::DemodulatorMgr() :
        activeDemodulator(NULL), lastActiveDemodulator(NULL), activeVisualDemodulator(NULL), lastBandwidth(DEFAULT_DEMOD_BW), lastDemodType(
                DEFAULT_DEMOD_TYPE), lastSquelchEnabled(false), lastSquelch(-100), lastGain(1.0), lastMuted(false), lastDeltaLock(false) {
}

DemodulatorMgr::~DemodulatorMgr() {
    terminateAll();
}

DemodulatorInstance *DemodulatorMgr::newThread() {
    DemodulatorInstance *newDemod = new DemodulatorInstance;
    demods.push_back(newDemod);

    std::stringstream label;
    label << demods.size();
    newDemod->setLabel(label.str());

    return newDemod;
}

void DemodulatorMgr::terminateAll() {
    while (demods.size()) {
        DemodulatorInstance *d = demods.back();
        demods.pop_back();
        wxGetApp().removeDemodulator(d);
        deleteThread(d);
    }
}

std::vector<DemodulatorInstance *> &DemodulatorMgr::getDemodulators() {
    return demods;
}

std::vector<DemodulatorInstance *> DemodulatorMgr::getOrderedDemodulators(bool actives) {
    std::vector<DemodulatorInstance *> demods_ordered = demods;
    if (actives) {
        std::sort(demods_ordered.begin(), demods_ordered.end(), inactiveCompare);
        std::vector<DemodulatorInstance *>::iterator i;
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
    std::sort(demods_ordered.begin(), demods_ordered.end(), demodFreqCompare);
    return demods_ordered;
}

DemodulatorInstance *DemodulatorMgr::getPreviousDemodulator(DemodulatorInstance *demod, bool actives) {
    if (!getLastActiveDemodulator()) {
        return nullptr;
    }
    std::vector<DemodulatorInstance *> demods_ordered = getOrderedDemodulators(actives);
    std::vector<DemodulatorInstance *>::iterator p = std::find(demods_ordered.begin(), demods_ordered.end(), demod);
    if (p == demods_ordered.end()) {
        return nullptr;
    }
    if (*p == demods_ordered.front()) {
        return demods_ordered.back();
    }
    return *(--p);
}

DemodulatorInstance *DemodulatorMgr::getNextDemodulator(DemodulatorInstance *demod, bool actives) {
    if (!getLastActiveDemodulator()) {
        return nullptr;
    }
    std::vector<DemodulatorInstance *> demods_ordered = getOrderedDemodulators(actives);
    std::vector<DemodulatorInstance *>::iterator p = std::find(demods_ordered.begin(), demods_ordered.end(), demod);
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

DemodulatorInstance *DemodulatorMgr::getLastDemodulator() {
    std::vector<DemodulatorInstance *> demods_ordered = getOrderedDemodulators();
    return *(demods_ordered.end());
}

DemodulatorInstance *DemodulatorMgr::getFirstDemodulator() {
    std::vector<DemodulatorInstance *> demods_ordered = getOrderedDemodulators();
    return *(demods_ordered.begin());
}

void DemodulatorMgr::deleteThread(DemodulatorInstance *demod) {
    std::vector<DemodulatorInstance *>::iterator i;

    i = std::find(demods.begin(), demods.end(), demod);

    if (activeDemodulator == demod) {
        activeDemodulator = NULL;
    }
    if (lastActiveDemodulator == demod) {
        lastActiveDemodulator = NULL;
    }
    if (activeVisualDemodulator == demod) {
        activeVisualDemodulator = NULL;
    }

    if (i != demods.end()) {
        demods.erase(i);
    }
    demod->terminate();

    demods_deleted.push_back(demod);

    garbageCollect();
}

std::vector<DemodulatorInstance *> *DemodulatorMgr::getDemodulatorsAt(long long freq, int bandwidth) {
    std::vector<DemodulatorInstance *> *foundDemods = new std::vector<DemodulatorInstance *>();

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        DemodulatorInstance *testDemod = demods[i];

        long long freqTest = testDemod->getFrequency();
        long long bandwidthTest = testDemod->getBandwidth();
        long long halfBandwidthTest = bandwidthTest / 2;

        long long halfBuffer = bandwidth / 2;

        if ((freq <= (freqTest + ((testDemod->getDemodulatorType() != "LSB")?halfBandwidthTest:0) + halfBuffer)) && (freq >= (freqTest - ((testDemod->getDemodulatorType() != "USB")?halfBandwidthTest:0) - halfBuffer))) {
            foundDemods->push_back(testDemod);
        }
    }

    return foundDemods;
}

bool DemodulatorMgr::anyDemodulatorsAt(long long freq, int bandwidth) {
    
    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        DemodulatorInstance *testDemod = demods[i];

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


void DemodulatorMgr::setActiveDemodulator(DemodulatorInstance *demod, bool temporary) {
    if (!temporary) {
        if (activeDemodulator != NULL) {
            lastActiveDemodulator = activeDemodulator;
            updateLastState();
        } else {
            lastActiveDemodulator = demod;
        }
        updateLastState();
    }

    if (activeVisualDemodulator) {
        activeVisualDemodulator->setVisualOutputQueue(NULL);
    }
    if (demod) {
        demod->setVisualOutputQueue(wxGetApp().getAudioVisualQueue());
        activeVisualDemodulator = demod;
    } else {
        DemodulatorInstance *last = getLastActiveDemodulator();
        if (last) {
            last->setVisualOutputQueue(wxGetApp().getAudioVisualQueue());
        }
        activeVisualDemodulator = last;
    }

    activeDemodulator = demod;

    garbageCollect();
}

DemodulatorInstance *DemodulatorMgr::getActiveDemodulator() {
    if (activeDemodulator && !activeDemodulator->isActive()) {
        activeDemodulator = getLastActiveDemodulator();
    }
    return activeDemodulator;
}

DemodulatorInstance *DemodulatorMgr::getLastActiveDemodulator() {
    return lastActiveDemodulator;
}

void DemodulatorMgr::garbageCollect() {
    if (demods_deleted.size()) {
        std::vector<DemodulatorInstance *>::iterator i;

        for (i = demods_deleted.begin(); i != demods_deleted.end(); i++) {
            if ((*i)->isTerminated()) {
                DemodulatorInstance *deleted = (*i);
                demods_deleted.erase(i);

                std::cout << "Garbage collected demodulator instance " << deleted->getLabel() << std::endl;

                delete deleted;
                return;
            }
        }
    }
}

void DemodulatorMgr::updateLastState() {
    if (std::find(demods.begin(), demods.end(), lastActiveDemodulator) == demods.end()) {
        if (activeDemodulator && activeDemodulator->isActive()) {
            lastActiveDemodulator = activeDemodulator;
        } else if (activeDemodulator && !activeDemodulator->isActive()){
            activeDemodulator = NULL;
            lastActiveDemodulator = NULL;
        }
    }

    if (lastActiveDemodulator && !lastActiveDemodulator->isActive()) {
        lastActiveDemodulator = NULL;
    }

    if (lastActiveDemodulator) {
        lastBandwidth = lastActiveDemodulator->getBandwidth();
        lastDemodType = lastActiveDemodulator->getDemodulatorType();
        lastDemodLock = lastActiveDemodulator->getDemodulatorLock();
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

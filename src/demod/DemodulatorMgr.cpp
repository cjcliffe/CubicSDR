#include <DemodulatorMgr.h>
#include <sstream>
#include <algorithm>
#include "CubicSDR.h"
#include <string>
#include <sstream>

DemodulatorMgr::DemodulatorMgr() :
        activeDemodulator(NULL), lastActiveDemodulator(NULL), activeVisualDemodulator(NULL), lastBandwidth(DEFAULT_DEMOD_BW), lastDemodType(
                DEFAULT_DEMOD_TYPE), lastSquelchEnabled(false), lastSquelch(-100), lastGain(1.0), lastMuted(false) {
    setLastBandwidth("FM",200000);
    setLastBandwidth("FMS",200000);
    setLastBandwidth("AM",6000);
    setLastBandwidth("USB",5400);
    setLastBandwidth("LSB",5400);
    setLastBandwidth("DSB",5400);
    setLastBandwidth("IQ",48000);
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
        lastBandwidthNamed[lastDemodType] = lastBandwidth;
    }

}

int DemodulatorMgr::getLastBandwidth() const {
    return lastBandwidth;
}

void DemodulatorMgr::setLastBandwidth(int lastBandwidth) {
    if (lastBandwidth < 1500) {
        lastBandwidth = 1500;
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

int DemodulatorMgr::getLastBandwidth(std::string modemType) {
    return lastBandwidthNamed[modemType];
}
void DemodulatorMgr::setLastBandwidth(std::string modemType, int lastBandwidth_in) {
    lastBandwidthNamed[modemType] = lastBandwidth_in;
}

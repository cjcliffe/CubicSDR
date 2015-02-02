#include <DemodulatorMgr.h>
#include <sstream>
#include <algorithm>
#include "CubicSDR.h"
#include <string>
#include <sstream>

DemodulatorMgr::DemodulatorMgr() :
        activeDemodulator(NULL), lastActiveDemodulator(NULL), activeVisualDemodulator(NULL) {

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

        if ((freq <= (freqTest + halfBandwidthTest + halfBuffer)) && (freq >= (freqTest - halfBandwidthTest - halfBuffer))) {
            foundDemods->push_back(testDemod);
        }
    }

    return foundDemods;
}

void DemodulatorMgr::setActiveDemodulator(DemodulatorInstance *demod, bool temporary) {
    if (!temporary) {
        if (activeDemodulator != NULL) {
            lastActiveDemodulator = activeDemodulator;
        } else {
            lastActiveDemodulator = demod;
        }
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
    return activeDemodulator;
}

DemodulatorInstance *DemodulatorMgr::getLastActiveDemodulator() {
    if (std::find(demods.begin(), demods.end(), lastActiveDemodulator) == demods.end()) {
        lastActiveDemodulator = activeDemodulator;
    }

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


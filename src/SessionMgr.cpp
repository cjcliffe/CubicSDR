// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SessionMgr.h"
#include "AppServices.h"
#include "DemodulatorMgr.h"
#include "CubicSDRDefs.h"
#include "StringUtil.h"
#include <algorithm>
#include <cctype>

void SessionMgr::saveSession(std::string fileName) {

    DataTree s("cubicsdr_session");
    DataNode *header = s.rootNode()->newChild("header");
    //save as wstring to prevent problems
    header->newChild("version")->element()->set(utf8ToWstring(CUBICSDR_VERSION));

    *header->newChild("center_freq") = GetAppServices().getFrequency();
    *header->newChild("sample_rate") = GetAppServices().getSampleRate();
    *header->newChild("solo_mode") = GetAppServices().getSoloMode()?1:0;

    long long viewCenterFreq = 0;
    long long viewBandwidth = 0;

    if (GetAppServices().getMainZoomedView(viewCenterFreq, viewBandwidth)) {
        DataNode *viewState = header->newChild("view_state");

        *viewState->newChild("center_freq") = viewCenterFreq;
        *viewState->newChild("bandwidth") = viewBandwidth;
    }

    DataNode *demods = s.rootNode()->newChild("demodulators");

    //make a local copy snapshot of the list
    std::vector<DemodulatorInstancePtr> instances = GetAppServices().getDemodMgr().getDemodulators();

    for (const auto &instance : instances) {
        DataNode *demod = demods->newChild("demodulator");
        GetAppServices().getDemodMgr().saveInstance(demod, instance);
    } //end for demodulators

    // Make sure the file name actually ends in .xml
    std::string lcFileName = fileName;
    std::transform(lcFileName.begin(), lcFileName.end(), lcFileName.begin(), ::tolower);

    if (lcFileName.find_last_of(".xml") != lcFileName.length()-1) {
        fileName.append(".xml");
    }

    s.SaveToFileXML(fileName);
}

bool SessionMgr::loadSession(const std::string& fileName) {

    DataTree l;
    if (!l.LoadFromFileXML(fileName)) {
        return false;
    }

    //Check if it is a session file, read the root node.
    if (l.rootNode()->getName() != "cubicsdr_session") {
        return false;
    }

    GetAppServices().getDemodMgr().setActiveDemodulator(nullptr);
    GetAppServices().getDemodMgr().terminateAll();


    try {
        if (!l.rootNode()->hasAnother("header")) {
            return false;
        }
        DataNode *header = l.rootNode()->getNext("header");

        if (header->hasAnother("version")) {
            //"Force" the retrieving of the value as string, even if it looks like a number internally ! (ex: "0.2.0")
            DataNode *versionNode = header->getNext("version");
            std::wstring version;
            try {
                versionNode->element()->get(version);

                std::cout << "Loading session file version: '" << std::string(version.begin(), version.end()) << "'..." << std::endl;
            }
            catch (DataTypeMismatchException &e) {
                //this is for managing the old session format NOT encoded as std:wstring,
                //force current version
                std::cout << "Warning while Loading session file version, probably old format :'" << e.what() << "' please consider re-saving the current session..." << std::endl << std::flush;
                version = utf8ToWstring(CUBICSDR_VERSION);
            }
        }

        if (header->hasAnother("sample_rate")) {

            long sample_rate = (long)*header->getNext("sample_rate");

            SDRDeviceInfo *dev = GetAppServices().getSDRThread()->getDevice();
            if (dev) {
                //retrieve the available sample rates. A valid previously chosen manual
                //value is constrained within these limits. If it doesn't behave, lets the device choose
                //for us.
                long minRate = MANUAL_SAMPLE_RATE_MIN;
                long maxRate = MANUAL_SAMPLE_RATE_MAX;

                std::vector<long> sampleRates = dev->getSampleRates(SOAPY_SDR_RX, 0);

                if (!sampleRates.empty()) {
                    minRate = sampleRates.front();
                    maxRate = sampleRates.back();
                }

                //If it is beyond limits, make device choose a reasonable value
                if (sample_rate < minRate || sample_rate > maxRate) {
                    sample_rate = dev->getSampleRateNear(SOAPY_SDR_RX, 0, sample_rate);
                }


                //update applied value
                GetAppServices().setSampleRate(sample_rate);
            } else {
                GetAppServices().setSampleRate(sample_rate);
            }
        }

        if (header->hasAnother("solo_mode")) {

            int solo_mode_activated = (int)*header->getNext("solo_mode");

            GetAppServices().setSoloMode(solo_mode_activated > 0);
        }
        else {
            GetAppServices().setSoloMode(false);
        }

        DemodulatorInstancePtr loadedActiveDemod = nullptr;
        DemodulatorInstancePtr newDemod = nullptr;

        if (l.rootNode()->hasAnother("demodulators")) {

            DataNode *demodulators = l.rootNode()->getNext("demodulators");

            std::vector<DemodulatorInstancePtr> demodsLoaded;

            while (demodulators->hasAnother("demodulator")) {
                DataNode *demod = demodulators->getNext("demodulator");

                if (!demod->hasAnother("bandwidth") || !demod->hasAnother("frequency")) {
                    continue;
                }

                newDemod = GetAppServices().getDemodMgr().loadInstance(demod);

                if (demod->hasAnother("active")) {
                    loadedActiveDemod = newDemod;
                }

                newDemod->run();
                newDemod->setActive(true);
                demodsLoaded.push_back(newDemod);
            }

            if (!demodsLoaded.empty()) {
                GetAppServices().notifyDemodulatorsChanged();
            }

        } // if l.rootNode()->hasAnother("demodulators")

        if (header->hasAnother("center_freq")) {
            long long center_freq = (long long)*header->getNext("center_freq");
            GetAppServices().setFrequency(center_freq);
            //            std::cout << "\tCenter Frequency: " << center_freq << std::endl;
        }

        if (header->hasAnother("view_state")) {
            DataNode *viewState = header->getNext("view_state");

            if (viewState->hasAnother("center_freq") && viewState->hasAnother("bandwidth")) {
                auto center_freq = (long long)*viewState->getNext("center_freq");
                auto bandwidth = (int)*viewState->getNext("bandwidth");
                GetAppServices().setMainZoomedView(center_freq, bandwidth);
            }
        } else {
            GetAppServices().resetMainZoomedView();
        }

        if (loadedActiveDemod || newDemod) {
            GetAppServices().getDemodMgr().setActiveDemodulator(loadedActiveDemod?loadedActiveDemod:newDemod);
        }
    } catch (DataTypeMismatchException &e) {
        std::cout << e.what() << std::endl;
        return false;
    }

    return true;
}
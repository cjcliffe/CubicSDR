// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SessionMgr.h"
#include "CubicSDR.h"

void SessionMgr::saveSession(std::string fileName) {

    DataTree s("cubicsdr_session");
    DataNode *header = s.rootNode()->newChild("header");
    //save as wstring to prevent problems
    header->newChild("version")->element()->set(wxString(CUBICSDR_VERSION).ToStdWstring());

    *header->newChild("center_freq") = wxGetApp().getFrequency();
    *header->newChild("sample_rate") = wxGetApp().getSampleRate();
    *header->newChild("solo_mode") = wxGetApp().getSoloMode()?1:0;

    WaterfallCanvas *waterfallCanvas = wxGetApp().getAppFrame()->getWaterfallCanvas();

    if (waterfallCanvas->getViewState()) {
        DataNode *viewState = header->newChild("view_state");

        *viewState->newChild("center_freq") = waterfallCanvas->getCenterFrequency();
        *viewState->newChild("bandwidth") = waterfallCanvas->getBandwidth();
    }

    DataNode *demods = s.rootNode()->newChild("demodulators");

    //make a local copy snapshot of the list
    std::vector<DemodulatorInstancePtr> instances = wxGetApp().getDemodMgr().getDemodulators();

    for (const auto &instance : instances) {
        DataNode *demod = demods->newChild("demodulator");
        wxGetApp().getDemodMgr().saveInstance(demod, instance);
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

    wxGetApp().getDemodMgr().setActiveDemodulator(nullptr, false);
    wxGetApp().getDemodMgr().terminateAll();

    WaterfallCanvas *waterfallCanvas = wxGetApp().getAppFrame()->getWaterfallCanvas();
    SpectrumCanvas *spectrumCanvas = wxGetApp().getAppFrame()->getSpectrumCanvas();

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

                std::cout << "Loading session file version: '" << version << "'..." << std::endl;
            }
            catch (DataTypeMismatchException &e) {
                //this is for managing the old session format NOT encoded as std:wstring,
                //force current version
                std::cout << "Warning while Loading session file version, probably old format :'" << e.what() << "' please consider re-saving the current session..." << std::endl << std::flush;
                version = wxString(CUBICSDR_VERSION).ToStdWstring();
            }
        }

        if (header->hasAnother("sample_rate")) {

            long sample_rate = (long)*header->getNext("sample_rate");

            SDRDeviceInfo *dev = wxGetApp().getSDRThread()->getDevice();
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
                wxGetApp().setSampleRate(sample_rate);
            } else {
                wxGetApp().setSampleRate(sample_rate);
            }
        }

        if (header->hasAnother("solo_mode")) {

            int solo_mode_activated = (int)*header->getNext("solo_mode");

            wxGetApp().setSoloMode(solo_mode_activated > 0);
        }
        else {
            wxGetApp().setSoloMode(false);
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

                newDemod = wxGetApp().getDemodMgr().loadInstance(demod);

                if (demod->hasAnother("active")) {
                    loadedActiveDemod = newDemod;
                }

                newDemod->run();
                newDemod->setActive(true);
                demodsLoaded.push_back(newDemod);
            }

            if (!demodsLoaded.empty()) {
                wxGetApp().notifyDemodulatorsChanged();
            }

        } // if l.rootNode()->hasAnother("demodulators")

        if (header->hasAnother("center_freq")) {
            long long center_freq = (long long)*header->getNext("center_freq");
            wxGetApp().setFrequency(center_freq);
            //            std::cout << "\tCenter Frequency: " << center_freq << std::endl;
        }

        if (header->hasAnother("view_state")) {
            DataNode *viewState = header->getNext("view_state");

            if (viewState->hasAnother("center_freq") && viewState->hasAnother("bandwidth")) {
                auto center_freq = (long long)*viewState->getNext("center_freq");
                auto bandwidth = (int)*viewState->getNext("bandwidth");
                spectrumCanvas->setView(center_freq, bandwidth);
                waterfallCanvas->setView(center_freq, bandwidth);
            }
        } else {
            spectrumCanvas->disableView();
            waterfallCanvas->disableView();
            spectrumCanvas->setCenterFrequency(wxGetApp().getFrequency());
            waterfallCanvas->setCenterFrequency(wxGetApp().getFrequency());
        }

        if (loadedActiveDemod || newDemod) {
            wxGetApp().getDemodMgr().setActiveDemodulator(loadedActiveDemod?loadedActiveDemod:newDemod, false);
        }
    } catch (DataTypeMismatchException &e) {
        std::cout << e.what() << std::endl;
        return false;
    }

    return true;
}
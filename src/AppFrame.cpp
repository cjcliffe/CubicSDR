#include "AppFrame.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/numdlg.h"
#include "wx/filedlg.h"

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include <vector>
#include "SDRThread.h"
#include "DemodulatorMgr.h"
#include "AudioThread.h"
#include "CubicSDR.h"
#include "DataTree.h"

#include <thread>

#include <wx/panel.h>

wxBEGIN_EVENT_TABLE(AppFrame, wxFrame)
//EVT_MENU(wxID_NEW, AppFrame::OnNewWindow)
EVT_MENU(wxID_CLOSE, AppFrame::OnClose)
EVT_MENU(wxID_ANY, AppFrame::OnMenu)
EVT_COMMAND(wxID_ANY, wxEVT_THREAD, AppFrame::OnThread)
EVT_IDLE(AppFrame::OnIdle)
wxEND_EVENT_TABLE()

AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, wxT(CUBICSDR_TITLE)), activeDemodulator(NULL) {

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodOpts = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodVisuals = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodTray = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *demodScopeTray = new wxBoxSizer(wxVERTICAL);

    demodModeSelector = new ModeSelectorCanvas(this, NULL);
    demodModeSelector->addChoice(DEMOD_TYPE_FM, "FM");
    demodModeSelector->addChoice(DEMOD_TYPE_AM, "AM");
    demodModeSelector->addChoice(DEMOD_TYPE_LSB, "LSB");
    demodModeSelector->addChoice(DEMOD_TYPE_USB, "USB");
    demodModeSelector->addChoice(DEMOD_TYPE_DSB, "DSB");
    demodTray->Add(demodModeSelector, 2, wxEXPAND | wxALL, 0);

//    demodTray->AddSpacer(2);

    demodSpectrumCanvas = new SpectrumCanvas(this, NULL);
    demodSpectrumCanvas->setup(1024);
    demodSpectrumCanvas->setView(DEFAULT_FREQ, 300000);
    demodVisuals->Add(demodSpectrumCanvas, 3, wxEXPAND | wxALL, 0);

    demodVisuals->AddSpacer(1);

    demodWaterfallCanvas = new WaterfallCanvas(this, NULL);
    demodWaterfallCanvas->setup(1024, 128);
    demodWaterfallCanvas->setView(DEFAULT_FREQ, 300000);
    demodWaterfallCanvas->attachSpectrumCanvas(demodSpectrumCanvas);
    demodSpectrumCanvas->attachWaterfallCanvas(demodWaterfallCanvas);
    demodVisuals->Add(demodWaterfallCanvas, 6, wxEXPAND | wxALL, 0);

    demodTray->Add(demodVisuals, 30, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(2);

    demodSignalMeter = new MeterCanvas(this, NULL);
    demodSignalMeter->setMax(0.5);
    demodSignalMeter->setHelpTip("Current Signal Level.  Click / Drag to set Squelch level.");
    demodTray->Add(demodSignalMeter, 1, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(2);

    scopeCanvas = new ScopeCanvas(this, NULL);
    demodScopeTray->Add(scopeCanvas, 8, wxEXPAND | wxALL, 0);

    demodScopeTray->AddSpacer(2);

    demodTuner = new TuningCanvas(this, NULL);
    demodTuner->setHelpTip("Testing tuner");
    demodScopeTray->Add(demodTuner, 1, wxEXPAND | wxALL, 0);

    demodTray->Add(demodScopeTray, 30, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(2);

    demodGainMeter = new MeterCanvas(this, NULL);
    demodGainMeter->setMax(2.0);
    demodGainMeter->setHelpTip("Current Demodulator Gain Level.  Click / Drag to set Gain level.");
    demodTray->Add(demodGainMeter, 1, wxEXPAND | wxALL, 0);

    vbox->Add(demodTray, 12, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(2);
    spectrumCanvas = new SpectrumCanvas(this, NULL);
    spectrumCanvas->setup(2048);
    vbox->Add(spectrumCanvas, 5, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(2);
    waterfallCanvas = new WaterfallCanvas(this, NULL);
    waterfallCanvas->setup(2048, 512);
    waterfallCanvas->attachSpectrumCanvas(spectrumCanvas);
    spectrumCanvas->attachWaterfallCanvas(waterfallCanvas);
    vbox->Add(waterfallCanvas, 20, wxEXPAND | wxALL, 0);

    this->SetSizer(vbox);

//    waterfallCanvas->SetFocusFromKbd();
    waterfallCanvas->SetFocus();

//    SetIcon(wxICON(sample));

// Make a menubar
    wxMenuBar *menuBar = new wxMenuBar;
    wxMenu *menu = new wxMenu;
//    menu->Append(wxID_NEW);
    menu->Append(wxID_SET_FREQ_OFFSET, "Set Frequency Offset");
    menu->Append(wxID_OPEN, "&Open Session");
    menu->Append(wxID_SAVE, "&Save Session");
    menu->Append(wxID_SAVEAS, "Save Session &As..");
    menu->AppendSeparator();
    menu->Append(wxID_RESET, "&Reset Session");
    menu->Append(wxID_CLOSE);

    menuBar->Append(menu, wxT("&File"));

    menu = new wxMenu;

    std::vector<RtAudio::DeviceInfo>::iterator devices_i;
    std::map<int, RtAudio::DeviceInfo>::iterator mdevices_i;
    AudioThread::enumerateDevices(devices);

    int i = 0;

    for (devices_i = devices.begin(); devices_i != devices.end(); devices_i++) {
        if (devices_i->inputChannels) {
            inputDevices[i] = *devices_i;
        }
        if (devices_i->outputChannels) {
            outputDevices[i] = *devices_i;
        }
        i++;
    }

    i = 0;

    for (mdevices_i = outputDevices.begin(); mdevices_i != outputDevices.end(); mdevices_i++) {
        wxMenuItem *itm = menu->AppendRadioItem(wxID_RT_AUDIO_DEVICE + mdevices_i->first, mdevices_i->second.name, wxT("Description?"));
        itm->SetId(wxID_RT_AUDIO_DEVICE + mdevices_i->first);
        if (mdevices_i->second.isDefaultOutput) {
            itm->Check(true);
        }
        outputDeviceMenuItems[mdevices_i->first] = itm;
    }

    menuBar->Append(menu, wxT("Active Demodulator &Output"));

    SetMenuBar(menuBar);

    CreateStatusBar();
    SetClientSize(1280, 600);
    Centre();
    Show();

    GetStatusBar()->SetStatusText(wxString::Format(wxT("Set center frequency: %i"), DEFAULT_FREQ));

    wxAcceleratorEntry entries[3];
    entries[0].Set(wxACCEL_CTRL, (int) 'O', wxID_OPEN);
    entries[1].Set(wxACCEL_CTRL, (int) 'S', wxID_SAVE);
    entries[2].Set(wxACCEL_CTRL, (int) 'A', wxID_SAVEAS);

    wxAcceleratorTable accel(3, entries);
    SetAcceleratorTable(accel);

//    static const int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
//    wxLogStatus("Double-buffered display %s supported", wxGLCanvas::IsDisplaySupported(attribs) ? "is" : "not");
//    ShowFullScreen(true);
}

AppFrame::~AppFrame() {

}

void AppFrame::OnMenu(wxCommandEvent& event) {
    if (event.GetId() >= wxID_RT_AUDIO_DEVICE && event.GetId() < wxID_RT_AUDIO_DEVICE + devices.size()) {
        if (activeDemodulator) {
            activeDemodulator->setOutputDevice(event.GetId() - wxID_RT_AUDIO_DEVICE);
            activeDemodulator = NULL;
        }
    } else if (event.GetId() == wxID_SET_FREQ_OFFSET) {
        long ofs = wxGetNumberFromUser("Shift the displayed frequency by this amount.\ni.e. -125000000 for -125 MHz", "Frequency (Hz)",
                "Frequency Offset", wxGetApp().getOffset(), -2000000000, 2000000000, this);
        if (ofs != -1) {
            wxGetApp().setOffset(ofs);
        }
    } else if (event.GetId() == wxID_SAVE) {
        if (!currentSessionFile.empty()) {
            saveSession(currentSessionFile);
        } else {
            wxFileDialog saveFileDialog(this, _("Save XML Session file"), "", "", "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (saveFileDialog.ShowModal() == wxID_CANCEL) {
                return;
            }
            saveSession(saveFileDialog.GetPath().ToStdString());
        }
    } else if (event.GetId() == wxID_OPEN) {
        wxFileDialog openFileDialog(this, _("Open XML Session file"), "", "", "XML files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (openFileDialog.ShowModal() == wxID_CANCEL) {
            return;
        }
        loadSession(openFileDialog.GetPath().ToStdString());
    } else if (event.GetId() == wxID_SAVEAS) {
        wxFileDialog saveFileDialog(this, _("Save XML Session file"), "", "", "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (saveFileDialog.ShowModal() == wxID_CANCEL) {
            return;
        }
        saveSession(saveFileDialog.GetPath().ToStdString());
    } else if (event.GetId() == wxID_RESET) {
        wxGetApp().getDemodMgr().terminateAll();
        wxGetApp().setFrequency(DEFAULT_FREQ);
        wxGetApp().setOffset(0);
        SetTitle(wxT(CUBICSDR_TITLE));
        currentSessionFile = "";
    } else if (event.GetId() == wxID_EXIT) {
        Close(false);
    }
}

void AppFrame::OnClose(wxCommandEvent& WXUNUSED(event)) {
    Close(false);
}

void AppFrame::OnNewWindow(wxCommandEvent& WXUNUSED(event)) {
    new AppFrame();
}

void AppFrame::OnThread(wxCommandEvent& event) {
    event.Skip();
}

void AppFrame::OnIdle(wxIdleEvent& event) {
    bool work_done = false;

//#ifdef __APPLE__
//    std::this_thread::sleep_for(std::chrono::milliseconds(4));
//    std::this_thread::yield();
//#endif

    DemodulatorInstance *demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    if (demod) {
        if (demod != activeDemodulator) {
            demodSignalMeter->setInputValue(demod->getSquelchLevel());
            demodGainMeter->setInputValue(demod->getGain());
            int outputDevice = demod->getOutputDevice();
            scopeCanvas->setDeviceName(outputDevices[outputDevice].name);
            outputDeviceMenuItems[outputDevice]->Check(true);
            int dType = demod->getDemodulatorType();
            demodModeSelector->setSelection(dType);
        }
        if (demodWaterfallCanvas->getDragState() == WaterfallCanvas::WF_DRAG_NONE) {
            if (demod->getParams().frequency != demodWaterfallCanvas->getCenterFrequency()) {
                demodWaterfallCanvas->setCenterFrequency(demod->getFrequency());
                demodSpectrumCanvas->setCenterFrequency(demod->getFrequency());
            }
            int dSelection = demodModeSelector->getSelection();
            if (dSelection != -1 && dSelection != demod->getDemodulatorType()) {
                demod->setDemodulatorType(dSelection);
            }

            unsigned int demodBw = (unsigned int) ceil((float) demod->getParams().bandwidth * 2.5);
            if (demodBw > SRATE / 2) {
                demodBw = SRATE / 2;
            }
            if (demodBw < 80000) {
                demodBw = 80000;
            }
            demodWaterfallCanvas->setBandwidth(demodBw);
            demodSpectrumCanvas->setBandwidth(demodBw);
        }
        demodSignalMeter->setLevel(demod->getSignalLevel());
        demodGainMeter->setLevel(demod->getGain());
        if (demodSignalMeter->inputChanged()) {
            demod->setSquelchLevel(demodSignalMeter->getInputValue());
        }
        if (demodGainMeter->inputChanged()) {
            demod->setGain(demodGainMeter->getInputValue());
            demodGainMeter->setLevel(demodGainMeter->getInputValue());
        }
        activeDemodulator = demod;
    }

    if (!wxGetApp().getIQVisualQueue()->empty()) {
        DemodulatorThreadIQData *iqData;
        wxGetApp().getIQVisualQueue()->pop(iqData);

        if (iqData && iqData->data.size()) {
//            spectrumCanvas->setData(iqData);
            waterfallCanvas->setData(iqData);
            demodWaterfallCanvas->setData(iqData);
            delete iqData;
        } else {
            std::cout << "Incoming IQ data empty?" << std::endl;
        }
        work_done = true;
    }

    if (!wxGetApp().getAudioVisualQueue()->empty()) {
        AudioThreadInput *demodAudioData;
        wxGetApp().getAudioVisualQueue()->pop(demodAudioData);
        if (demodAudioData && demodAudioData->data.size()) {
            if (scopeCanvas->waveform_points.size() != demodAudioData->data.size() * 2) {
                scopeCanvas->waveform_points.resize(demodAudioData->data.size() * 2);
            }

            for (int i = 0, iMax = demodAudioData->data.size(); i < iMax; i++) {
                scopeCanvas->waveform_points[i * 2 + 1] = demodAudioData->data[i] * 0.5f;
                scopeCanvas->waveform_points[i * 2] = ((double) i / (double) iMax);
            }

            scopeCanvas->setStereo(demodAudioData->channels == 2);

            delete demodAudioData;
        } else {
            std::cout << "Incoming Demodulator data empty?" << std::endl;
        }
        work_done = true;
    }

    if (!waterfallCanvas->HasFocus()) {
        waterfallCanvas->SetFocus();
    }

    if (!work_done) {
        event.Skip();
    }
}

void AppFrame::saveSession(std::string fileName) {
    DataTree s("cubicsdr_session");
    DataNode *header = s.rootNode()->newChild("header");
    *header->newChild("version") = std::string(CUBICSDR_VERSION);
    *header->newChild("center_freq") = wxGetApp().getFrequency();
    *header->newChild("offset") = wxGetApp().getOffset();

    DataNode *demods = s.rootNode()->newChild("demodulators");

    std::vector<DemodulatorInstance *> &instances = wxGetApp().getDemodMgr().getDemodulators();
    std::vector<DemodulatorInstance *>::iterator instance_i;
    for (instance_i = instances.begin(); instance_i != instances.end(); instance_i++) {
        DataNode *demod = demods->newChild("demodulator");
        *demod->newChild("bandwidth") = (*instance_i)->getBandwidth();
        *demod->newChild("frequency") = (*instance_i)->getFrequency();
        *demod->newChild("type") = (*instance_i)->getDemodulatorType();
        *demod->newChild("squelch_level") = (*instance_i)->getSquelchLevel();
        *demod->newChild("squelch_enabled") = (*instance_i)->isSquelchEnabled() ? 1 : 0;
        *demod->newChild("stereo") = (*instance_i)->isStereo() ? 1 : 0;
        *demod->newChild("output_device") = outputDevices[(*instance_i)->getOutputDevice()].name;
        *demod->newChild("gain") = (*instance_i)->getGain();
    }

    s.SaveToFileXML(fileName);


    currentSessionFile = fileName;
    std::string filePart = fileName.substr(fileName.find_last_of(filePathSeparator)+1);
    GetStatusBar()->SetStatusText(wxString::Format(wxT("Saved session: %s"), currentSessionFile.c_str()));
    SetTitle(wxString::Format(wxT("%s: %s"), CUBICSDR_TITLE, filePart.c_str()));
}

bool AppFrame::loadSession(std::string fileName) {
    DataTree l;
    if (!l.LoadFromFileXML(fileName)) {
        return false;
    }

    wxGetApp().getDemodMgr().terminateAll();

    try {
        DataNode *header = l.rootNode()->getNext("header");

        std::string version(*header->getNext("version"));
        long long center_freq = *header->getNext("center_freq");
        long long offset = *header->getNext("offset");

        std::cout << "Loading " << version << " session file" << std::endl;
        std::cout << "\tCenter Frequency: " << center_freq << std::endl;
        std::cout << "\tOffset: " << offset << std::endl;

        wxGetApp().setOffset(offset);
        wxGetApp().setFrequency(center_freq);

        DataNode *demodulators = l.rootNode()->getNext("demodulators");

        while (demodulators->hasAnother("demodulator")) {
            DataNode *demod = demodulators->getNext("demodulator");

            if (!demod->hasAnother("bandwidth") || !demod->hasAnother("frequency")) {
                continue;
            }

            long bandwidth = *demod->getNext("bandwidth");
            long long freq = *demod->getNext("frequency");
            int type = demod->hasAnother("type") ? *demod->getNext("type") : DEMOD_TYPE_FM;
            float squelch_level = demod->hasAnother("squelch_level") ? (float) *demod->getNext("squelch_level") : 0;
            int squelch_enabled = demod->hasAnother("squelch_enabled") ? (int) *demod->getNext("squelch_enabled") : 0;
            int stereo = demod->hasAnother("stereo") ? (int) *demod->getNext("stereo") : 0;
            std::string output_device = demod->hasAnother("output_device") ? string(*(demod->getNext("output_device"))) : "";
            float gain = demod->hasAnother("gain") ? (float) *demod->getNext("gain") : 1.0;

            DemodulatorInstance *newDemod = wxGetApp().getDemodMgr().newThread();
            newDemod->setDemodulatorType(type);
            newDemod->setBandwidth(bandwidth);
            newDemod->setFrequency(freq);
            newDemod->setGain(gain);
            newDemod->updateLabel(freq);
            if (squelch_enabled) {
                newDemod->setSquelchEnabled(true);
                newDemod->setSquelchLevel(squelch_level);
            }
            if (stereo) {
                newDemod->setStereo(true);
            }

            bool found_device = false;
            std::map<int, RtAudio::DeviceInfo>::iterator i;
            for (i = outputDevices.begin(); i != outputDevices.end(); i++) {
                if (i->second.name == output_device) {
                    newDemod->setOutputDevice(i->first);
                    found_device = true;
                }
            }

            if (!found_device) {
                std::cout << "\tWarning: named output device '" << output_device << "' was not found. Using default output.";
            }

            newDemod->run();

            wxGetApp().bindDemodulator(newDemod);

            std::cout << "\tAdded demodulator at frequency " << freq << " type " << type << std::endl;
            std::cout << "\t\tBandwidth: " << bandwidth << std::endl;
            std::cout << "\t\tSquelch Level: " << squelch_level << std::endl;
            std::cout << "\t\tSquelch Enabled: " << (squelch_enabled ? "true" : "false") << std::endl;
            std::cout << "\t\tStereo: " << (stereo ? "true" : "false") << std::endl;
            std::cout << "\t\tOutput Device: " << output_device << std::endl;
        }
    } catch (DataInvalidChildException &e) {
        std::cout << e.what() << std::endl;
        return false;
    } catch (DataTypeMismatchException &e) {
        std::cout << e.what() << std::endl;
        return false;
    }

    currentSessionFile = fileName;

    std::string filePart = fileName.substr(fileName.find_last_of(filePathSeparator)+1);

    GetStatusBar()->SetStatusText(wxString::Format(wxT("Loaded session file: %s"), currentSessionFile.c_str()));
    SetTitle(wxString::Format(wxT("%s: %s"), CUBICSDR_TITLE, filePart.c_str()));
}

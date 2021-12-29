// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SDRDevices.h"

#include <wx/msgdlg.h>

#include "CubicSDR.h"

#include <algorithm>

#ifdef __linux__
#include "CubicSDR.xpm"
#endif

SDRDevicesDialog::SDRDevicesDialog( wxWindow* parent, const wxPoint &pos): devFrame( parent, wxID_ANY, wxT(CUBICSDR_INSTALL_NAME " :: SDR Devices"), pos) {
    refresh = true;
    failed = false;
    m_refreshButton->Disable();
    m_addRemoteButton->Disable();
    m_useSelectedButton->Disable();
    m_deviceTimer.Start(250);
    selId = nullptr;
    editId = nullptr;
    removeId = nullptr;
    devAddDialog = nullptr;
    dev = nullptr;

#ifdef __linux__
    SetIcon(wxICON(cubicsdr));
#elif _WIN32
    SetIcon(wxICON(frame_icon));
#endif
}

void SDRDevicesDialog::OnClose( wxCloseEvent& /* event */) {
    wxGetApp().setDeviceSelectorClosed();
    Destroy();
}

void SDRDevicesDialog::OnDeleteItem( wxTreeEvent& event ) {
    event.Skip();
}

wxPGProperty *SDRDevicesDialog::addArgInfoProperty(wxPropertyGrid *pg, SoapySDR::ArgInfo arg) {
    
    wxPGProperty *prop = nullptr;
    
    int intVal;
    double floatVal;
    std::vector<std::string>::iterator stringIter;
    
    switch (arg.type) {
        case SoapySDR::ArgInfo::INT:
            try {
                intVal = std::stoi(arg.value);
            } catch (const std::invalid_argument &) {
                intVal = 0;
            }
            prop = pg->Append( new wxIntProperty(arg.name, wxPG_LABEL, intVal) );
            if (arg.range.minimum() != arg.range.maximum()) {
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MIN, arg.range.minimum());
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MAX, arg.range.maximum());
            }
            break;
        case SoapySDR::ArgInfo::FLOAT:
            try {
                floatVal = std::stod(arg.value);
            } catch (const std::invalid_argument &) {
                floatVal = 0;
            }
            prop = pg->Append( new wxFloatProperty(arg.name, wxPG_LABEL, floatVal) );
            if (arg.range.minimum() != arg.range.maximum()) {
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MIN, arg.range.minimum());
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MAX, arg.range.maximum());
            }
            break;
        case SoapySDR::ArgInfo::BOOL:
            prop = pg->Append( new wxBoolProperty(arg.name, wxPG_LABEL, (arg.value=="true")) );
            break;
        case SoapySDR::ArgInfo::STRING:
            if (!arg.options.empty()) {
                intVal = 0;
                prop = pg->Append( new wxEnumProperty(arg.name, wxPG_LABEL) );
                for (stringIter = arg.options.begin(); stringIter != arg.options.end(); stringIter++) {
                    std::string optName = (*stringIter);
                    std::string displayName = optName;
                    if (!arg.optionNames.empty()) {
                        displayName = arg.optionNames[intVal];
                    }

                    prop->AddChoice(displayName);
                    if ((*stringIter)==arg.value) {
                        prop->SetChoiceSelection(intVal);
                    }
                    
                    intVal++;
                }
            } else {
                prop = pg->Append( new wxStringProperty(arg.name, wxPG_LABEL, arg.value) );
            }
            break;
    }
    
    if (prop != nullptr) {
        prop->SetHelpString(arg.key + ": " + arg.description);
    }
    
    return prop;
}

void SDRDevicesDialog::refreshDeviceProperties() {

    SDRDeviceInfo *selDev = getSelectedDevice(devTree->GetSelection());
    if (selDev && selDev->isAvailable()) {
        dev = selDev;
        selId = devTree->GetSelection();
        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getName());
        m_propertyGrid->Clear();
        
        SoapySDR::Device *soapyDev = dev->getSoapyDevice();
        SoapySDR::ArgInfoList args = soapyDev->getSettingInfo();
        
        //A) General settings: name, offset, sample rate, agc, antennas (if > 1) 
        m_propertyGrid->Append(new wxPropertyCategory("General Settings"));
        
        devSettings.clear();

        //A-1) Name
        devSettings["name"] = m_propertyGrid->Append( new wxStringProperty("Name", wxPG_LABEL, devConfig->getDeviceName()) );
        //A-2) Offset
        devSettings["offset"] = m_propertyGrid->Append( new wxIntProperty("Offset (KHz)", wxPG_LABEL, devConfig->getOffset() / 1000) );
        
        //A-3) Antennas, is there are more than 1 RX antenna, else do not expose the setting.
        //get the saved setting
        const std::string& currentSetAntenna = devConfig->getAntennaName();

        //compare to the list of existing antennas
        SoapySDR::ArgInfo antennasArg;
        std::vector<std::string> antennaOpts = selDev->getAntennaNames(SOAPY_SDR_RX, 0);

        //only do something if there is more than 1 antenna
        if (antennaOpts.size() > 1) {

            //by default, choose the first of the list.
            std::string antennaToSelect = antennaOpts.front();

            auto found_i = std::find(antennaOpts.begin(), antennaOpts.end(), currentSetAntenna);

            if (found_i != antennaOpts.end()) {
                antennaToSelect = currentSetAntenna;
            }
            else {
                //erroneous antenna name, re-write device config with the first choice of the list.
                devConfig->setAntennaName(antennaToSelect);
            }

            //build device settings
            for (const std::string& antenna : antennaOpts) {
                antennasArg.options.push_back(antenna);
                antennasArg.optionNames.push_back(antenna);
            }

            antennasArg.type = SoapySDR::ArgInfo::STRING;
            antennasArg.units = "";
            antennasArg.name = "Antenna";
            antennasArg.key = "antenna";
            antennasArg.value = antennaToSelect;

            devSettings["antenna"] = addArgInfoProperty(m_propertyGrid, antennasArg);
            deviceArgs["antenna"] = antennasArg;

        } //end if more than 1 antenna
        else {
            devConfig->setAntennaName("");
        }

        //A-4) Sample_rate:
        long currentSampleRate = wxGetApp().getSampleRate();
        long deviceSampleRate = devConfig->getSampleRate();
        
        if (!deviceSampleRate) {
            deviceSampleRate = selDev->getSampleRateNear(SOAPY_SDR_RX, 0, currentSampleRate);
        }
        
        SoapySDR::ArgInfo sampleRateArg;
        std::vector<long> rateOpts = selDev->getSampleRates(SOAPY_SDR_RX, 0);

        for (long rate : rateOpts) {
            sampleRateArg.options.push_back(std::to_string(rate));
            sampleRateArg.optionNames.push_back(frequencyToStr(rate));
        }
        
        sampleRateArg.type = SoapySDR::ArgInfo::STRING;
        sampleRateArg.units = "Hz";
        sampleRateArg.name = "Sample Rate";
        sampleRateArg.key = "sample_rate";
        sampleRateArg.value = std::to_string(deviceSampleRate);
        
        devSettings["sample_rate"] = addArgInfoProperty(m_propertyGrid, sampleRateArg);
        deviceArgs["sample_rate"] = sampleRateArg;

        

        //B) Runtime Settings:
        runtimeArgs.clear();
        runtimeProps.clear();
        streamProps.clear();
       
        if (!args.empty()) {
            m_propertyGrid->Append(new wxPropertyCategory("Run-time Settings"));
            
            for (const auto & args_i : args) {
                SoapySDR::ArgInfo arg = args_i;
				//We-reread the Device configuration, else we use the user settings.
				if (dev) {
					//Apply saved settings
					DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
					arg.value = devConfig->getSetting(arg.key, soapyDev->readSetting(arg.key)); //use SoapyDevice data as fallback.
				}
				else {
					//re-read the SoapyDevice
					arg.value = soapyDev->readSetting(arg.key);
				}
               
                runtimeProps[arg.key] = addArgInfoProperty(m_propertyGrid, arg);
                runtimeArgs[arg.key] = arg;
            }
        }
        
        if (dev) {
            args = dev->getSoapyDevice()->getStreamArgsInfo(SOAPY_SDR_RX, 0);
            
            DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
            ConfigSettings devStreamOpts = devConfig->getStreamOpts();
            if (!devStreamOpts.empty()) {
                for (auto & arg : args) {
                    if (devStreamOpts.find(arg.key) != devStreamOpts.end()) {
                        arg.value = devStreamOpts[arg.key];
                    }
                }
            }
            
            if (!args.empty()) {
                m_propertyGrid->Append(new wxPropertyCategory("Stream Settings"));
                
                for (const SoapySDR::ArgInfo& arg : args) {
                  
                    streamProps[arg.key] = addArgInfoProperty(m_propertyGrid, arg);
                }
            }
        }
        
        if (selDev->isManual()) {
            m_addRemoteButton->SetLabel("Remove");
            removeId = selId;
        } else {
            m_addRemoteButton->SetLabel("Add");
            removeId = nullptr;
        }
        
    } else if (selDev && !selDev->isAvailable() && selDev->isManual()) {
        m_propertyGrid->Clear();

        devSettings.clear();
        runtimeArgs.clear();
        runtimeProps.clear();
        streamProps.clear();

        removeId = devTree->GetSelection();
        dev = nullptr;
        selId = nullptr;
        editId = nullptr;
        
        m_addRemoteButton->SetLabel("Remove");
    } else if (!selDev) {
        m_addRemoteButton->SetLabel("Add");
        removeId = nullptr;
    }
}

void SDRDevicesDialog::OnSelectionChanged( wxTreeEvent& event ) {
    refreshDeviceProperties();
    event.Skip();
}

void SDRDevicesDialog::OnAddRemote( wxMouseEvent& /* event */) {
    if (removeId != nullptr) {
        SDRDeviceInfo *selDev = getSelectedDevice(removeId);

        if (selDev) {
            SDREnumerator::removeManual(selDev->getDriver(),selDev->getManualParams());
            m_propertyGrid->Clear();
			devSettings.clear();
			runtimeArgs.clear();
			runtimeProps.clear();
			streamProps.clear();
            dev = nullptr;
            selId = nullptr;
            editId = nullptr;
            devTree->Delete(removeId);
            removeId = nullptr;
            m_addRemoteButton->SetLabel("Add");
        }
        
        return;
    }
    
    devAddDialog = new SDRDeviceAddDialog(this);
    devAddDialog->ShowModal();
    
    if (devAddDialog->wasOkPressed()) {
        std::string module = devAddDialog->getSelectedModule();
        
        if (module == "SoapyRemote") {
            if (!SDREnumerator::hasRemoteModule()) {
                wxMessageDialog *info;
                info = new wxMessageDialog(nullptr, wxT("Install SoapyRemote module to add remote servers.\n\nhttps://github.com/pothosware/SoapyRemote"), wxT("SoapyRemote not found."), wxOK | wxICON_ERROR);
                info->ShowModal();
                return;
            }

            wxString remoteAddr = devAddDialog->getModuleParam();
        
            if (!remoteAddr.Trim().empty()) {
                wxGetApp().addRemote(remoteAddr.Trim().ToStdString());
            }
            devTree->Disable();
            m_addRemoteButton->Disable();
            m_useSelectedButton->Disable();
            refresh = true;
        } else {
            std::string mod = devAddDialog->getSelectedModule();
            std::string param = devAddDialog->getModuleParam();
            SDREnumerator::addManual(mod, param);
            doRefreshDevices();
        }
    }
}

SDRDeviceInfo *SDRDevicesDialog::getSelectedDevice(wxTreeItemId selId_in) {
    devItems_i = devItems.find(selId_in);
    if (devItems_i != devItems.end()) {
        return devItems[selId_in];
    }
    return nullptr;
}

void SDRDevicesDialog::OnUseSelected( wxMouseEvent& event) {
    if (dev != nullptr) {
        
        SoapySDR::ArgInfoList args = dev->getSoapyDevice()->getSettingInfo();
        
        SoapySDR::Kwargs settingArgs;
        SoapySDR::Kwargs streamArgs;
        
        for (const SoapySDR::ArgInfo& arg : args) {
           
            wxPGProperty *prop = runtimeProps[arg.key];
            
            if (arg.type == SoapySDR::ArgInfo::STRING && !arg.options.empty()) {
                settingArgs[arg.key] = getSelectedChoiceOption(prop, arg);
            } else if (arg.type == SoapySDR::ArgInfo::BOOL) {
                settingArgs[arg.key] = (prop->GetValueAsString()=="True")?"true":"false";
            } else {
                settingArgs[arg.key] = prop->GetValueAsString();
            }
        }
        
        if (dev) {
            args = dev->getSoapyDevice()->getStreamArgsInfo(SOAPY_SDR_RX, 0);
            
            if (!args.empty()) {
                for (const auto & args_i : args) {
                    SoapySDR::ArgInfo arg = args_i;
                    wxPGProperty *prop = streamProps[arg.key];
            
                    if (arg.type == SoapySDR::ArgInfo::STRING && !arg.options.empty()) {
                        streamArgs[arg.key] = getSelectedChoiceOption(prop, arg);
                    } else if (arg.type == SoapySDR::ArgInfo::BOOL) {
                        streamArgs[arg.key] = (prop->GetValueAsString()=="True")?"true":"false";
                    } else {
                        streamArgs[arg.key] = prop->GetValueAsString();
                    }
                }
            }
        }
        
        AppConfig *cfg = wxGetApp().getConfig();
        DeviceConfig *devConfig = cfg->getDevice(dev->getDeviceId());
        devConfig->setSettings(settingArgs);
        devConfig->setStreamOpts(streamArgs);
        wxGetApp().setDeviceArgs(settingArgs);
        wxGetApp().setStreamArgs(streamArgs);
        wxGetApp().setDevice(dev,0);
        
        //update main application title with Device name:
        wxString titleBar = CUBICSDR_TITLE;
        titleBar += " - " + wxGetApp().getDevice()->getName();
        wxGetApp().getAppFrame()->SetTitle(titleBar);

        Close();
    }
    event.Skip();
}

void SDRDevicesDialog::OnTreeDoubleClick( wxMouseEvent& event ) {
    OnUseSelected(event);
}

void SDRDevicesDialog::OnDeviceTimer( wxTimerEvent& event ) {
    if (refresh) {
        if (wxGetApp().areModulesMissing()) {
            if (!failed) {
                failed = true;
                wxMessageDialog *info;
                info = new wxMessageDialog(nullptr, wxT("\nNo SoapySDR modules were found.\n\nCubicSDR requires at least one SoapySDR device support module to be installed.\n\nPlease visit https://github.com/cjcliffe/CubicSDR/wiki and in the build instructions for your platform read the 'Support Modules' section for more information."), wxT("\x28\u256F\xB0\u25A1\xB0\uFF09\u256F\uFE35\x20\u253B\u2501\u253B"), wxOK | wxICON_ERROR);
                info->ShowModal();
            }
            return;
        }
        
        if (wxGetApp().areDevicesEnumerating() || !wxGetApp().areDevicesReady()) {
            std::string msg = wxGetApp().getNotification();
            devStatusBar->SetStatusText(msg);
            devTree->DeleteAllItems();
            devTree->AddRoot(msg);
            event.Skip();
            return;
        }
                
        devTree->DeleteAllItems();
        
        wxTreeItemId devRoot = devTree->AddRoot("Devices");
        wxTreeItemId localBranch = devTree->AppendItem(devRoot, "Local");
        wxTreeItemId dsBranch = devTree->AppendItem(devRoot, "Local Net");
        wxTreeItemId remoteBranch = devTree->AppendItem(devRoot, "Remote");
        wxTreeItemId manualBranch = devTree->AppendItem(devRoot, "Manual");
        
        devs[""] = SDREnumerator::enumerate_devices("",true);
        if (devs[""] != nullptr) {
            for (devs_i = devs[""]->begin(); devs_i != devs[""]->end(); devs_i++) {
                DeviceConfig *devConfig;
                if ((*devs_i)->isManual()) {
                    std::string devName = "Unknown";
                    if ((*devs_i)->isAvailable()) {
                        devConfig = wxGetApp().getConfig()->getDevice((*devs_i)->getDeviceId());
                        devName = devConfig->getDeviceName();
                    } else {
                        devName = (*devs_i)->getDeviceId();
                    }
                    devItems[devTree->AppendItem(manualBranch, devName)] = (*devs_i);
                } else if ((*devs_i)->isRemote()) {
                    devConfig = wxGetApp().getConfig()->getDevice((*devs_i)->getDeviceId());
                    devItems[devTree->AppendItem(dsBranch, devConfig->getDeviceName())] = (*devs_i);
                } else {
                    devConfig = wxGetApp().getConfig()->getDevice((*devs_i)->getDeviceId());
                    devItems[devTree->AppendItem(localBranch, devConfig->getDeviceName())] = (*devs_i);
                }
            }
        }
        
        std::vector<std::string> remotes = SDREnumerator::getRemotes();
       
        std::vector<SDRDeviceInfo *>::iterator remoteDevs_i;
        
        if (!remotes.empty()) {
            for (const std::string& remote : remotes) {
                devs[remote] = SDREnumerator::enumerate_devices(remote, true);
                DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(remote);

                wxTreeItemId remoteNode = devTree->AppendItem(remoteBranch, devConfig->getDeviceName());
                
                if (devs[remote] != nullptr) {
                    for (remoteDevs_i = devs[remote]->begin(); remoteDevs_i != devs[remote]->end(); remoteDevs_i++) {
                        devItems[devTree->AppendItem(remoteNode, (*remoteDevs_i)->getName())] = (*remoteDevs_i);
                    }
                }
            }
        }
        
        m_refreshButton->Enable();
        m_addRemoteButton->Enable();
        m_useSelectedButton->Enable();
        devTree->Enable();
        devTree->ExpandAll();
        
        devStatusBar->SetStatusText("Ready.");

        refresh = false;
    }
}

void SDRDevicesDialog::OnRefreshDevices( wxMouseEvent& /* event */) {
    doRefreshDevices();
}

std::string SDRDevicesDialog::getSelectedChoiceOption(wxPGProperty* prop, const SoapySDR::ArgInfo& arg) {

    std::string optionName;

    int choiceIndex = prop->GetChoiceSelection();
    
    if (!arg.options.empty()) {
        int choiceMax = arg.options.size();
        
        if (choiceIndex >= 0 && choiceIndex < choiceMax) {
            //normal selection
            optionName = arg.options[choiceIndex];
        } else {
            //choose the first one of the list:
            optionName = arg.options[0];
            prop->SetChoiceSelection(0);
        }
    } 

    return optionName;
}

void SDRDevicesDialog::OnPropGridChanged( wxPropertyGridEvent& event ) {

    if (event.GetProperty() == devSettings["name"]) {
        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
        
        wxString devName = event.GetPropertyValue().GetString();
        
        devConfig->setDeviceName(devName.ToStdString());
        if (editId) {
            devTree->SetItemText(editId, devConfig->getDeviceName());
        }
        if (devName.empty()) {
            event.GetProperty()->SetValueFromString(devConfig->getDeviceName());
        }
    } else if (dev && event.GetProperty() == devSettings["offset"]) {
        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
        
        long offset_In_KHz = event.GetPropertyValue().GetInteger();
        
        devConfig->setOffset((long long) offset_In_KHz * 1000);
        if (dev->isActive() || !wxGetApp().getDevice()) {

            wxGetApp().setOffset((long long)offset_In_KHz * 1000);
        }

    } 
    else if (dev && event.GetProperty() == devSettings["sample_rate"]) {

        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
        
        std::string strRate = deviceArgs["sample_rate"].options[event.GetPropertyValue().GetInteger()];
        long srate = 0;
        try {
            srate = std::stol(strRate);
            devConfig->setSampleRate(srate);
             if (dev->isActive() || !wxGetApp().getDevice()) {
                wxGetApp().setSampleRate(srate);
            }
        } catch (const std::invalid_argument &) {
            // nop
        }
    } else if (dev && event.GetProperty() == devSettings["antenna"]) {
        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());

        std::string strAntennaName = deviceArgs["antenna"].options[event.GetPropertyValue().GetInteger()];
        
        try {
            devConfig->setAntennaName(strAntennaName);

            if (dev->isActive() || !wxGetApp().getDevice()) { 
                wxGetApp().setAntennaName(strAntennaName);
            }
        }
        catch (const std::invalid_argument &) {
            // nop
        }
    }
    else if (dev) {
        wxPGProperty *prop = event.GetProperty();
        //change value of RuntimeProps
        for (auto & runtimeProp : runtimeProps) {
            if (runtimeProp.second == prop) {
                SoapySDR::Device *soapyDev = dev->getSoapyDevice();
                std::string settingValue = prop->GetValueAsString().ToStdString();
                SoapySDR::ArgInfo arg = runtimeArgs[runtimeProp.first];
                if (arg.type == SoapySDR::ArgInfo::STRING && !arg.options.empty()) {
                    settingValue = getSelectedChoiceOption(prop, arg);
                } else if (arg.type == SoapySDR::ArgInfo::BOOL) {
                    settingValue = (prop->GetValueAsString()=="True")?"true":"false";
                } else {
                    settingValue = prop->GetValueAsString();
                }

                soapyDev->writeSetting(runtimeProp.first, settingValue);
                if (dev->isActive()) {
                    wxGetApp().getSDRThread()->writeSetting(runtimeProp.first, settingValue);
                }
                return;
            }
        }
    }
}

void SDRDevicesDialog::OnPropGridFocus( wxFocusEvent& /* event */) {
    editId = selId;
}


void SDRDevicesDialog::doRefreshDevices() {
    selId = nullptr;
    editId = nullptr;
    removeId = nullptr;
    dev = nullptr;
    wxGetApp().stopDevice(false, 2000);
    devTree->DeleteAllItems();
    devTree->Disable();
    m_propertyGrid->Clear();
	devSettings.clear();
	runtimeArgs.clear();
	runtimeProps.clear();
	streamProps.clear();

    m_refreshButton->Disable();
    m_addRemoteButton->Disable();
    m_useSelectedButton->Disable();
    wxGetApp().reEnumerateDevices();
    refresh = true;
    m_addRemoteButton->SetLabel("Add");
}

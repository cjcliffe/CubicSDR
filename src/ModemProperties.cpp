// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemProperties.h"
#include "CubicSDR.h"

ModemProperties::ModemProperties(wxWindow *parent, wxWindowID winid,
         const wxPoint& pos, const wxSize& size, long style, const wxString& name) : wxPanel(parent, winid, pos, size, style, name) {
    
   	m_propertyGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE);

    bSizer = new wxBoxSizer( wxVERTICAL );
    
    bSizer->Add(m_propertyGrid, 1, wxEXPAND, 0);
    
    this->SetSizer(bSizer);
    
    m_propertyGrid->Connect( wxEVT_PG_ITEM_COLLAPSED, wxPropertyGridEventHandler( ModemProperties::OnCollapse ), nullptr, this );
    m_propertyGrid->Connect( wxEVT_PG_ITEM_EXPANDED, wxPropertyGridEventHandler( ModemProperties::OnExpand ), nullptr, this );
    m_propertyGrid->Connect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( ModemProperties::OnChange ), nullptr, this );
    this->Connect( wxEVT_SHOW, wxShowEventHandler( ModemProperties::OnShow ), nullptr, this );
    
    this->Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( ModemProperties::OnMouseEnter ), nullptr, this);
    this->Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( ModemProperties::OnMouseLeave ), nullptr, this);

    updateTheme();

    mouseInView = false;
    collapsed = false;
    syncingValues = false;
}

void ModemProperties::OnShow(wxShowEvent & /* event */) {
}

void ModemProperties::updateTheme() {
    wxColour bgColor(ThemeMgr::mgr.currentTheme->generalBackground);
    wxColour textColor(ThemeMgr::mgr.currentTheme->text);
    wxColour btn(ThemeMgr::mgr.currentTheme->button);
    wxColour btnHl(ThemeMgr::mgr.currentTheme->buttonHighlight);

    m_propertyGrid->SetEmptySpaceColour(bgColor);
    m_propertyGrid->SetCellBackgroundColour(bgColor);
    m_propertyGrid->SetCellTextColour(textColor);
    m_propertyGrid->SetSelectionTextColour(bgColor);
    m_propertyGrid->SetSelectionBackgroundColour(btnHl);
    m_propertyGrid->SetCaptionTextColour(bgColor);
    m_propertyGrid->SetCaptionBackgroundColour(btn);
    m_propertyGrid->SetLineColour(btn);
}

ModemProperties::~ModemProperties() = default;


void ModemProperties::initDefaultProperties() {
    
    if (audioOutputDevices.empty()) {
        std::vector<string> outputOpts;
        std::vector<string> outputOptNames;

        AudioThread::enumerateDevices(audioDevices);
        
        int i = 0;
        
        for (const auto& aDev : audioDevices) {
            if (aDev.inputChannels) {
                audioInputDevices[i] = aDev;
            }
            if (aDev.outputChannels) {
                audioOutputDevices[i] = aDev;
            }
            i++;
        }
        
        // int defaultDevice = 0;
        // int dc = 0;
        
        for (const auto& mdevices_i : audioOutputDevices) {
            outputOpts.push_back(std::to_string(mdevices_i.first));
            outputOptNames.push_back(mdevices_i.second.name);
            
            // if (mdevices_i.second.isDefaultOutput) {
            //     defaultDevice = dc;
            // }
            // dc++;
        }
        
        outputArg.key ="._audio_output";
        outputArg.name = "Audio Out";
        outputArg.description = "Set the current modem's audio output device.";
        outputArg.type = ModemArgInfo::Type::STRING;
        outputArg.options = outputOpts;
        outputArg.optionNames = outputOptNames;
    }
    
    int currentOutput = demodContext->getOutputDevice();

    outputArg.value = std::to_string(currentOutput);
    
    defaultProps["._audio_output"] = addArgInfoProperty(m_propertyGrid, outputArg);
}

static bool sameArgStructure(const ModemArgInfoList& a, const ModemArgInfoList& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); i++) {
        const ModemArgInfo& x = a[i];
        const ModemArgInfo& y = b[i];
        if (x.key != y.key || x.name != y.name || x.description != y.description
            || x.type != y.type || x.options != y.options || x.optionNames != y.optionNames
            || x.range.minimum() != y.range.minimum() || x.range.maximum() != y.range.maximum()) {
            return false;
        }
    }
    return true;
}

void ModemProperties::initProperties(ModemArgInfoList newArgs, const DemodulatorInstancePtr& demodInstance) {
    // Same type and argument structure: sync values in place instead of a
    // full grid rebuild (~1.4s), which every demodulator switch schedules.
    if (demodInstance && demodContext
        && demodContext->getDemodulatorType() == demodInstance->getDemodulatorType()
        && sameArgStructure(args, newArgs)) {
        syncPropertyValues(newArgs, demodInstance);
        return;
    }
    args = newArgs;
    demodContext = demodInstance;
    
    bSizer->Layout();
    m_propertyGrid->Clear();

    if (!demodInstance) {
        m_propertyGrid->Append(new wxPropertyCategory("Modem Settings"));
        return;
    }
    m_propertyGrid->Freeze();
    
    m_propertyGrid->Append(new wxPropertyCategory(demodInstance->getDemodulatorType() + " Settings"));
    
    initDefaultProperties();
    
    ModemArgInfoList::const_iterator args_i;
    
    for (args_i = args.begin(); args_i != args.end(); args_i++) {
        ModemArgInfo arg = (*args_i);
        props[arg.key] = addArgInfoProperty(m_propertyGrid, arg);
    }
    
    m_propertyGrid->FitColumns();
    
    if (collapsed) {
        m_propertyGrid->CollapseAll();
    }
    m_propertyGrid->Thaw();
}

void ModemProperties::syncPropertyValues(ModemArgInfoList newArgs, const DemodulatorInstancePtr& demodInstance) {
    demodContext = demodInstance;
    args = newArgs;

    // Programmatic sets must not echo back through OnChange into the modem.
    syncingValues = true;
    m_propertyGrid->Freeze();

    int currentOutput = demodContext->getOutputDevice();
    outputArg.value = std::to_string(currentOutput);
    auto outProp = defaultProps.find("._audio_output");
    if (outProp != defaultProps.end() && outProp->second) {
        for (size_t i = 0; i < outputArg.options.size(); i++) {
            if (outputArg.options[i] == outputArg.value) {
                outProp->second->SetChoiceSelection((int)i);
                break;
            }
        }
    }

    for (auto &arg : newArgs) {
        auto found = props.find(arg.key);
        if (found == props.end() || !found->second) {
            continue;
        }
        wxPGProperty *prop = found->second;
        switch (arg.type) {
            case ModemArgInfo::Type::INT: {
                int intVal = 0;
                try {
                    intVal = std::stoi(arg.value);
                } catch (const std::invalid_argument &) {
                }
                prop->SetValue(wxVariant((long)intVal));
                break;
            }
            case ModemArgInfo::Type::FLOAT: {
                double floatVal = 0;
                try {
                    floatVal = std::stod(arg.value);
                } catch (const std::invalid_argument &) {
                }
                prop->SetValue(wxVariant(floatVal));
                break;
            }
            case ModemArgInfo::Type::BOOL:
                prop->SetValue(wxVariant(arg.value == "true"));
                break;
            case ModemArgInfo::Type::STRING:
                if (!arg.options.empty()) {
                    for (size_t i = 0; i < arg.options.size(); i++) {
                        if (arg.options[i] == arg.value) {
                            prop->SetChoiceSelection((int)i);
                            break;
                        }
                    }
                } else {
                    prop->SetValue(wxVariant(wxString(arg.value)));
                }
                break;
            default:
                break;
        }
    }

    m_propertyGrid->Thaw();
    m_propertyGrid->Refresh();
    m_propertyGrid->FitColumns();
    syncingValues = false;
}

wxPGProperty *ModemProperties::addArgInfoProperty(wxPropertyGrid *pg, ModemArgInfo arg) {
    wxPGProperty *prop = nullptr;
    
    int intVal;
    double floatVal;
    std::vector<std::string>::iterator stringIter;
    
    switch (arg.type) {
        case ModemArgInfo::Type::INT:
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
        case ModemArgInfo::Type::FLOAT:
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
        case ModemArgInfo::Type::BOOL:
            prop = pg->Append( new wxBoolProperty(arg.name, wxPG_LABEL, (arg.value=="true")) );
            break;
        case ModemArgInfo::Type::STRING:
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
        case ModemArgInfo::Type::PATH_DIR:
            break;
        case ModemArgInfo::Type::PATH_FILE:
            break;
        case ModemArgInfo::Type::COLOR:
            break;
    }
    
    if (prop != nullptr) {
        prop->SetHelpString(arg.name + ": " + arg.description);
    }
    
    return prop;
}

std::string ModemProperties::readProperty(const std::string& key) {
    int i = 0;
    ModemArgInfoList::const_iterator args_i;

    for (args_i = args.begin(); args_i != args.end(); args_i++) {
        ModemArgInfo arg = (*args_i);
        if (arg.key == key) {
            wxPGProperty *prop = props[key];
            
            std::string result;
            if (arg.type == ModemArgInfo::Type::STRING && !arg.options.empty()) {
                return arg.options[prop->GetChoiceSelection()];
            } else if (arg.type == ModemArgInfo::Type::BOOL) {
                return (prop->GetValueAsString()=="True")?"true":"false";
            } else {
                return prop->GetValueAsString().ToStdString();
            }
        }
        i++;
    }
    return "";
}

void ModemProperties::OnChange(wxPropertyGridEvent &event) {
    if (syncingValues) {
        return;
    }
    if (!demodContext || !demodContext->isActive()) {
        return;
    }
    
    std::map<std::string, wxPGProperty *>::const_iterator prop_i;
    
    if (event.m_property == defaultProps["._audio_output"]) {
        int sel = event.m_property->GetChoiceSelection();
        
        outputArg.value = outputArg.options[sel];
        
        if (demodContext) {
            try {
                demodContext->setOutputDevice(std::stoi(outputArg.value));
            } catch (const exception &) {
                // .. this should never happen ;)
            }

            wxGetApp().getAppFrame()->setScopeDeviceName(outputArg.optionNames[sel]);
        }
        
        return;
    }
    
    for (prop_i = props.begin(); prop_i != props.end(); prop_i++) {
        if (prop_i->second == event.m_property) {
            std::string key = prop_i->first;
            std::string value = readProperty(prop_i->first);
            demodContext->writeModemSetting(key, value);
            return;
        }
    }
}

void ModemProperties::OnCollapse(wxPropertyGridEvent & /* event */) {
    collapsed = true;
}

void ModemProperties::OnExpand(wxPropertyGridEvent &/* event */) {
    collapsed = false;
}

void ModemProperties::OnMouseEnter(wxMouseEvent & /* event */) {
    mouseInView = true;
}

void ModemProperties::OnMouseLeave(wxMouseEvent & /* event */) {
    mouseInView = false;
}

bool ModemProperties::isMouseInView() {
    return mouseInView || (m_propertyGrid && m_propertyGrid->IsEditorFocused());
}

void ModemProperties::setCollapsed(bool state) {
    collapsed = state;
    if (m_propertyGrid) {
        if (state) {
            m_propertyGrid->CollapseAll();
        } else {
            m_propertyGrid->ExpandAll();
        }
    }
}

bool ModemProperties::isCollapsed() const {
    return collapsed;
}

void ModemProperties::fitColumns() {
    m_propertyGrid->FitColumns();
}

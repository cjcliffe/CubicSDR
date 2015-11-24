#include "ModemProperties.h"

ModemProperties::ModemProperties(wxWindow *parent, wxWindowID winid,
         const wxPoint& pos, const wxSize& size, long style, const wxString& name) : wxPanel(parent, winid, pos, size, style, name) {
    
   	m_propertyGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE);
    
    wxBoxSizer* bSizer;

    bSizer = new wxBoxSizer( wxVERTICAL );
    
    bSizer->Add(m_propertyGrid, wxEXPAND );
    
    this->SetSizer(bSizer);
}

ModemProperties::~ModemProperties() {
    
}

void ModemProperties::initProperties(ModemArgInfoList newArgs) {
    args = newArgs;
    
//    props.erase(props.begin(), props.end());

    m_propertyGrid->Clear();
    m_propertyGrid->Append(new wxPropertyCategory("Modem Settings"));
    
    ModemArgInfoList::const_iterator args_i;
    
    for (args_i = args.begin(); args_i != args.end(); args_i++) {
        ModemArgInfo arg = (*args_i);
        props.push_back(addArgInfoProperty(m_propertyGrid, arg));
    }
}

wxPGProperty *ModemProperties::addArgInfoProperty(wxPropertyGrid *pg, ModemArgInfo arg) {
    wxPGProperty *prop = nullptr;
    
    int intVal;
    double floatVal;
    std::vector<std::string>::iterator stringIter;
    
    switch (arg.type) {
        case ModemArgInfo::INT:
            try {
                intVal = std::stoi(arg.value);
            } catch (std::invalid_argument e) {
                intVal = 0;
            }
            prop = pg->Append( new wxIntProperty(arg.name, wxPG_LABEL, intVal) );
            if (arg.range.minimum() != arg.range.maximum()) {
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MIN, arg.range.minimum());
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MAX, arg.range.maximum());
            }
            break;
        case ModemArgInfo::FLOAT:
            try {
                floatVal = std::stod(arg.value);
            } catch (std::invalid_argument e) {
                floatVal = 0;
            }
            prop = pg->Append( new wxFloatProperty(arg.name, wxPG_LABEL, floatVal) );
            if (arg.range.minimum() != arg.range.maximum()) {
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MIN, arg.range.minimum());
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MAX, arg.range.maximum());
            }
            break;
        case ModemArgInfo::BOOL:
            prop = pg->Append( new wxBoolProperty(arg.name, wxPG_LABEL, (arg.value=="true")) );
            break;
        case ModemArgInfo::STRING:
            if (arg.options.size()) {
                intVal = 0;
                prop = pg->Append( new wxEnumProperty(arg.name, wxPG_LABEL) );
                for (stringIter = arg.options.begin(); stringIter != arg.options.end(); stringIter++) {
                    std::string optName = (*stringIter);
                    std::string displayName = optName;
                    if (arg.optionNames.size()) {
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
        case ModemArgInfo::PATH_DIR:
            break;
        case ModemArgInfo::PATH_FILE:
            break;
        case ModemArgInfo::COLOR:
            break;
    }
    
    if (prop != NULL) {
        prop->SetHelpString(arg.key + ": " + arg.description);
    }
    
    return prop;
}


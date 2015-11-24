#pragma once

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

#include "Modem.h"

class ModemProperties : public wxPanel {
public:
    ModemProperties(
        wxWindow *parent,
        wxWindowID winid = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL | wxNO_BORDER,
        const wxString& name = wxPanelNameStr
    );
    ~ModemProperties();
    
    void initProperties(ModemArgInfoList newArgs);
    
private:
    wxPGProperty *addArgInfoProperty(wxPropertyGrid *pg, ModemArgInfo arg);

    wxPropertyGrid* m_propertyGrid;
    ModemArgInfoList args;
    std::vector<wxPGProperty *> props;
};
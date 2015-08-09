#pragma once

#include "GLPanel.h"

class ScopePanel : public GLPanel {
    
public:
    typedef enum ScopeMode { SCOPE_MODE_Y, SCOPE_MODE_2Y, SCOPE_MODE_XY } ScopeMode;
    
    ScopePanel();
    
    void setMode(ScopeMode scopeMode);
    void setPoints(std::vector<float> &points);
    void drawPanelContents();
    
    std::vector<float> points;
    ScopeMode scopeMode;
    GLPanel bgPanel;
    GLPanel bgPanelStereo[2];
};
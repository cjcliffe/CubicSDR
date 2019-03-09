// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "GLPanel.h"

class MeterPanel : public GLPanel {
    
public:
    MeterPanel(std::string name, float low, float high, float current);
    ~MeterPanel();
    void setName(std::string name_in);
    std::string getName();
    void setRange(float low, float high);
    float getLow();
    float getHigh();
    void setValue(float value);
    void setHighlight(float value);
    void setHighlightVisible(bool vis);
    float getValue();
    bool isMeterHit(CubicVR::vec2 mousePoint);
    float getMeterHitValue(CubicVR::vec2 mousePoint);
    void setChanged(bool changed);
    bool getChanged();
    
protected:
    void drawPanelContents();
    void setValueLabel(std::string label);
    void setPanelLevel(float setValue, GLPanel &panel);
    
private:
    std::string name;
    float low, high, current;
    bool changed = false;
    GLPanel bgPanel;
    GLPanel levelPanel;
    GLPanel highlightPanel;
    GLTextPanel labelPanel;
    GLTextPanel valuePanel;
};
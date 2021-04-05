// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "GLPanel.h"

class MeterPanel : public GLPanel {
    
public:
    MeterPanel(const std::string& name, float low, float high, float current);
    ~MeterPanel() override;
    void setName(std::string name_in);
    std::string getName();
    void setRange(float low_in, float high_in);
    float getLow() const;
    float getHigh() const;
    void setValue(float value);
    void setHighlight(float value);
    void setHighlightVisible(bool vis);
    float getValue() const;
    bool isMeterHit(CubicVR::vec2 mousePoint);
    float getMeterHitValue(CubicVR::vec2 mousePoint);
    void setChanged(bool changed_in);
    bool getChanged() const;
    
protected:
    void drawPanelContents() override;
    void setValueLabel(std::string label);
    void setPanelLevel(float setValue, GLPanel &panel) const;
    
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
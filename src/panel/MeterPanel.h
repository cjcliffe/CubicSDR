#pragma once

#include "GLPanel.h"

class MeterPanel : public GLPanel {
    
public:
    MeterPanel(std::string name, float low, float high, float current);
    void setName(std::string name_in);
    void setRange(float low, float high);
    void setValue(float value);
    void setHighlight(float value);
    float getValue();
    bool isMeterHit(CubicVR::vec2 mousePoint);
    float getMeterHitValue(CubicVR::vec2 mousePoint, GLPanel &panel);
    
protected:
    void drawPanelContents();
    void setValueLabel(std::string label);
    void setPanelLevel(float setValue, GLPanel &panel);
    
private:
    std::string name;
    float low, high, current;
    GLPanel panel;
    GLPanel bgPanel;
    GLPanel levelPanel;
    GLPanel highlightPanel;
    GLTextPanel labelPanel;
    GLTextPanel valuePanel;
};
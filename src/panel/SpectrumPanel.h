#pragma once

#include "GLPanel.h"

class SpectrumPanel : public GLPanel {
public:
    SpectrumPanel();
    
    void setPoints(std::vector<float> &points);
    
    float getFloorValue() const;
    void setFloorValue(float floorValue);

    float getCeilValue() const;
    void setCeilValue(float ceilValue);
    
    void setFreq(long long freq);
    long long getFreq();
    
    void setBandwidth(long long bandwidth);
    long long getBandwidth();
    
protected:
    void drawPanelContents();

private:
    float floorValue, ceilValue;
    long long freq;
    long long bandwidth;
    std::vector<float> points;
};
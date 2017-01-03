// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "GLPanel.h"

class SpectrumPanel : public GLPanel {
public:
    SpectrumPanel();
    
    void setPoints(std::vector<float> &points);
    void setPeakPoints(std::vector<float> &points);
    
    float getFloorValue();
    void setFloorValue(float floorValue);

    float getCeilValue();
    void setCeilValue(float ceilValue);
    
    void setFreq(long long freq);
    long long getFreq();
    
    void setBandwidth(long long bandwidth);
    long long getBandwidth();

    void setFFTSize(int fftSize_in);
    int getFFTSize();

    void setShowDb(bool showDb);
    bool getShowDb();

    void setUseDBOffset(bool useOfs);
    bool getUseDBOffset();

protected:
    void drawPanelContents();

private:
    float floorValue, ceilValue;
    int fftSize;
    long long freq;
    long long bandwidth;
    std::vector<float> points;
    std::vector<float> peak_points;
    
    GLTextPanel dbPanelCeil;
    GLTextPanel dbPanelFloor;
    bool showDb, useDbOfs;
};

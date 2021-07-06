// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "GLPanel.h"

class SpectrumPanel : public GLPanel {
public:
    SpectrumPanel();
    
    void setPoints(std::vector<float> &points_in);
    void setPeakPoints(std::vector<float> &points_in);
    
    float getFloorValue() const;
    void setFloorValue(float floorValue_in);

    float getCeilValue() const;
    void setCeilValue(float ceilValue_in);
    
    void setFreq(long long freq_in);
    long long getFreq() const;
    
    void setBandwidth(long long bandwidth_in);
    long long getBandwidth() const;

    void setFFTSize(int fftSize_in);
    int getFFTSize() const;

    void setShowDb(bool showDb_in);
    bool getShowDb() const;

    void setUseDBOffset(bool useOfs);
    bool getUseDBOffset() const;

protected:
    void drawPanelContents() override;

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

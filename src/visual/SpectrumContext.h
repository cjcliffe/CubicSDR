#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

#define NUM_WATERFALL_LINES 512

class SpectrumCanvas;

class SpectrumContext: public PrimaryGLContext {
public:
    SpectrumContext(SpectrumCanvas *canvas, wxGLContext *sharedContext);

    void Draw(std::vector<float> &points, long long freq, int bandwidth);

    float getFloorValue() const;
    void setFloorValue(float floorValue);
    float getCeilValue() const;
    void setCeilValue(float ceilValue);

private:
    int fft_size;
    float floorValue, ceilValue;
};

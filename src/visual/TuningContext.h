// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

class TuningCanvas;

class TuningContext: public PrimaryGLContext {
public:
    TuningContext(TuningCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs *ctxAttrs);

    void DrawBegin();
    void Draw(float r, float g, float b, float a, float p1, float p2);
    void DrawTuner(long long freq, int count, float displayPos, float displayWidth);
    static void DrawTunerDigitBox(int index, int count, float displayPos, float displayWidth, const RGBA4f& c);
    int GetTunerDigitIndex(float mPos, int count, float displayPos, float displayWidth);
    void DrawTunerBarIndexed(int start, int end, int count, float displayPos, float displayWidth, const RGBA4f& color, float alpha, bool top, bool bottom);

    void DrawDemodFreqBw(long long freq, unsigned int bw, long long center);
    void DrawEnd();

private:
    std::locale comma_locale;
    std::stringstream freqStr;
    std::stringstream freqStrFormatted;
};

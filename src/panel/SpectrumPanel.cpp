// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SpectrumPanel.h"

#include <iostream>
#include <iomanip>
#include "CubicSDR.h"
#include "ColorTheme.h"
#include "CubicSDRDefs.h"
#include "SpectrumTicks.h"

SpectrumPanel::SpectrumPanel() {
    floorValue = 0;
    ceilValue = 1;
    showDb = true;
    useDbOfs = true;
    fftSize = DEFAULT_FFT_SIZE;
    bandwidth = DEFAULT_DEMOD_BW;
    freq = 0;
    
    setFill(GLPANEL_FILL_GRAD_Y);
    setFillColor(ThemeMgr::mgr.currentTheme->fftBackground * 2.0, ThemeMgr::mgr.currentTheme->fftBackground);
    
    dbPanelCeil.setMarginPx(0);
    dbPanelCeil.setFill(GLPanel::GLPANEL_FILL_GRAD_X);
    dbPanelCeil.setFillColor(RGBA4f(0.2f,0.2f,0.2f,5.0f), RGBA4f(0.2f,0.2f,0.2f,0.0f));
    
    dbPanelFloor.setMarginPx(0);
    dbPanelFloor.setFill(GLPanel::GLPANEL_FILL_GRAD_X);
    dbPanelFloor.setFillColor(RGBA4f(0.2f,0.2f,0.2f,5.0f), RGBA4f(0.2f,0.2f,0.2f,0.0f));
}


float SpectrumPanel::getFloorValue() const {
    return floorValue;
}

void SpectrumPanel::setFloorValue(float floorValue_in) {
    floorValue = floorValue_in;
}

float SpectrumPanel::getCeilValue() const {
    return ceilValue;
}

void SpectrumPanel::setCeilValue(float ceilValue_in) {
    ceilValue = ceilValue_in;
}

void SpectrumPanel::setFreq(long long freq_in) {
    freq = freq_in;
}

long long SpectrumPanel::getFreq() const {
    return freq;
}

void SpectrumPanel::setBandwidth(long long bandwidth_in) {
    bandwidth = bandwidth_in;
}

long long SpectrumPanel::getBandwidth() const {
    return bandwidth;
}

void SpectrumPanel::setFFTSize(int fftSize_in) {
    this->fftSize = fftSize_in;
}

int SpectrumPanel::getFFTSize() const {
    return fftSize;
}

void SpectrumPanel::setShowDb(bool showDb_in) {
    showDb = showDb_in;
    if (showDb_in) {
        addChild(&dbPanelCeil);
        addChild(&dbPanelFloor);
    } else {
        removeChild(&dbPanelCeil);
        removeChild(&dbPanelFloor);
    }
    
}

bool SpectrumPanel::getShowDb() const {
    return showDb;
}

void SpectrumPanel::setUseDBOffset(bool useOfs) {
    this->useDbOfs = useOfs;
}

bool SpectrumPanel::getUseDBOffset() const {
    return useDbOfs;
}


void SpectrumPanel::setPoints(std::vector<float> &points_in) {
    points.assign(points_in.begin(), points_in.end());
    fill_points = SpectrumTicks::fillStrip(points);
}

void SpectrumPanel::setPeakPoints(std::vector<float> &points_in) {
    peak_points.assign(points_in.begin(), points_in.end());
}


void SpectrumPanel::drawPanelContents() {
    glDisable(GL_TEXTURE_2D);
    
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );

    glLoadMatrixf((transform * (CubicVR::mat4::translate(-1.0f, -0.75f, 0.0f) * CubicVR::mat4::scale(2.0f, 1.5f, 1.0f))).to_ptr());

    if (!points.empty()) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        SpectrumTicks::GridGeometry grid = SpectrumTicks::gridGeometry(floorValue, ceilValue);
        glEnableClientState(GL_VERTEX_ARRAY);
        for (auto &band : grid.bands) {
            if (!band.verts.empty()) {
                glColor4f(0.12f, 0.12f, 0.12f, band.alpha);
                glVertexPointer(2, GL_FLOAT, 0, &band.verts[0]);
                glDrawArrays(GL_LINES, 0, band.verts.size() / 2);
            }
        }
        
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(ThemeMgr::mgr.currentTheme->fftFill.r, ThemeMgr::mgr.currentTheme->fftFill.g, ThemeMgr::mgr.currentTheme->fftFill.b, ThemeMgr::mgr.currentTheme->fftFill.a);
        glVertexPointer(2, GL_FLOAT, 0, &fill_points[0]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, fill_points.size() / 2);
        float fillBase = (-1.0f + 0.75f) / 1.5f;
        float skirtVerts[8] = { 0, 0, 0, fillBase, 1, fillBase, 1, 0 };
        float skirtColors[16] = {
            ThemeMgr::mgr.currentTheme->fftFill.r, ThemeMgr::mgr.currentTheme->fftFill.g, ThemeMgr::mgr.currentTheme->fftFill.b, ThemeMgr::mgr.currentTheme->fftFill.a,
            ThemeMgr::mgr.currentTheme->fftBackground.r, ThemeMgr::mgr.currentTheme->fftBackground.g, ThemeMgr::mgr.currentTheme->fftBackground.b, ThemeMgr::mgr.currentTheme->fftFill.a,
            ThemeMgr::mgr.currentTheme->fftBackground.r, ThemeMgr::mgr.currentTheme->fftBackground.g, ThemeMgr::mgr.currentTheme->fftBackground.b, ThemeMgr::mgr.currentTheme->fftFill.a,
            ThemeMgr::mgr.currentTheme->fftFill.r, ThemeMgr::mgr.currentTheme->fftFill.g, ThemeMgr::mgr.currentTheme->fftFill.b, ThemeMgr::mgr.currentTheme->fftFill.a };
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, skirtVerts);
        glColorPointer(4, GL_FLOAT, 0, skirtColors);
        glDrawArrays(GL_QUADS, 0, 4);
        glDisableClientState(GL_COLOR_ARRAY);
        glColor3f(ThemeMgr::mgr.currentTheme->fftLine.r, ThemeMgr::mgr.currentTheme->fftLine.g, ThemeMgr::mgr.currentTheme->fftLine.b);
        glVertexPointer(2, GL_FLOAT, 0, &points[0]);
        glDrawArrays(GL_LINE_STRIP, 0, points.size() / 2);
        if (!peak_points.empty()) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0, 1.0, 0, 0.5);
            glVertexPointer(2, GL_FLOAT, 0, &peak_points[0]);
            glDrawArrays(GL_LINE_STRIP, 0, peak_points.size() / 2);
        }
        glDisableClientState(GL_VERTEX_ARRAY);
    }
  
    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);
    
    float viewHeight = (float) vp[3];
    float viewWidth = (float) vp[2];
    glLoadMatrixf(transform.to_ptr());
    
    SpectrumTicks::AxisGeometry axis = SpectrumTicks::axisGeometry(freq, bandwidth, viewWidth, viewHeight, GLFont::getScaleFactor());
    
    GLFont::Drawer refDrawingFont = GLFont::getFont(axis.fontSize, GLFont::getScaleFactor());

    std::vector<float> majorTicks, minorTicks;
    for (auto &tick : axis.ticks) {
        std::vector<float> &tickVerts = tick.major ? majorTicks : minorTicks;
        tickVerts.push_back((float)tick.x);
        tickVerts.push_back((float)axis.tickBase);
        tickVerts.push_back((float)tick.x);
        tickVerts.push_back(1.0f);
        
        glColor4f(ThemeMgr::mgr.currentTheme->text.r, ThemeMgr::mgr.currentTheme->text.g, ThemeMgr::mgr.currentTheme->text.b, 1.0);
        
        refDrawingFont.drawString(tick.label, tick.x, axis.hPos, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
    }
    
    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    if (!majorTicks.empty()) {
        glLineWidth(4.0);
        glColor3f(ThemeMgr::mgr.currentTheme->freqLine.r, ThemeMgr::mgr.currentTheme->freqLine.g, ThemeMgr::mgr.currentTheme->freqLine.b);
        glVertexPointer(2, GL_FLOAT, 0, &majorTicks[0]);
        glDrawArrays(GL_LINES, 0, majorTicks.size() / 2);
    }
    if (!minorTicks.empty()) {
        glLineWidth(1.0);
        glColor3f(ThemeMgr::mgr.currentTheme->freqLine.r * 0.65, ThemeMgr::mgr.currentTheme->freqLine.g * 0.65, ThemeMgr::mgr.currentTheme->freqLine.b * 0.65);
        glVertexPointer(2, GL_FLOAT, 0, &minorTicks[0]);
        glDrawArrays(GL_LINES, 0, minorTicks.size() / 2);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glLineWidth(1.0);

    if (showDb) {
        float dbPanelWidth = (1.0 / viewWidth)*88.0 * GLFont::getScaleFactor();
        float dbPanelHeight = (1.0/viewHeight)*14.0 * GLFont::getScaleFactor();
        float dbOfs = useDbOfs?wxGetApp().getConfig()->getDBOffset():0;
        
        SpectrumTicks::DbLabels db = SpectrumTicks::dbLabels(getCeilValue(), getFloorValue(), fftSize, dbOfs);
        dbPanelCeil.setText(db.ceil, GLFont::GLFONT_ALIGN_RIGHT);
        dbPanelCeil.setSize(dbPanelWidth, dbPanelHeight);
        dbPanelCeil.setPosition(-1.0 + dbPanelWidth, 1.0 - dbPanelHeight);
        
        dbPanelFloor.setText(db.floor, GLFont::GLFONT_ALIGN_RIGHT);
        dbPanelFloor.setSize(dbPanelWidth, dbPanelHeight);
        dbPanelFloor.setPosition(-1.0 + dbPanelWidth, - 1.0 + dbPanelHeight);
    }
}

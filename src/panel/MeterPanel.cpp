// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "MeterPanel.h"
#include "ColorTheme.h"


MeterPanel::MeterPanel(const std::string& name, float low, float high, float current) {
    this->name = name;
    this->low = low;
    this->high = high;
    this->current = current;
    
    RGBA4f c1, c2;
    
    setFill(GLPanel::GLPANEL_FILL_NONE);
    
    bgPanel.setBorderPx(1);
    bgPanel.setCoordinateSystem(GLPanel::GLPANEL_Y_UP);
    bgPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_X);
    
    levelPanel.setBorderPx(0);
    levelPanel.setMarginPx(1);
    
    setPanelLevel(current, levelPanel);
    levelPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_BAR_X);
    levelPanel.setBlend(GL_ONE, GL_ONE);
    
    bgPanel.addChild(&levelPanel);
    
    setPanelLevel(current, highlightPanel);
    highlightPanel.setBorderPx(0);
    highlightPanel.setMarginPx(1);
    highlightPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_BAR_X);
    highlightPanel.setBlend(GL_ONE, GL_ONE);
    highlightPanel.visible = false;
    c1 = RGBA4f(0.3f,0.3f,0.3f,1.0f);
    c2 = RGBA4f(0.65f,0.65f,0.65f,1.0f);
    highlightPanel.setFillColor(c1, c2);
    
    bgPanel.addChild(&highlightPanel);
    
    addChild(&bgPanel);
    
    labelPanel.setSize(1.0f, 0.1f);
    labelPanel.setPosition(0.0, 1.0);
    labelPanel.setText(name,GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, true);
    labelPanel.setFill(GLPanel::GLPANEL_FILL_NONE);
    
    addChild(&labelPanel);
    
    valuePanel.setSize(1.0f, 0.1f);
    valuePanel.setPosition(0.0, -1.0);
    
    setValueLabel(std::to_string(int(current)));
    valuePanel.setFill(GLPanel::GLPANEL_FILL_NONE);
    
    addChild(&valuePanel);
}

MeterPanel::~MeterPanel() = default;


void MeterPanel::setName(std::string name_in) {
    name = name_in;
}

std::string MeterPanel::getName() {
    return name;
}

void MeterPanel::setRange(float low_in, float high_in) {
    low = low_in;
    high = high_in;
}

float MeterPanel::getLow() const {
    return low;
}

float MeterPanel::getHigh() const {
    return high;
}

void MeterPanel::setValue(float value) {
    if (value > high) {
        value = high;
    }
    if (value < low) {
        value = low;
    }
    
    current = value;
    setValueLabel(std::to_string(int(current)));
    setPanelLevel(value, levelPanel);
}

void MeterPanel::setHighlight(float value) {
    if (value > high) {
        value = high;
    }
    if (value < low) {
        value = low;
    }
    
    setPanelLevel(value, highlightPanel);
}

void MeterPanel::setHighlightVisible(bool vis) {
    highlightPanel.visible = vis;
}

float MeterPanel::getValue() const {
    return current;
}

bool MeterPanel::isMeterHit(CubicVR::vec2 mousePoint) {
    CubicVR::vec2 hitResult;
    
    if (bgPanel.hitTest(mousePoint, hitResult)) {
        return true;
    }
    
    return false;
}

float MeterPanel::getMeterHitValue(CubicVR::vec2 mousePoint) {
    CubicVR::vec2 hitResult;
    
    if (bgPanel.hitTest(mousePoint, hitResult)) {
        float hitLevel = ((hitResult.y + 1.0) * 0.5);
        
        if (hitLevel < 0.0f) {
            hitLevel = 0.0f;
        }
        if (hitLevel > 1.0f) {
            hitLevel = 1.0f;
        }
        
        return low + (hitLevel * (high-low));
    } else {
        return 0;
    }
}

void MeterPanel::drawPanelContents() {
    GLint vp[4];

    glGetIntegerv( GL_VIEWPORT, vp);
    
    double viewHeight = vp[3];
    
    CubicVR::vec4 t = CubicVR::mat4::vec4_multiply(CubicVR::vec4(0,0.5,0,1), transform);
    CubicVR::vec4 b = CubicVR::mat4::vec4_multiply(CubicVR::vec4(0,-0.5,0,1), transform);
    
    double hScale = t.y-b.y;
    
    viewHeight = viewHeight * hScale;
    
    double labelHeight = GLFont::getScaledPx(18, GLFont::getScaleFactor());
    double labelPad = 8.0;
    
    double pScale = (1.0/viewHeight);
    RGBA4f c1, c2;
    
    bgPanel.setSize(1.0f, 1.0 - pScale * (labelHeight + labelPad * 2.0));
    
    valuePanel.setPosition(0.0, (pScale * (labelHeight / 2.0 + labelPad) ) - 1.0);
    valuePanel.setSize(1.0, pScale*labelHeight);
    
    labelPanel.setPosition(0.0, 1.0 - (pScale * (labelHeight / 2.0 + labelPad)));
    labelPanel.setSize(1.0, pScale*labelHeight);

    c1 = ThemeMgr::mgr.currentTheme->generalBackground;
    c2 = ThemeMgr::mgr.currentTheme->generalBackground * 0.5;
    c1.a = 1.0;
    c2.a = 1.0;
    bgPanel.setFillColor(c1, c2);

    c1 = ThemeMgr::mgr.currentTheme->meterLevel * 0.5;
    c2 = ThemeMgr::mgr.currentTheme->meterLevel;
    c1.a = 1.0;
    c2.a = 1.0;
    levelPanel.setFillColor(c1, c2);
    
    drawChildren();
}

void MeterPanel::setValueLabel(std::string label) {
    valuePanel.setText(label,
                       GLFont::GLFONT_ALIGN_CENTER,
                       GLFont::GLFONT_ALIGN_CENTER,
                       true);
    
}

void MeterPanel::setPanelLevel(float setValue, GLPanel &panel) const {
    float valueNorm = (setValue - low) / (high - low);
    panel.setSize(1.0, valueNorm);
    panel.setPosition(0.0, (-1.0+(valueNorm)));
}

bool MeterPanel::getChanged() const {
    return changed;
}

void MeterPanel::setChanged(bool changed_in) {
    changed = changed_in;
}

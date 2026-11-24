// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "MeterPanel.h"
#include "ColorTheme.h"
#include "GainSlider.h"


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
    value = GainSlider::clampGain(value, low, high);
    
    current = value;
    setValueLabel(GainSlider::gainLabelText(current));
    setPanelLevel(value, levelPanel);
}

void MeterPanel::setHighlight(float value) {
    value = GainSlider::clampGain(value, low, high);
    
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
        return GainSlider::hitValue(hitResult.y, low, high);
    } else {
        return 0;
    }
}

void MeterPanel::drawPanelContents() {
    GLint vp[4];

    glGetIntegerv( GL_VIEWPORT, vp);
    
    double viewHeightPx = vp[3];
    
    CubicVR::vec4 t = CubicVR::mat4::vec4_multiply(CubicVR::vec4(0,0.5,0,1), transform);
    CubicVR::vec4 b = CubicVR::mat4::vec4_multiply(CubicVR::vec4(0,-0.5,0,1), transform);
    
    double hScale = t.y-b.y;
    
    double labelHeight = GLFont::getScaledPx(18, GLFont::getScaleFactor());
    
    GainSlider::LabelLayout layout = GainSlider::meterLabelLayout(viewHeightPx, hScale, labelHeight);
    
    RGBA4f c1, c2;

    bgPanel.setSize(1.0f, layout.bgH);
    
    valuePanel.setPosition(0.0, layout.valY);
    valuePanel.setSize(1.0, layout.valH);
    
    labelPanel.setPosition(0.0, layout.labY);
    labelPanel.setSize(1.0, layout.labH);

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
    float valueNorm = GainSlider::levelNorm(setValue, low, high);
    panel.setSize(1.0, valueNorm);
    panel.setPosition(0.0, (-1.0+(valueNorm)));
}

bool MeterPanel::getChanged() const {
    return changed;
}

void MeterPanel::setChanged(bool changed_in) {
    changed = changed_in;
}

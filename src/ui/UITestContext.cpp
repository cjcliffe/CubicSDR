// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "UITestContext.h"
#include "UITestCanvas.h"
#include "ColorTheme.h"

UITestContext::UITestContext(UITestCanvas *canvas, wxGLContext *sharedContext) :
PrimaryGLContext(canvas, sharedContext), testMeter("TEST",0,100,50) {
    
    testPanel.setPosition(0.0, 0.0);
    testPanel.setSize(1.0, 1.0);
    testPanel.setMarginPx(10);
    testPanel.setFill(GLPanel::GLPANEL_FILL_SOLID);
    testPanel.setFillColor(RGBA4f(0.0,0.0,1.0));
    
    testChildPanel.setPosition(0.0, 0.0);
    testChildPanel.setMarginPx(5);
    testChildPanel.setSize(1.0f, 0.33f);
    testChildPanel.setCoordinateSystem(GLPanel::GLPANEL_Y_DOWN_ZERO_ONE);
    testChildPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_BAR_X);
    testChildPanel.setFillColor(RGBA4f(0.0,0.0,1.0), RGBA4f(0.0,1.0,0.0));
    testChildPanel.setBorderPx(1);

    testChildPanel2.setPosition(0.0f, -0.66f);
    testChildPanel2.setSize(1.0f, 0.33f);
    testChildPanel2.setMarginPx(5);
    testChildPanel2.setFill(GLPanel::GLPANEL_FILL_GRAD_X);
    testChildPanel2.setFillColor(RGBA4f(0.0,0.0,1.0), RGBA4f(0.0,1.0,0.0));
    testChildPanel2.setBorderColor(RGBA4f(1.0,0.0,0.0));
    testChildPanel2.setBorderPx(1);

    testChildPanel3.setPosition(0.0f, 0.66f);
    testChildPanel3.setSize(1.0f, 0.33f);
    testChildPanel3.setMarginPx(5);
    testChildPanel3.setFill(GLPanel::GLPANEL_FILL_GRAD_X);
    testChildPanel3.setFillColor(RGBA4f(0.0,0.0,1.0), RGBA4f(0.0,1.0,0.0));
    testChildPanel3.setBorderColor(RGBA4f(1.0,0.0,0.0));
    testChildPanel3.setBorderPx(1);

    testText1.setText("Testing 123..");
    testText1.setFill(GLPanel::GLPANEL_FILL_NONE);
    testChildPanel2.addChild(&testText1);
    
//    testPanel.addChild(&testChildPanel);
//    testPanel.addChild(&testChildPanel2);
//    testPanel.addChild(&testChildPanel3);
    testMeter.setSize(0.1f,0.9f);
    testPanel.addChild(&testMeter);
}

void UITestContext::DrawBegin() {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    
    glClearColor(ThemeMgr::mgr.currentTheme->generalBackground.r, ThemeMgr::mgr.currentTheme->generalBackground.g, ThemeMgr::mgr.currentTheme->generalBackground.b, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glDisable(GL_TEXTURE_2D);
}

void UITestContext::Draw() {
    testPanel.calcTransform(CubicVR::mat4::identity());
    testPanel.draw();
}

void UITestContext::DrawEnd() {
//    glFlush();
    
//    CheckGLError();
}


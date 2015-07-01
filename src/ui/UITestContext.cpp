#include "UITestContext.h"
#include "UITestCanvas.h"
#include "ColorTheme.h"

UITestContext::UITestContext(UITestCanvas *canvas, wxGLContext *sharedContext) :
PrimaryGLContext(canvas, sharedContext) {
    
    testPanel.setPosition(0.0, 0.0);
    testPanel.setSize(1.0, 1.0);
    testPanel.setMarginPx(10);
    testPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_BAR_Y);
    testPanel.setFillColor(RGB(0.0,0.0,1.0), RGB(0.0,1.0,0.0));
    
    testChildPanel.setPosition(0.0, 0.0);
    testChildPanel.setMarginPx(10,10,10,5);
    testChildPanel.setSize(1.0, 0.5);
    testChildPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_BAR_X);
    testChildPanel.setFillColor(RGB(0.0,0.0,1.0), RGB(0.0,1.0,0.0));
    testChildPanel.setBorderPx(1);

    testChildPanel2.setPosition(0.0, 0.5);
    testChildPanel2.setSize(1.0, 0.5);
    testChildPanel2.setMarginPx(10,10,5,10);
    testChildPanel2.setFill(GLPanel::GLPANEL_FILL_GRAD_X);
    testChildPanel2.setFillColor(RGB(0.0,0.0,1.0), RGB(0.0,1.0,0.0));
    testChildPanel2.setBorderColor(RGB(1.0,0.0,0.0));
    testChildPanel2.setBorderPx(1);
    
    testText1.setText("Testing 123..");
    testText1.setFill(GLPanel::GLPANEL_FILL_NONE);
    testChildPanel2.addChild(&testText1);
    
    testPanel.addChild(&testChildPanel);
    testPanel.addChild(&testChildPanel2);
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
    testPanel.draw(CubicVR::mat4::identity());
}

void UITestContext::DrawEnd() {
    glFlush();
    
    CheckGLError();
}


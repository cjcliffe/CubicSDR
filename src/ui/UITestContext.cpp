#include "UITestContext.h"
#include "UITestCanvas.h"
#include "ColorTheme.h"

UITestContext::UITestContext(UITestCanvas *canvas, wxGLContext *sharedContext) :
PrimaryGLContext(canvas, sharedContext) {
    testPanel.setPosition(0.0, 0.0);
    testPanel.setSize(1.0, 1.0);
    testPanel.setMargin(0.02, 0.02, 0.1, 0.1);
    testChildPanel.setPosition(0.0, 0.0);
    testChildPanel.setMargin(0.05);
    testChildPanel.setSize(1.0, 0.3);
    testChildPanel2.setPosition(0.0, 0.3);
    testChildPanel2.setSize(1.0, 0.3);
    testChildPanel2.setMargin(0.05);
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
    testPanel.draw();
}

void UITestContext::DrawEnd() {
    glFlush();
    
    CheckGLError();
}


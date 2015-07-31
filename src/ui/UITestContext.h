#pragma once

#include "PrimaryGLContext.h"
#include "GLPanel.h"

class UITestCanvas;

class UITestContext: public PrimaryGLContext {
public:
    UITestContext(UITestCanvas *canvas, wxGLContext *sharedContext);
    
    void DrawBegin();
    void Draw();
    void DrawEnd();
    
private:
    GLPanel testPanel;
    GLTestPanel testChildPanel;
    GLPanel testChildPanel2;
    GLPanel testChildPanel3;
    GLTextPanel testText1;
};

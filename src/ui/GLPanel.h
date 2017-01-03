// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>
#include "GLExt.h"
#include "GLFont.h"
#include "ColorTheme.h"
#include "cubic_math.h"

class GLPanelEdges {
public:
    float left;
    float right;
    float top;
    float bottom;
    
    GLPanelEdges(): left(0), right(0), top(0), bottom(0) {
    }
    
    GLPanelEdges(float l, float r, float t, float b) {
        left = l;
        right = r;
        top = t;
        bottom = b;
    }
};

class GLPanel {
private:
    std::vector<float> glPoints;
    std::vector<float> glColors;
    
    void genArrays();
    void setViewport();
    
public:
    typedef enum GLPanelFillType { GLPANEL_FILL_NONE, GLPANEL_FILL_SOLID, GLPANEL_FILL_GRAD_X, GLPANEL_FILL_GRAD_Y, GLPANEL_FILL_GRAD_BAR_X, GLPANEL_FILL_GRAD_BAR_Y } GLPanelFillType;
    typedef enum GLPanelCoordinateSystem { GLPANEL_Y_DOWN_ZERO_ONE, GLPANEL_Y_UP_ZERO_ONE, GLPANEL_Y_UP, GLPANEL_Y_DOWN } GLPanelCoordinateSystem;
    float pos[2];
    float rot[3];
    float size[2];
    float view[2];
    GLPanelFillType fillType;
    GLPanelCoordinateSystem coord;
    float marginPx;
    GLPanelEdges borderPx;
    RGBA4f fill[2];
    RGBA4f borderColor;
    bool contentsVisible, visible;
    CubicVR::mat4 transform, transformInverse;
    CubicVR::mat4 localTransform;
    float min, mid, max;
    // screen dimensions
    CubicVR::vec2 vmin, vmax;
    // unit dimensions
    CubicVR::vec2 umin, umax, ucenter;
    // pixel dimensions
    CubicVR::vec2 pdim, pvec;
    GLuint srcBlend, dstBlend;
    
    std::vector<GLPanel *> children;
    
    GLPanel();
    
    void setPosition(float x, float y);


    void setSize(float w, float h);
    float getWidth();
    float getHeight();
    float getWidthPx();
    float getHeightPx();
    void setCoordinateSystem(GLPanelCoordinateSystem coord);
    
    bool hitTest(CubicVR::vec2 pos, CubicVR::vec2 &result);
    
    void setFill(GLPanelFillType fill_mode);
    void setFillColor(RGBA4f color1);
    void setFillColor(RGBA4f color1, RGBA4f color2);
    void setMarginPx(float marg);

    void setBorderColor(RGBA4f clr);
    void setBorderPx(float bord);
    void setBorderPx(float bordl, float bordr, float bordt, float bordb);
    
    void setBlend(GLuint src, GLuint dst);
    
    void addChild(GLPanel *childPanel);
    void removeChild(GLPanel *childPanel);
    
    void drawChildren();
    virtual void drawPanelContents();
    void calcTransform(CubicVR::mat4 transform);
    void draw();
};


class GLTextPanel : public GLPanel {
private:
    std::string textVal;
    GLFont::Align horizAlign;
    GLFont::Align vertAlign;
    bool useNativeFont;
public:
    GLTextPanel();
    
    void drawPanelContents();
    
    void setText(std::string text, GLFont::Align hAlign = GLFont::GLFONT_ALIGN_CENTER, GLFont::Align vAlign = GLFont::GLFONT_ALIGN_CENTER , bool useNativeFont = false);
    std::string getText();
};

class GLTestPanel : public GLPanel {
public:
    GLTestPanel() : GLPanel() {
        
    }
    
    void drawPanelContents();
};

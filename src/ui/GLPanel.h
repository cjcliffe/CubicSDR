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
    float size[2];
    float view[2];
    GLPanelFillType fillType;
    GLPanelCoordinateSystem coord;
    GLPanelEdges marginPx;
    GLPanelEdges borderPx;
    RGB fill[2];
    RGB borderColor;
    bool contentsVisible;
    CubicVR::mat4 transform;
    CubicVR::mat4 localTransform;
    float min, mid, max;
    // screen dimensions
    CubicVR::vec2 vmin, vmax;
    // unit dimensions
    CubicVR::vec2 umin, umax, ucenter;
    // pixel dimensions
    CubicVR::vec2 pdim, pvec;

    std::vector<GLPanel *> children;
    
    GLPanel();
    
    void setPosition(float x, float y);
    void setSize(float w, float h);
    float getWidthPx();
    float getHeightPx();
    void setCoordinateSystem(GLPanelCoordinateSystem coord);
    
    void setFill(GLPanelFillType fill_mode);
    void setFillColor(RGB color1);
    void setFillColor(RGB color1, RGB color2);
    void setMarginPx(float marg);
    void setMarginPx(float margl, float margr, float margt, float margb);

    void setBorderColor(RGB clr);
    void setBorderPx(float bord);
    void setBorderPx(float bordl, float bordr, float bordt, float bordb);
    
    void addChild(GLPanel *childPanel);
    
    void drawChildren();
    virtual void drawPanelContents();
    void draw(CubicVR::mat4 transform, GLPanel *parent=NULL);
};


class GLTextPanel : public GLPanel {
private:
    std::string textVal;
public:
    GLTextPanel();
    
    void drawPanelContents();
    void setText(std::string text);
    std::string getText();
};

class GLTestPanel : public GLPanel {
public:
    GLTestPanel() : GLPanel() {
        
    }
    
    void drawPanelContents();
};

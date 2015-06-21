#pragma once

#include <vector>
#include "GLExt.h"

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

    void genArrays() {
        if (!glPoints.size()) {
            glPoints.resize(2 * 4);
            glColors.resize(3 * 4);
        }
        
        float pts[2 * 4] = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f
        };
        
        float clr[3 * 4] = {
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0,
            1.0, 1.0, 1.0
        };
        
        glPoints.reserve(2 * 4);
        glColors.reserve(3 * 4);
        glPoints.assign(pts, pts + (2 * 4));
        glColors.assign(clr, clr + (3 * 4));
    }
    
public:
    typedef enum GLPanelFillType { GLPANEL_FILL_NONE, GLPANEL_FILL_SOLID } GLPanelFillType;
    typedef enum GLPanelCoordinateSystem { GLPANEL_Y_DOWN_ZERO_ONE, GLPANEL_Y_UP_ZERO_ONE, GLPANEL_Y_UP, GLPANEL_Y_DOWN } GLPanelCoordinateSystem;
    float pos[2];
    float size[2];
    float view[2];
    GLPanelFillType fill;
    GLPanelCoordinateSystem coord;
    GLPanelEdges margin;
    GLPanelEdges border;
    bool contentsVisible;
    
    std::vector<GLPanel *> children;
    
    GLPanel();
    
    void setViewport();
    void setPosition(float x, float y);
    void setSize(float w, float h);
    float getWidthPx();
    float getHeightPx();
    
    void setFill(GLPanelFillType fill_mode);
    void setMargin(float marg);
    void setMargin(float margl, float margr, float margt, float margb);
        
    void addChild(GLPanel *childPanel);
    
    void drawChildren();
    virtual void drawPanelContents();
    void draw(GLPanel *parent=NULL);
};


class GLTestPanel : public GLPanel {
public:
    GLTestPanel() : GLPanel() {
        
    }
    
    void drawPanelContents() {
        glColor3f(1.0,1.0,1.0);
        glBegin(GL_LINES);
        glVertex2f(0, 0.5);
        glVertex2f(1, 0.5);
        glVertex2f(0.5, 0);
        glVertex2f(0.5, 1);
        
        glVertex2f(0.5, 1);
        glVertex2f(0.48, 0.80);
        glVertex2f(0.5, 1);
        glVertex2f(0.52, 0.80);
        glEnd();
    }
};

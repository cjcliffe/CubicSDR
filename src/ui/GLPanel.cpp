
#include "GLPanel.h"



GLPanel::GLPanel() : fill(GLPANEL_FILL_SOLID), coord(GLPANEL_Y_DOWN_ZERO_ONE), contentsVisible(true) {
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    size[0] = 1.0f;
    size[1] = 1.0f;
    
    genArrays();
}

void GLPanel::setViewport() {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    
    view[0] = vp[2];
    view[1] = vp[3];
}

void GLPanel::setPosition(float x, float y) {
    pos[0] = x;
    pos[1] = y;
}

void GLPanel::setSize(float w, float h) {
    size[0] = w;
    size[1] = h;
}

float GLPanel::getWidthPx() {
    return size[0]*view[0];
}

float GLPanel::getHeightPx() {
    return size[1]*view[0];
}

void GLPanel::setFill(GLPanelFillType fill_mode) {
    fill = fill_mode;
}

void GLPanel::setMargin(float marg) {
    margin.left = margin.right = margin.top = margin.bottom = marg;
}

void GLPanel::setMargin(float margl, float margr, float margt, float margb) {
    margin.left = margl;
    margin.right = margr;
    margin.top = margt;
    margin.bottom = margb;
}


void GLPanel::addChild(GLPanel *childPanel) {
    children.push_back(childPanel);
}

void GLPanel::drawChildren() {
    if (children.size()) {
        std::vector<GLPanel *>::iterator panel_i;
        
        for (panel_i = children.begin(); panel_i != children.end(); panel_i++) {
            (*panel_i)->draw(this);
        }
    }
}

void GLPanel::drawPanelContents() {
    drawChildren();
}

void GLPanel::draw(GLPanel *parent) {
    if (!parent) {
        if (coord == GLPANEL_Y_DOWN_ZERO_ONE) {
            glPushMatrix();
            glTranslatef(-1.0f, 1.0f, 0.0f);
            glScalef(2.0f, -2.0f, 2.0f);
        }
        if (coord == GLPANEL_Y_UP_ZERO_ONE) {
            glPushMatrix();
            glTranslatef(-1.0f, -1.0f, 0.0f);
            glScalef(2.0f, 2.0f, 2.0f);
        }
        if (coord == GLPANEL_Y_DOWN) {
            glPushMatrix();
            glScalef(1.0f, -1.0f, 1.0f);
        }
        if (coord == GLPANEL_Y_UP) {
            glPushMatrix();
        }
    }
    glPushMatrix();
    glTranslatef(pos[0]+margin.left, pos[1]+margin.top, 0);
    glScalef(size[0]-(margin.left+margin.right), size[1]-(margin.top+margin.bottom), 0);
    
    if (fill != GLPANEL_FILL_NONE) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &glPoints[0]);
        glColorPointer(3, GL_FLOAT, 0, &glColors[0]);
        
        glDrawArrays(GL_QUADS, 0, glPoints.size() / 2);
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
    }
    
    if (contentsVisible) {
        glPushMatrix();
        drawPanelContents();
        glPopMatrix();
    }
    
    glPopMatrix();
    if (!parent) {
        glPopMatrix();
    }
}

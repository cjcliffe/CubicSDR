
#include "GLPanel.h"


GLPanel::GLPanel() : fillType(GLPANEL_FILL_SOLID), coord(GLPANEL_Y_DOWN_ZERO_ONE), contentsVisible(true) {
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    size[0] = 1.0f;
    size[1] = 1.0f;
    fill[0] = RGB(0.5,0.5,0.5);
    fill[1] = RGB(0.1,0.1,0.1);
    genArrays();
}

void GLPanel::genArrays() {
    if (fillType == GLPANEL_FILL_SOLID || fillType == GLPANEL_FILL_GRAD_X || fillType == GLPANEL_FILL_GRAD_Y) {
        glPoints.reserve(2 * 4);
        glPoints.resize(2 * 4);
        glColors.reserve(3 * 4);
        glColors.resize(3 * 4);
        
        float pts[2 * 4] = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f
        };
        
        RGB c[4];
        
        if (fillType == GLPANEL_FILL_SOLID) {
            c[0] = c[1] = c[2] = c[3] = fill[0];
        } else if (fillType == GLPANEL_FILL_GRAD_X) {
            c[0] = c[1] = fill[0];
            c[2] = c[3] = fill[1];
        } else if (fillType == GLPANEL_FILL_GRAD_Y) {
            c[0] = c[3] = fill[0];
            c[1] = c[2] = fill[1];
        }
        
        float clr[3 * 4] = {
            c[0].r, c[0].g, c[0].b,
            c[1].r, c[1].g, c[1].b,
            c[2].r, c[2].g, c[2].b,
            c[3].r, c[3].g, c[3].b
        };
        
        glPoints.assign(pts, pts + (2 * 4));
        glColors.assign(clr, clr + (3 * 4));
    } else {
        glPoints.reserve(2 * 8);
        glPoints.resize(2 * 8);
        glColors.reserve(3 * 8);
        glColors.resize(3 * 8);
        
        RGB c[8];
        
        if (fillType == GLPANEL_FILL_GRAD_BAR_X) {
            float pts[2 * 8] = {
                0.0f, 0.0f,
                0.0f, 1.0f,
                0.5f, 1.0f,
                0.5f, 0.0f,
                
                0.5f, 0.0f,
                0.5f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f
            };
            glPoints.assign(pts, pts + (2 * 8));

            c[0] = c[1] = fill[0];
            c[2] = c[3] = fill[1];

            c[4] = c[5] = fill[1];
            c[6] = c[7] = fill[0];

        } else if (fillType == GLPANEL_FILL_GRAD_BAR_Y) {
            float pts[2 * 8] = {
                0.0f, 0.0f,
                0.0f, 0.5f,
                1.0f, 0.5f,
                1.0f, 0.0f,
                
                0.0f, 0.5f,
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.5f
            };
            glPoints.assign(pts, pts + (2 * 8));

            c[0] = c[3] = fill[0];
            c[1] = c[2] = fill[1];

            c[4] = c[7] = fill[1];
            c[5] = c[6] = fill[0];
        }
        
        float clr[3 * 8] = {
            c[0].r, c[0].g, c[0].b,
            c[1].r, c[1].g, c[1].b,
            c[2].r, c[2].g, c[2].b,
            c[3].r, c[3].g, c[3].b,
            c[4].r, c[4].g, c[4].b,
            c[5].r, c[5].g, c[5].b,
            c[6].r, c[6].g, c[6].b,
            c[7].r, c[7].g, c[7].b
        };
        
        glColors.assign(clr, clr + (3 * 8));
    }
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
    fillType = fill_mode;
    genArrays();
}

void GLPanel::setFillColor(RGB color1) {
    fill[0] = color1;
    genArrays();
}

void GLPanel::setFillColor(RGB color1, RGB color2) {
    fill[0] = color1;
    fill[1] = color2;
    genArrays();
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
    
    if (fillType != GLPANEL_FILL_NONE) {
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






void GLTestPanel::drawPanelContents() {
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
    
    glVertex2f(1, 0.5);
    glVertex2f(0.90, 0.25);
    glVertex2f(1, 0.5);
    glVertex2f(0.90, 0.75);
    
    glEnd();
}

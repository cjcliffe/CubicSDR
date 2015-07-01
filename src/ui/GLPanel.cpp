
#include "GLPanel.h"
#include "cubic_math.h"

GLPanel::GLPanel() : fillType(GLPANEL_FILL_SOLID), contentsVisible(true), transform(CubicVR::mat4::identity()) {
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    size[0] = 1.0f;
    size[1] = 1.0f;
    fill[0] = RGB(0.5,0.5,0.5);
    fill[1] = RGB(0.1,0.1,0.1);
    borderColor = RGB(0.8, 0.8, 0.8);
    setCoordinateSystem(GLPANEL_Y_DOWN_ZERO_ONE);
}

void GLPanel::genArrays() {
    if (fillType == GLPANEL_FILL_SOLID || fillType == GLPANEL_FILL_GRAD_X || fillType == GLPANEL_FILL_GRAD_Y) {
        glPoints.reserve(2 * 4);
        glPoints.resize(2 * 4);
        glColors.reserve(3 * 4);
        glColors.resize(3 * 4);
        
        float pts[2 * 4] = {
            min, min,
            min, max,
            max, max,
            max, min
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
                min, min,
                min, max,
                mid, max,
                mid, min,
                
                mid, min,
                mid, max,
                max, max,
                max, min
            };
            glPoints.assign(pts, pts + (2 * 8));

            c[0] = c[1] = fill[0];
            c[2] = c[3] = fill[1];

            c[4] = c[5] = fill[1];
            c[6] = c[7] = fill[0];

        } else if (fillType == GLPANEL_FILL_GRAD_BAR_Y) {
            float pts[2 * 8] = {
                min, min,
                min, mid,
                max, mid,
                max, min,
                
                min, mid,
                min, max,
                max, max,
                max, mid
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


void GLPanel::setCoordinateSystem(GLPanelCoordinateSystem coord_in) {
    coord = coord_in;
    
    if (coord == GLPANEL_Y_DOWN || coord == GLPANEL_Y_UP) {
        min = -1;
        mid = 0;
        max = 1;
    } else {
        min = 0;
        mid = 0.5;
        max = 1;
    }
    
    genArrays();
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

void GLPanel::setMarginPx(float marg) {
    marginPx.left = marginPx.right = marginPx.top = marginPx.bottom = marg;
}

void GLPanel::setMarginPx(float margl, float margr, float margt, float margb) {
    marginPx.left = margl;
    marginPx.right = margr;
    marginPx.top = margt;
    marginPx.bottom = margb;
}

void GLPanel::setBorderColor(RGB clr) {
    borderColor = clr;
}

void GLPanel::setBorderPx(float bord) {
    borderPx.left = borderPx.right = borderPx.top = borderPx.bottom = bord;
}

void GLPanel::setBorderPx(float bordl, float bordr, float bordt, float bordb) {
    borderPx.left = bordl;
    borderPx.right = bordr;
    borderPx.top = bordt;
    borderPx.bottom = bordb;
}

void GLPanel::addChild(GLPanel *childPanel) {
    children.push_back(childPanel);
}

void GLPanel::drawChildren() {
    if (children.size()) {
        std::vector<GLPanel *>::iterator panel_i;
        
        for (panel_i = children.begin(); panel_i != children.end(); panel_i++) {
            (*panel_i)->draw(transform, this);
        }
    }
}

void GLPanel::drawPanelContents() {
    drawChildren();
}

void GLPanel::draw(CubicVR::mat4 transform_in, GLPanel *parent) {
    using namespace CubicVR;
    
    mat4 mCoord = mat4::identity();

    if (!parent) {
        if (coord == GLPANEL_Y_DOWN_ZERO_ONE) {
            mCoord *= mat4::translate(-1.0f, 1.0f, 0.0f) * mat4::scale(2.0f, -2.0f, 2.0f);
        }
        if (coord == GLPANEL_Y_UP_ZERO_ONE) {
            mCoord = mat4::translate(-1.0f, -1.0f, 0.0f) * mat4::scale(2.0f, 2.0f, 2.0f);
        }
        if (coord == GLPANEL_Y_DOWN) {
            mCoord = mat4::scale(2.0f, 2.0f, 2.0f);
        }
//        if (coord == GLPANEL_Y_UP) {
//        }
    } else if (parent->coord != coord) {
        if (parent->coord == GLPANEL_Y_DOWN_ZERO_ONE && coord == GLPANEL_Y_UP_ZERO_ONE) {
            mCoord *= mat4::translate(0.0f, 1.0f, 0.0f) * mat4::scale(1.0f, -1.0f, 1.0f);
        }
        if (parent->coord == GLPANEL_Y_UP_ZERO_ONE && coord == GLPANEL_Y_DOWN_ZERO_ONE) {
            mCoord *= mat4::translate(0.0f, -1.0f, 0.0f) * mat4::scale(1.0f, -1.0f, 1.0f);
        }
//        if (coord == GLPANEL_Y_UP_ZERO_ONE) {
//            mCoord = mat4::translate(-1.0f, -1.0f, 0.0f) * mat4::scale(2.0f, 2.0f, 2.0f);
//        }
//        if (coord == GLPANEL_Y_DOWN) {
//            mCoord = mat4::scale(2.0f, 2.0f, 2.0f);
//        }
        //        if (coord == GLPANEL_Y_UP) {
        //        }
    }
    
    // compute local transform
    localTransform = mCoord * (mat4::translate(pos[0], pos[1], 0) * mat4::scale(size[0], size[1], 0));
    // compute global transform
    transform = transform_in * localTransform;
    
    // init view[]
    setViewport();
    
    // get min/max transform
    vec4 vmin_t = mat4::vec4_multiply(vec4(min, min, 0, 1), transform);
    vec4 vmax_t = mat4::vec4_multiply(vec4(max, max, 0, 1), transform);
    
    // screen dimensions
    vmin = vec2((vmin_t.x > vmax_t.x)?vmax_t.x:vmin_t.x, (vmin_t.y > vmax_t.y)?vmax_t.y:vmin_t.y);
    vmax = vec2((vmin_t.x > vmax_t.x)?vmin_t.x:vmax_t.x, (vmin_t.y > vmax_t.y)?vmin_t.y:vmax_t.y);
    
    // unit dimensions
    umin = (vmin * 0.5) + vec2(1,1);
    umax = (vmax * 0.5) + vec2(1,1);
    
    ucenter = vec2((umin + umax) * 0.5);
    
    // pixel dimensions
    pdim = vec2((umax.x - umin.x) * view[0], (umax.y  - umin.y) * view[1]);
    pvec = vec2((vmax.x - vmin.x) / pdim.x, (vmax.y - vmin.y) / pdim.y);
    
    std::cout << umin << " :: " << ucenter << " :: " << pdim << " :: " << pvec << std::endl;
    
    if (marginPx.left || marginPx.right || marginPx.top || marginPx.bottom) {
        localTransform *= mat4::translate(marginPx.left * pvec.x / size[0], marginPx.top * pvec.y / size[1], 0) *
            mat4::scale(1.0 - (marginPx.left + marginPx.right) * pvec.x / size[0], 1.0 - (marginPx.top + marginPx.bottom) * pvec.y / size[1], 0);
        transform = transform_in * localTransform;
    }
    
    glLoadMatrixf(transform);
    
    if (fillType != GLPANEL_FILL_NONE) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &glPoints[0]);
        glColorPointer(3, GL_FLOAT, 0, &glColors[0]);
        
        glDrawArrays(GL_QUADS, 0, glPoints.size() / 2);
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        
        if (borderPx.left || borderPx.right || borderPx.top || borderPx.bottom) {
            glEnable(GL_LINE_SMOOTH);
            glColor3f(borderColor.r, borderColor.g, borderColor.b);
            
            if (borderPx.left) {
                glLineWidth(borderPx.left);
                glBegin(GL_LINES);
                glVertex2f(min, min);
                glVertex2f(min, max);
                glEnd();
            }
        
            if (borderPx.right) {
                glLineWidth(borderPx.right);
                glBegin(GL_LINES);
                glVertex2f(max, min);
                glVertex2f(max, max);
                glEnd();
            }

            if (borderPx.top) {
                glLineWidth(borderPx.top);
                glBegin(GL_LINES);
                glVertex2f(min, min);
                glVertex2f(max, min);
                glEnd();
            }

            if (borderPx.bottom) {
                glLineWidth(borderPx.bottom);
                glBegin(GL_LINES);
                glVertex2f(min, max);
                glVertex2f(max, max);
                glEnd();
            }

            glDisable(GL_LINE_SMOOTH);
        }
    }
    
    if (contentsVisible) {
        glPushMatrix();
        drawPanelContents();
        glPopMatrix();
    }
}


GLTextPanel::GLTextPanel() : GLPanel() {
    coord = GLPANEL_Y_UP_ZERO_ONE;
}

void GLTextPanel::drawPanelContents() {
    glColor4f(1, 1, 1, 1.0);
    GLFont::getFont(GLFont::GLFONT_SIZE48).drawString(textVal, mid,  mid,  48, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, (int)pdim.x*2, (int)pdim.y*2);
}

void GLTextPanel::setText(std::string text) {
    textVal = text;
}

std::string GLTextPanel::getText() {
    return textVal;
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

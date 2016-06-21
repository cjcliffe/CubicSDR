
#include "GLPanel.h"
#include "cubic_math.h"
#include <algorithm>

using namespace CubicVR;

GLPanel::GLPanel() : fillType(GLPANEL_FILL_SOLID), contentsVisible(true), visible(true), transform(mat4::identity()) {
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    rot[0] = 0.0f;
    rot[1] = 0.0f;
    rot[2] = 0.0f;
    size[0] = 1.0f;
    size[1] = 1.0f;
    fill[0] = RGBA4f(0.5f,0.5f,0.5f);
    fill[1] = RGBA4f(0.1f,0.1f,0.1f);
    borderColor = RGBA4f(0.8f, 0.8f, 0.8f);
    setCoordinateSystem(GLPANEL_Y_UP);
    setMarginPx(0);
    setBorderPx(0);
    srcBlend = GL_SRC_ALPHA;
    dstBlend = GL_ONE_MINUS_SRC_ALPHA;
}

void GLPanel::genArrays() {
    float min = -1.0, mid = 0, max = 1.0;
    
    if (fillType == GLPANEL_FILL_SOLID || fillType == GLPANEL_FILL_GRAD_X || fillType == GLPANEL_FILL_GRAD_Y) {
        glPoints.reserve(2 * 4);
        glPoints.resize(2 * 4);
        glColors.reserve(4 * 4);
        glColors.resize(4 * 4);
        
        float pts[2 * 4] = {
            min, min,
            min, max,
            max, max,
            max, min
        };
        
        RGBA4f c[4];
        
        if (fillType == GLPANEL_FILL_SOLID) {
            c[0] = c[1] = c[2] = c[3] = fill[0];
        } else if (fillType == GLPANEL_FILL_GRAD_X) {
            c[0] = c[1] = fill[0];
            c[2] = c[3] = fill[1];
        } else if (fillType == GLPANEL_FILL_GRAD_Y) {
            c[0] = c[3] = fill[0];
            c[1] = c[2] = fill[1];
        }
        
        float clr[4 * 4] = {
             c[0].r, c[0].g, c[0].b, c[0].a,
             c[1].r, c[1].g, c[1].b, c[1].a,
             c[2].r, c[2].g, c[2].b, c[2].a,
             c[3].r, c[3].g, c[3].b, c[3].a
         };
        
        glPoints.assign(pts, pts + (2 * 4));
        glColors.assign(clr, clr + (4 * 4));
    } else {
        glPoints.reserve(2 * 8);
        glPoints.resize(2 * 8);
        glColors.reserve(4 * 8);
        glColors.resize(4 * 8);
        
        RGBA4f c[8];
        
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
        
        float clr[4 * 8] = {
            c[0].r, c[0].g, c[0].b, c[0].a,
            c[1].r, c[1].g, c[1].b, c[1].a,
            c[2].r, c[2].g, c[2].b, c[2].a,
            c[3].r, c[3].g, c[3].b, c[3].a,
            c[4].r, c[4].g, c[4].b, c[4].a,
            c[5].r, c[5].g, c[5].b, c[5].a,
            c[6].r, c[6].g, c[6].b, c[6].a,
            c[7].r, c[7].g, c[7].b, c[7].a
        };
        
        glColors.assign(clr, clr + (4 * 8));
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

float GLPanel::getWidth() {
    return size[0];
}

float GLPanel::getHeight() {
    return size[1];
}

float GLPanel::getWidthPx() {
    return pdim.x;
}

float GLPanel::getHeightPx() {
    return pdim.y;
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

bool GLPanel::hitTest(CubicVR::vec2 pos, CubicVR::vec2 &result) {
    CubicVR::vec4 hitPos = CubicVR::mat4::vec4_multiply(CubicVR::vec4(pos.x, pos.y, 0.0, 1.0), transformInverse);
    
    if (hitPos.x >= -1.0 && hitPos.x <= 1.0 && hitPos.y >= -1.0 && hitPos.y <= 1.0) {
        result.x = hitPos.x;
        result.y = hitPos.y;
        return true;
    }
    
    return false;
}


void GLPanel::setFill(GLPanelFillType fill_mode) {
    fillType = fill_mode;
    genArrays();
}

void GLPanel::setFillColor(RGBA4f color1) {
    fill[0] = color1;
    genArrays();
}

void GLPanel::setFillColor(RGBA4f color1, RGBA4f color2) {
    fill[0] = color1;
    fill[1] = color2;
    genArrays();
}

void GLPanel::setMarginPx(float marg) {
    marginPx = marg;
}


void GLPanel::setBorderColor(RGBA4f clr) {
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

void GLPanel::setBlend(GLuint src, GLuint dst) {
    srcBlend = src;
    dstBlend = dst;
}

void GLPanel::addChild(GLPanel *childPanel) {
    std::vector<GLPanel *>::iterator i = std::find(children.begin(), children.end(), childPanel);

    if (i == children.end()) {
        children.push_back(childPanel);
    }
}

void GLPanel::removeChild(GLPanel *childPanel) {
    std::vector<GLPanel *>::iterator i = std::find(children.begin(), children.end(), childPanel);

    if (i != children.end()) {
        children.erase(i);
    }
}

void GLPanel::drawChildren() {
    if (children.size()) {
        std::vector<GLPanel *>::iterator panel_i;
        
        for (panel_i = children.begin(); panel_i != children.end(); panel_i++) {
            (*panel_i)->calcTransform(transform);
            (*panel_i)->draw();
        }
    }
}

void GLPanel::drawPanelContents() {
    drawChildren();
}

void GLPanel::calcTransform(mat4 transform_in) {
    // compute local transform
    localTransform = mat4::translate(pos[0], pos[1], 0) * mat4::scale(size[0], size[1], 1);
    
    if (rot[0] || rot[1] || rot[2]) {
        localTransform *= mat4::rotate(rot[0], rot[1], rot[2]);
    }
    
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
    pdim = vec2((vmax.x - vmin.x) / 2.0 * view[0], (vmax.y  - vmin.y) / 2.0 * view[1]);
    pvec = vec2(((vmax.x - vmin.x) / 2.0) / pdim.x, ((vmax.y - vmin.y) / 2.0) / pdim.y);
    
//    std::cout << umin << " :: " << ucenter << " :: " << pdim << " :: " << pvec << std::endl;
    
    if (marginPx) {
        transform *= mat4::scale(1.0 - marginPx * 2.0 * pvec.x / size[0], 1.0 - marginPx * 2.0 * pvec.y / size[1], 1);
    }
    
    transformInverse = CubicVR::mat4::inverse(transform);
}

void GLPanel::draw() {
    float min = -1.0, max = 1.0;

    glLoadMatrixf(transform);
    
    if (fillType != GLPANEL_FILL_NONE && visible) {
        glEnable(GL_BLEND);
        glBlendFunc(srcBlend, dstBlend);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &glPoints[0]);
        glColorPointer(4, GL_FLOAT, 0, &glColors[0]);
        
        glDrawArrays(GL_QUADS, 0, glPoints.size() / 2);
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        
        if (borderPx.left || borderPx.right || borderPx.top || borderPx.bottom) {
            glEnable(GL_LINE_SMOOTH);
            glColor4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            
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
        glDisable(GL_BLEND);
    }
    
    if (contentsVisible) {
        mat4 mCoord = mat4::identity();

        if (coord == GLPANEL_Y_DOWN_ZERO_ONE) {
            mCoord *= mat4::translate(-1.0f, 1.0f, 0.0f) * mat4::scale(2.0f, -2.0f, 2.0f);
        }
        if (coord == GLPANEL_Y_UP_ZERO_ONE) {
            mCoord = mat4::translate(-1.0f, -1.0f, 0.0f) * mat4::scale(2.0f, 2.0f, 2.0f);
        }
        if (coord == GLPANEL_Y_DOWN) {
            mCoord = mat4::scale(1.0f, -1.0f, 1.0f);
        }
        //        if (coord == GLPANEL_Y_UP) {
        //        }
        glLoadMatrixf(transform * mCoord);
        drawPanelContents();
    }
}


GLTextPanel::GLTextPanel() : GLPanel() {
    coord = GLPANEL_Y_UP;
    horizAlign = GLFont::GLFONT_ALIGN_CENTER;
    vertAlign = GLFont::GLFONT_ALIGN_CENTER;
}

void GLTextPanel::drawPanelContents() {
    glColor4f(1, 1, 1, 1.0);
    
    GLFont::GLFontSize sz;

    
    if (pdim.y <= 16) {
        sz = GLFont::GLFONT_SIZE12;

    } else if (pdim.y <= 18) {
        sz = GLFont::GLFONT_SIZE16;

    } else if(pdim.y <= 24) {
        sz = GLFont::GLFONT_SIZE18;

    } else if(pdim.y <= 32) {
        sz = GLFont::GLFONT_SIZE24;

    } else if(pdim.y <= 48) {
        sz = GLFont::GLFONT_SIZE32;

    } else {
        sz = GLFont::GLFONT_SIZE48;

    }
    

    GLFont::getFont(sz).drawString(textVal, mid,  mid, horizAlign, vertAlign, (int)pdim.x, (int)pdim.y);
}

void GLTextPanel::setText(std::string text, GLFont::Align hAlign, GLFont::Align vAlign) {
    textVal = text;
    horizAlign = hAlign;
    vertAlign = vAlign;
}

std::string GLTextPanel::getText() {
    return textVal;
}



void GLTestPanel::drawPanelContents() {
    glColor3f(1.0,1.0,1.0);
    glBegin(GL_LINES);
    glVertex2f(min, mid);
    glVertex2f(max, mid);
    glVertex2f(mid, min);
    glVertex2f(mid, max);
    
    glVertex2f(mid, max);
    glVertex2f(mid - 0.02, max - 0.2);
    glVertex2f(mid, 1);
    glVertex2f(mid + 0.02, max - 0.2);
    
    glVertex2f(max, mid);
    glVertex2f(max - 0.1, mid + max * 0.25);
    glVertex2f(max, mid);
    glVertex2f(max - 0.1, mid - max * 0.25);
    
    glEnd();
}

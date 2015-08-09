#include "ScopePanel.h"
#include "ColorTheme.h"

ScopePanel::ScopePanel() : GLPanel(), scopeMode(SCOPE_MODE_Y) {
    bgPanel.setFill(GLPanelFillType::GLPANEL_FILL_GRAD_BAR_Y);
    bgPanelStereo[0].setFill(GLPanelFillType::GLPANEL_FILL_GRAD_BAR_Y);
    bgPanelStereo[0].setPosition(0, 0.5);
    bgPanelStereo[0].setSize(1, 0.5);
    bgPanelStereo[1].setFill(GLPanelFillType::GLPANEL_FILL_GRAD_BAR_Y);
    bgPanelStereo[1].setPosition(0, -0.5);
    bgPanelStereo[1].setSize(1, 0.5);
}

void ScopePanel::setMode(ScopeMode scopeMode) {
    this->scopeMode = scopeMode;
}


void ScopePanel::setPoints(std::vector<float> &points) {
    this->points.assign(points.begin(),points.end());
}

void ScopePanel::drawPanelContents() {

    glLineWidth(1.0);
    
    if (scopeMode == SCOPE_MODE_Y) {
        bgPanel.setFillColor(ThemeMgr::mgr.currentTheme->scopeBackground, ThemeMgr::mgr.currentTheme->scopeBackground * 2.0);
        bgPanel.calcTransform(transform);
        bgPanel.draw();

        glLoadIdentity();
        glColor3f(ThemeMgr::mgr.currentTheme->scopeLine.r * 0.35, ThemeMgr::mgr.currentTheme->scopeLine.g * 0.35,
                  ThemeMgr::mgr.currentTheme->scopeLine.b * 0.35);
        glBegin (GL_LINES);
        glVertex2f(-1.0, 0.0);
        glVertex2f(1.0, 0.0);
        glEnd();
    } else if (scopeMode == SCOPE_MODE_2Y)  {
        bgPanelStereo[0].setFillColor(ThemeMgr::mgr.currentTheme->scopeBackground, ThemeMgr::mgr.currentTheme->scopeBackground * 2.0);
        bgPanelStereo[1].setFillColor(ThemeMgr::mgr.currentTheme->scopeBackground, ThemeMgr::mgr.currentTheme->scopeBackground * 2.0);

        bgPanelStereo[0].calcTransform(transform);
        bgPanelStereo[0].draw();
        bgPanelStereo[1].calcTransform(transform);
        bgPanelStereo[1].draw();

        glLoadIdentity();
        glColor3f(ThemeMgr::mgr.currentTheme->scopeLine.r, ThemeMgr::mgr.currentTheme->scopeLine.g, ThemeMgr::mgr.currentTheme->scopeLine.b);
        glBegin (GL_LINES);
        glVertex2f(-1.0, 0.0);
        glVertex2f(1.0, 0.0);
        glColor3f(ThemeMgr::mgr.currentTheme->scopeLine.r * 0.35, ThemeMgr::mgr.currentTheme->scopeLine.g * 0.35,
                  ThemeMgr::mgr.currentTheme->scopeLine.b * 0.35);
        glVertex2f(-1.0, 0.5);
        glVertex2f(1.0, 0.5);
        glVertex2f(-1.0, -0.5);
        glVertex2f(1.0, -0.5);
        glEnd();

    } else if (scopeMode == SCOPE_MODE_XY) {
        // ...
    }
    
    if (points.size()) {
        glEnable (GL_BLEND);
        glEnable (GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(ThemeMgr::mgr.currentTheme->scopeLine.r, ThemeMgr::mgr.currentTheme->scopeLine.g, ThemeMgr::mgr.currentTheme->scopeLine.b, 1.0);
        glEnableClientState (GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points[0]);
        glLineWidth(1.5);
        if (scopeMode == SCOPE_MODE_Y) {
            glLoadMatrixf(bgPanel.transform);
            glDrawArrays(GL_LINE_STRIP, 0, points.size() / 2);
        } else if (scopeMode == SCOPE_MODE_2Y)  {
            glLoadMatrixf(bgPanelStereo[0].transform);
            glDrawArrays(GL_LINE_STRIP, 0, points.size() / 4);

            glLoadMatrixf(bgPanelStereo[1].transform);
            glDrawArrays(GL_LINE_STRIP, points.size() / 4, points.size() / 4);
        } else if (scopeMode == SCOPE_MODE_XY) {
            // ...
        }
        glLineWidth(1.0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisable(GL_BLEND);
    }
}


// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ScopePanel.h"
#include "ColorTheme.h"

ScopePanel::ScopePanel() : GLPanel(), scopeMode(SCOPE_MODE_Y) {
    setFill(GLPanelFillType::GLPANEL_FILL_NONE);
    bgPanel.setFill(GLPanelFillType::GLPANEL_FILL_GRAD_BAR_Y);
    bgPanelStereo[0].setFill(GLPanelFillType::GLPANEL_FILL_GRAD_BAR_Y);
    bgPanelStereo[0].setPosition(0, 0.5);
    bgPanelStereo[0].setSize(1, 0.5);
    bgPanelStereo[1].setFill(GLPanelFillType::GLPANEL_FILL_GRAD_BAR_Y);
    bgPanelStereo[1].setPosition(0, -0.5);
    bgPanelStereo[1].setSize(1, 0.5);
}

void ScopePanel::setMode(ScopeMode scopeMode_in) {
    scopeMode = scopeMode_in;
}

ScopePanel::ScopeMode ScopePanel::getMode() {
    return this->scopeMode;
}

void ScopePanel::setPoints(std::vector<float> &points_in) {
    points.assign(points_in.begin(), points_in.end());
}

void ScopePanel::drawPanelContents() {
    
    if (scopeMode == SCOPE_MODE_Y) {
        bgPanel.setFillColor(ThemeMgr::mgr.currentTheme->scopeBackground, ThemeMgr::mgr.currentTheme->scopeBackground * 2.0);
        bgPanel.calcTransform(transform);
        bgPanel.draw();
        glLineWidth(1.0);
        glEnable(GL_LINE_SMOOTH);
        glLoadMatrixf(transform.to_ptr());
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

        glLineWidth(1.0);
        glLoadMatrixf(transform.to_ptr());
        glColor3f(ThemeMgr::mgr.currentTheme->scopeLine.r, ThemeMgr::mgr.currentTheme->scopeLine.g, ThemeMgr::mgr.currentTheme->scopeLine.b);
        glEnable(GL_LINE_SMOOTH);
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
        RGBA4f bg1(ThemeMgr::mgr.currentTheme->scopeBackground), bg2(ThemeMgr::mgr.currentTheme->scopeBackground * 2.0);
        bg1.a = 0.05f;
        bg2.a = 0.05f;
        bgPanel.setFillColor(bg1, bg2);
        bgPanel.calcTransform(transform);
        bgPanel.draw();
        glLineWidth(1.0);
        glEnable(GL_POINT_SMOOTH);
        glPointSize(1.0);
        glLoadMatrixf(transform.to_ptr());
        glColor3f(ThemeMgr::mgr.currentTheme->scopeLine.r * 0.15, ThemeMgr::mgr.currentTheme->scopeLine.g * 0.15,
                  ThemeMgr::mgr.currentTheme->scopeLine.b * 0.15);
    }
    
    if (!points.empty()) {
        glEnable (GL_BLEND);
        glEnable (GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        if (scopeMode == SCOPE_MODE_XY) {
            glBlendFunc(GL_ONE, GL_ONE);
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        glColor4f(ThemeMgr::mgr.currentTheme->scopeLine.r, ThemeMgr::mgr.currentTheme->scopeLine.g, ThemeMgr::mgr.currentTheme->scopeLine.b, 1.0);
        glEnableClientState (GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points[0]);
        glLineWidth(1.5);
        if (scopeMode == SCOPE_MODE_Y) {
            glLoadMatrixf(bgPanel.transform.to_ptr());
            glDrawArrays(GL_LINE_STRIP, 0, points.size() / 2);
        } else if (scopeMode == SCOPE_MODE_2Y)  {
            glLoadMatrixf(bgPanelStereo[0].transform.to_ptr());
            glDrawArrays(GL_LINE_STRIP, 0, points.size() / 4);

            glLoadMatrixf(bgPanelStereo[1].transform.to_ptr());
            glDrawArrays(GL_LINE_STRIP, points.size() / 4, points.size() / 4);
        } else if (scopeMode == SCOPE_MODE_XY) {
            glLoadMatrixf(bgPanel.transform.to_ptr());
            glDrawArrays(GL_POINTS, 0, points.size() / 2);
        }
        glLineWidth(1.0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisable(GL_BLEND);
    }
}


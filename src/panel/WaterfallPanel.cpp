// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "WaterfallPanel.h"

WaterfallPanel::WaterfallPanel() : GLPanel(), fft_size(0), waterfall_lines(0), activeTheme(nullptr) {
	setFillColor(RGBA4f(0,0,0));
    for (unsigned int & i : waterfall) {
        i = 0;
    }
}

void WaterfallPanel::setup(unsigned int fft_size_in, int num_waterfall_lines_in) {
    waterfall_lines = num_waterfall_lines_in;
    fft_size = fft_size_in;
    ring.setup(fft_size_in, num_waterfall_lines_in);
    texInitialized.store(false);
}

void WaterfallPanel::refreshTheme() {
    glEnable (GL_TEXTURE_2D);
    
    for (unsigned int i : waterfall) {
        glBindTexture(GL_TEXTURE_2D, i);
        
        glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
        glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, &(ThemeMgr::mgr.currentTheme->waterfallGradient.getRed())[0]);
        glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, &(ThemeMgr::mgr.currentTheme->waterfallGradient.getGreen())[0]);
        glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, &(ThemeMgr::mgr.currentTheme->waterfallGradient.getBlue())[0]);
    }
}

void WaterfallPanel::setPoints(std::vector<float> &points_in) {
    ring.setPoints(points_in);
}

void WaterfallPanel::step() {
    ring.step();
}

void WaterfallPanel::update() {
    unsigned int half_fft_size = fft_size / 2;
    
    if (!texInitialized.load()) {
        for (int i = 0; i < 2; i++) {
            if (waterfall[i]) {
                glDeleteTextures(1, &waterfall[i]);
                waterfall[i] = 0;
            }
        }

        glGenTextures(2, waterfall);
        
        unsigned char *waterfall_tex;
        
        //Creates 2x 2D textures into card memory.
        //of size half_fft_size * waterfall_lines, which can be BIG.
        //The limit of the size of Waterfall is the size of the maximum supported 2D texture 
        //by the graphic card. (half_fft_size * waterfall_lines, i.e DEFAULT_DEMOD_WATERFALL_LINES_NB * DEFAULT_FFT_SIZE/2)
        waterfall_tex = new unsigned char[half_fft_size * waterfall_lines];
        memset(waterfall_tex, 0, half_fft_size * waterfall_lines);
        
        for (unsigned int i : waterfall) {
            glBindTexture(GL_TEXTURE_2D, i);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, half_fft_size, waterfall_lines, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, (GLvoid *) waterfall_tex);
        }
        
        delete[] waterfall_tex;

        refreshTheme();

        texInitialized.store(true);
        ring.setReady(true);
    }
    
    WaterfallRing::UploadPlan plan = ring.planUpload();
    for (const auto &op : plan.ops) {
        glBindTexture(GL_TEXTURE_2D, waterfall[op.texture]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, op.y, half_fft_size, op.height,
                        GL_COLOR_INDEX, GL_UNSIGNED_BYTE, (GLvoid *)(ring.reversedLine(op.texture) + op.srcOffset));
    }
    ring.commitUpload(plan);

}

void WaterfallPanel::drawPanelContents() {
    if (!texInitialized.load()) {
        return;
    }

    unsigned int half_fft_size = fft_size / 2;
    
    glLoadMatrixf(transform.to_ptr());
    
    glEnable (GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    
    if (activeTheme != ThemeMgr::mgr.currentTheme) {
        refreshTheme();
        activeTheme = ThemeMgr::mgr.currentTheme;
    }
    glColor3f(1.0, 1.0, 1.0);
    
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    
    float viewWidth = (float) vp[2];
    
    // some bias to prevent seams at odd scales
    float half_pixel = 1.0 / viewWidth;
    float half_texel = 1.0 / (float) half_fft_size;
    float vtexel = 1.0 / (float) waterfall_lines;
    float vofs = (float) (ring.rowOffset()) * vtexel;
    
    glBindTexture(GL_TEXTURE_2D, waterfall[0]);
    glBegin (GL_QUADS);
    glTexCoord2f(0.0 + half_texel, 1.0 + vofs);
    glVertex3f(-1.0, -1.0, 0.0);
    glTexCoord2f(1.0 - half_texel, 1.0 + vofs);
    glVertex3f(0.0 + half_pixel, -1.0, 0.0);
    glTexCoord2f(1.0 - half_texel, 0.0 + vofs);
    glVertex3f(0.0 + half_pixel, 1.0, 0.0);
    glTexCoord2f(0.0 + half_texel, 0.0 + vofs);
    glVertex3f(-1.0, 1.0, 0.0);
    glEnd();
    
    vofs = (float) (ring.rowOffset()) * vtexel;
    glBindTexture(GL_TEXTURE_2D, waterfall[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0 + half_texel, 1.0 + vofs);
    glVertex3f(0.0 - half_pixel, -1.0, 0.0);
    glTexCoord2f(1.0 - half_texel, 1.0 + vofs);
    glVertex3f(1.0, -1.0, 0.0);
    glTexCoord2f(1.0 - half_texel, 0.0 + vofs);
    glVertex3f(1.0, 1.0, 0.0);
    glTexCoord2f(0.0 + half_texel, 0.0 + vofs);
    glVertex3f(0.0 - half_pixel, 1.0, 0.0);
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_TEXTURE_2D);
}

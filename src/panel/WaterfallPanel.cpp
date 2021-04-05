// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "WaterfallPanel.h"

WaterfallPanel::WaterfallPanel() : GLPanel(), fft_size(0), waterfall_lines(0), waterfall_slice(nullptr), activeTheme(nullptr) {
	setFillColor(RGBA4f(0,0,0));
    for (unsigned int & i : waterfall) {
        i = 0;
    }
}

void WaterfallPanel::setup(unsigned int fft_size_in, int num_waterfall_lines_in) {
    waterfall_lines = num_waterfall_lines_in;
    fft_size = fft_size_in;
    lines_buffered.store(0);
    
    if (points.size() != fft_size) {
        points.resize(fft_size);
    }
    
    texInitialized.store(false);
    bufferInitialized.store(false);
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
    size_t halfPts = points_in.size() / 2;
    if (halfPts == fft_size) {
       
        for (unsigned int i = 0; i < fft_size; i++) {
            points[i] = points_in[i * 2 + 1];
        }
    } else {
        points.assign(points_in.begin(), points_in.end());
    }
}

void WaterfallPanel::step() {
    unsigned int half_fft_size = fft_size / 2;

    if (!bufferInitialized.load()) {
        delete waterfall_slice;
        waterfall_slice = new unsigned char[half_fft_size];
        bufferInitialized.store(true);
    }
    
    if (!texInitialized.load()) {
        return;
    }
    
    if (!points.empty() && points.size() == fft_size) {
        for (int j = 0; j < 2; j++) {
            for (unsigned int i = 0, iMax = half_fft_size; i < iMax; i++) {
                float v = points[j * half_fft_size + i];
                
                float wv = v < 0 ? 0 : (v > 0.99 ? 0.99 : v);
                
                waterfall_slice[i] = (unsigned char) floor(wv * 255.0);
            }
            
            unsigned int newBufSize = (half_fft_size*lines_buffered.load()+half_fft_size);
            if (lineBuffer[j].size() < newBufSize) {
                lineBuffer[j].resize(newBufSize);
                rLineBuffer[j].resize(newBufSize);
            }
            memcpy(&(lineBuffer[j][half_fft_size*lines_buffered.load()]), waterfall_slice, sizeof(unsigned char) * half_fft_size);
        }
        lines_buffered++;
    }
}

void WaterfallPanel::update() {
    unsigned int half_fft_size = fft_size / 2;
    
    if (!bufferInitialized.load()) {
        return;
    }
    
    if (!texInitialized.load()) {
        for (int i = 0; i < 2; i++) {
            if (waterfall[i]) {
                glDeleteTextures(1, &waterfall[i]);
                waterfall[i] = 0;
            }
            
            waterfall_ofs[i] = waterfall_lines - 1;
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
    }
    
    for (int i = 0, iMax = lines_buffered.load(); i < iMax; i++) {
        for (int j = 0; j < 2; j++) {
            memcpy(&(rLineBuffer[j][i*half_fft_size]),
                   &(lineBuffer[j][((iMax-1)*half_fft_size)-(i*half_fft_size)]), sizeof(unsigned char) * half_fft_size);
        }
    }
    
    unsigned int run_ofs = 0;
    while (lines_buffered.load()) {
        int run_lines = lines_buffered.load();
        if (run_lines > waterfall_ofs[0]) {
            run_lines = waterfall_ofs[0];
        }
        for (int j = 0; j < 2; j++) {
            glBindTexture(GL_TEXTURE_2D, waterfall[j]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, waterfall_ofs[j]-run_lines, half_fft_size, run_lines,
                            GL_COLOR_INDEX, GL_UNSIGNED_BYTE, (GLvoid *) &(rLineBuffer[j][run_ofs]));
            
            waterfall_ofs[j]-=run_lines;
            
            if (waterfall_ofs[j] == 0) {
                waterfall_ofs[j] = waterfall_lines;
            }
        }
        run_ofs += run_lines*half_fft_size;
        lines_buffered.store(lines_buffered.load()-run_lines);
    }
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
    float vofs = (float) (waterfall_ofs[0]) * vtexel;
    
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
    
    vofs = (float) (waterfall_ofs[1]) * vtexel;
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

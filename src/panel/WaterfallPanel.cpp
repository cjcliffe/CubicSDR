#include "WaterfallPanel.h"

WaterfallPanel::WaterfallPanel() : GLPanel(), fft_size(0), waterfall_lines(0), waterfall_slice(NULL), activeTheme(NULL) {
	setFillColor(RGBA4f(0,0,0));
    for (int i = 0; i < 2; i++) {
        waterfall[i] = 0;
    }
}

void WaterfallPanel::setup(int fft_size_in, int num_waterfall_lines_in) {
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
    
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, waterfall[i]);
        
        glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
        glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, &(ThemeMgr::mgr.currentTheme->waterfallGradient.getRed())[0]);
        glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, &(ThemeMgr::mgr.currentTheme->waterfallGradient.getGreen())[0]);
        glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, &(ThemeMgr::mgr.currentTheme->waterfallGradient.getBlue())[0]);
    }
}

void WaterfallPanel::setPoints(std::vector<float> &points) {
    int halfPts = points.size()/2;
    if (halfPts == fft_size) {
        for (int i = 0; i < fft_size; i++) {
            this->points[i] = points[i*2+1];
        }
    } else {
        this->points.assign(points.begin(), points.end());
    }
}

void WaterfallPanel::step() {
    int half_fft_size = fft_size / 2;

    if (!bufferInitialized.load()) {
        delete waterfall_slice;
        waterfall_slice = new unsigned char[half_fft_size];
        bufferInitialized.store(true);
    }
    
    if (!texInitialized.load()) {
        return;
    }
    
    if (points.size() && points.size() == fft_size) {
        for (int j = 0; j < 2; j++) {
            for (int i = 0, iMax = half_fft_size; i < iMax; i++) {
                float v = points[j * half_fft_size + i];
                
                float wv = v < 0 ? 0 : (v > 0.99 ? 0.99 : v);
                
                waterfall_slice[i] = (unsigned char) floor(wv * 255.0);
            }
            
            int newBufSize = (half_fft_size*lines_buffered.load()+half_fft_size);
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
    int half_fft_size = fft_size / 2;
    
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
        
        waterfall_tex = new unsigned char[half_fft_size * waterfall_lines];
        memset(waterfall_tex, 0, half_fft_size * waterfall_lines);
        
        for (int i = 0; i < 2; i++) {
            glBindTexture(GL_TEXTURE_2D, waterfall[i]);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
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
    
    int run_ofs = 0;
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

    int half_fft_size = fft_size / 2;
    
    glLoadMatrixf(transform);
    
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

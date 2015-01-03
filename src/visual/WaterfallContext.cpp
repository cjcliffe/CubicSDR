#include "WaterfallContext.h"
#include "WaterfallCanvas.h"
#include "CubicSDR.h"

WaterfallContext::WaterfallContext(WaterfallCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext), waterfall(0), waterfall_tex(NULL) {
    grad.addColor(GradientColor(0, 0, 0));
    grad.addColor(GradientColor(0, 0, 1.0));
    grad.addColor(GradientColor(0, 1.0, 0));
    grad.addColor(GradientColor(1.0, 1.0, 0));
    grad.addColor(GradientColor(1.0, 0.2, 0.0));

    grad.generate(256);
}

void WaterfallContext::Setup(int fft_size_in, int num_waterfall_lines_in) {
    if (waterfall) {
        glDeleteTextures(1, &waterfall);
        waterfall = 0;
    }
    if (waterfall_tex) {
        delete waterfall_tex;
    }

    waterfall_lines = num_waterfall_lines_in;
    fft_size = fft_size_in;

    waterfall_tex = new unsigned char[fft_size * waterfall_lines];
    memset(waterfall_tex,0,fft_size * waterfall_lines);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glGenTextures(1, &waterfall);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, waterfall);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, &(grad.getRed())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, &(grad.getGreen())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, &(grad.getBlue())[0]);

}

void WaterfallContext::Draw(std::vector<float> &points) {

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    if (points.size()) {
        memmove(waterfall_tex + fft_size, waterfall_tex, (waterfall_lines - 1) * fft_size);

        for (int i = 0, iMax = fft_size; i < iMax; i++) {
            float v = points[i * 2 + 1];

            float wv = v;
            if (wv < 0.0)
                wv = 0.0;
            if (wv > 1.0)
                wv = 1.0;
            waterfall_tex[i] = (unsigned char) floor(wv * 255.0);
        }
    }

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, waterfall);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fft_size, waterfall_lines, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, (GLvoid *) waterfall_tex);

    glColor3f(1.0, 1.0, 1.0);

    glBindTexture(GL_TEXTURE_2D, waterfall);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(-1.0, -1.0, 0.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(1.0, -1.0, 0.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(1.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-1.0, 1.0, 0.0);
    glEnd();

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

}


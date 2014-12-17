#include "WaterfallContext.h"
#include "WaterfallCanvas.h"
#include "CubicSDR.h"

WaterfallContext::WaterfallContext(WaterfallCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
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

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    grad.addColor(GradientColor(0, 0, 0));
    grad.addColor(GradientColor(0, 0, 1.0));
    grad.addColor(GradientColor(0, 1.0, 0));
    grad.addColor(GradientColor(1.0, 1.0, 0));
    grad.addColor(GradientColor(1.0, 0.2, 0.0));

    grad.generate(256);

    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, &(grad.getRed())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, &(grad.getGreen())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, &(grad.getBlue())[0]);
}

void WaterfallContext::Draw(std::vector<float> &points) {

    if (points.size()) {
        memmove(waterfall_tex + FFT_SIZE, waterfall_tex, (NUM_WATERFALL_LINES - 1) * FFT_SIZE);

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FFT_SIZE, NUM_WATERFALL_LINES, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, (GLvoid *) waterfall_tex);

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

}


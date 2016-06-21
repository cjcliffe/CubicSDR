#include "SpectrumPanel.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include "ColorTheme.h"
#include "CubicSDRDefs.h"

SpectrumPanel::SpectrumPanel() {
    floorValue = 0;
    ceilValue = 1;
    showDb = false;
    fftSize = DEFAULT_FFT_SIZE;
    bandwidth = DEFAULT_DEMOD_BW;
    freq = 0;
    
    setFill(GLPANEL_FILL_GRAD_Y);
    setFillColor(ThemeMgr::mgr.currentTheme->fftBackground * 2.0, ThemeMgr::mgr.currentTheme->fftBackground);
    
    dbPanelCeil.setMarginPx(0);
    dbPanelCeil.setFill(GLPanel::GLPANEL_FILL_GRAD_X);
    dbPanelCeil.setFillColor(RGBA4f(0.2f,0.2f,0.2f,5.0f), RGBA4f(0.2f,0.2f,0.2f,0.0f));
    
    dbPanelFloor.setMarginPx(0);
    dbPanelFloor.setFill(GLPanel::GLPANEL_FILL_GRAD_X);
    dbPanelFloor.setFillColor(RGBA4f(0.2f,0.2f,0.2f,5.0f), RGBA4f(0.2f,0.2f,0.2f,0.0f));
}


float SpectrumPanel::getFloorValue() {
    return floorValue;
}

void SpectrumPanel::setFloorValue(float floorValue) {
    this->floorValue = floorValue;
}

float SpectrumPanel::getCeilValue() {
    return ceilValue;
}

void SpectrumPanel::setCeilValue(float ceilValue) {
    this->ceilValue = ceilValue;
}

void SpectrumPanel::setFreq(long long freq) {
    this->freq = freq;
}

long long SpectrumPanel::getFreq() {
    return freq;
}

void SpectrumPanel::setBandwidth(long long bandwidth) {
    this->bandwidth = bandwidth;
}

long long SpectrumPanel::getBandwidth() {
    return bandwidth;
}

void SpectrumPanel::setFFTSize(int fftSize_in) {
    this->fftSize = fftSize_in;
}

int SpectrumPanel::getFFTSize() {
    return fftSize;
}

void SpectrumPanel::setShowDb(bool showDb) {
    this->showDb = showDb;
    if (showDb) {
        addChild(&dbPanelCeil);
        addChild(&dbPanelFloor);
    } else {
        removeChild(&dbPanelCeil);
        removeChild(&dbPanelFloor);
    }
    
}

bool SpectrumPanel::getShowDb() {
    return showDb;
}


void SpectrumPanel::setPoints(std::vector<float> &points) {
    this->points.assign(points.begin(), points.end());
}

void SpectrumPanel::setPeakPoints(std::vector<float> &points) {
    this->peak_points.assign(points.begin(), points.end());
}


void SpectrumPanel::drawPanelContents() {
    glDisable(GL_TEXTURE_2D);
    
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );

    glLoadMatrixf(transform * (CubicVR::mat4::translate(-1.0f, -0.75f, 0.0f) * CubicVR::mat4::scale(2.0f, 1.5f, 1.0f)));

    if (points.size()) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        double range = ceilValue-floorValue;
        double ranges[3][4] = { { 90.0, 5000.0, 10.0, 100.0 }, { 20.0, 150.0, 10.0, 10.0 }, { -20.0, 30.0, 10.0, 1.0 } };
        
        for (int i = 0; i < 3; i++) {
            double p = 0;
            double rangeMin = ranges[i][0];
            double rangeMax = ranges[i][1];
            double rangeTrans = ranges[i][2];
            double rangeStep = ranges[i][3];
            
            if (range >= rangeMin && range <= rangeMax) {
                double a = 1.0;
                
                if (range <= rangeMin+rangeTrans) {
                    a *= (range-rangeMin)/rangeTrans;
                }
                if (range >= rangeMax-rangeTrans) {
                    a *= (rangeTrans-(range-(rangeMax-rangeTrans)))/rangeTrans;
                }
                
                glColor4f(0.12f, 0.12f, 0.12f, a);
                glBegin(GL_LINES);
                for (double l = floorValue; l<=ceilValue+rangeStep; l+=rangeStep) {
                    p += rangeStep/range;
                    glVertex2f(0,p);  glVertex2f(1,p);
                }
                glEnd();
            }
        }
        
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor3f(ThemeMgr::mgr.currentTheme->fftLine.r, ThemeMgr::mgr.currentTheme->fftLine.g, ThemeMgr::mgr.currentTheme->fftLine.b);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points[0]);
        glDrawArrays(GL_LINE_STRIP, 0, points.size() / 2);
        if (peak_points.size()) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0, 1.0, 0, 0.5);
            glVertexPointer(2, GL_FLOAT, 0, &peak_points[0]);
            glDrawArrays(GL_LINE_STRIP, 0, peak_points.size() / 2);
        }
        glDisableClientState(GL_VERTEX_ARRAY);
    }
  
    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);
    
    float viewHeight = (float) vp[3];
    float viewWidth = (float) vp[2];
    glLoadMatrixf(transform);

    
    long long leftFreq = (double) freq - ((double) bandwidth / 2.0);
    long long rightFreq = leftFreq + (double) bandwidth;

    long long hzStep = 1000000;
    
    long double mhzStep = (100000.0 / (long double) (rightFreq - leftFreq)) * 2.0;
    double mhzVisualStep = 0.1;

    std::stringstream label;
    label.precision(1);

    if (mhzStep * 0.5 * viewWidth < 40) {
        mhzStep = (250000.0 / (long double) (rightFreq - leftFreq)) * 2.0;
        mhzVisualStep = 0.25;

        if (mhzStep * 0.5 * viewWidth < 40) {
            mhzStep = (500000.0 / (long double) (rightFreq - leftFreq)) * 2.0;
            mhzVisualStep = 0.5;
        }

        if (mhzStep * 0.5 * viewWidth < 40) {
            mhzStep = (1000000.0 / (long double) (rightFreq - leftFreq)) * 2.0;
            mhzVisualStep = 1.0;
        }
        
        if (mhzStep * 0.5 * viewWidth < 40) {
            mhzStep = (2500000.0 / (long double) (rightFreq - leftFreq)) * 2.0;
            mhzVisualStep = 2.5;
        }
        
        if (mhzStep * 0.5 * viewWidth < 40) {
            mhzStep = (5000000.0 / (long double) (rightFreq - leftFreq)) * 2.0;
            mhzVisualStep = 5.0;
        }

        if (mhzStep * 0.5 * viewWidth < 40) {
            mhzStep = (10000000.0 / (long double) (rightFreq - leftFreq)) * 2.0;
            mhzVisualStep = 10.0;
        }

        if (mhzStep * 0.5 * viewWidth < 40) {
            mhzStep = (50000000.0 / (long double) (rightFreq - leftFreq)) * 2.0;
            mhzVisualStep = 50.0;
        }
    } else if (mhzStep * 0.5 * viewWidth > 350) {
        mhzStep = (10000.0 / (long double) (rightFreq - leftFreq)) * 2.0;
        mhzVisualStep = 0.01;
        label.precision(2);
    }
    
    long long firstMhz = (leftFreq / hzStep) * hzStep;
    long double mhzStart = ((long double) (firstMhz - leftFreq) / (long double) (rightFreq - leftFreq)) * 2.0;
    long double currentMhz = trunc(floor(firstMhz / (long double)1000000.0));
    
    
    double hPos = 1.0 - (16.0 / viewHeight);
    double lMhzPos = 1.0 - (5.0 / viewHeight);
    
    GLFont::GLFontSize fontEnumSize = GLFont::GLFONT_SIZE12;
    
    if (viewHeight > 135) {

        fontEnumSize = GLFont::GLFONT_SIZE16;
        hPos = 1.0 - (18.0 / viewHeight);
    }
    
    for (double m = -1.0 + mhzStart, mMax = 1.0 + ((mhzStart>0)?mhzStart:-mhzStart); m <= mMax; m += mhzStep) {
        if (m < -1.0) {
            currentMhz += mhzVisualStep;
            continue;
        }
        if (m > 1.0) {
            break;
        }
        label << std::fixed << currentMhz;
        
        double fractpart, intpart;
        
        fractpart = modf(currentMhz, &intpart);
        
        if (fractpart < 0.001) {
            glLineWidth(4.0);
            glColor3f(ThemeMgr::mgr.currentTheme->freqLine.r, ThemeMgr::mgr.currentTheme->freqLine.g, ThemeMgr::mgr.currentTheme->freqLine.b);
        } else {
            glLineWidth(1.0);
            glColor3f(ThemeMgr::mgr.currentTheme->freqLine.r * 0.65, ThemeMgr::mgr.currentTheme->freqLine.g * 0.65,
                      ThemeMgr::mgr.currentTheme->freqLine.b * 0.65);
        }
        
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_LINES);
        glVertex2f(m, lMhzPos);
        glVertex2f(m, 1);
        glEnd();
        
        glColor4f(ThemeMgr::mgr.currentTheme->text.r, ThemeMgr::mgr.currentTheme->text.g, ThemeMgr::mgr.currentTheme->text.b,1.0);
        
        GLFont::getFont(fontEnumSize).drawString(label.str(), m, hPos, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
        
        label.str(std::string());
        
        currentMhz += mhzVisualStep;
    }
    
    glLineWidth(1.0);

    if (showDb) {
        float dbPanelWidth = (1.0/viewWidth)*75.0;
        float dbPanelHeight = (1.0/viewHeight)*14.0;
        
        
        std::stringstream ssLabel("");
        if (getCeilValue() != getFloorValue() && fftSize) {
            ssLabel << std::fixed << std::setprecision(1) << (20.0 * log10(2.0*(getCeilValue())/(double)fftSize)) << "dB";
        }
        dbPanelCeil.setText(ssLabel.str(), GLFont::GLFONT_ALIGN_RIGHT);
        dbPanelCeil.setSize(dbPanelWidth, dbPanelHeight);
        dbPanelCeil.setPosition(-1.0 + dbPanelWidth, 1.0 - dbPanelHeight);
        
        ssLabel.str("");
        if (getCeilValue() != getFloorValue() && fftSize) {
            ssLabel << (20.0 * log10(2.0*(getFloorValue())/(double)fftSize)) << "dB";
        }

        dbPanelFloor.setText(ssLabel.str(), GLFont::GLFONT_ALIGN_RIGHT);
        dbPanelFloor.setSize(dbPanelWidth, dbPanelHeight);
        dbPanelFloor.setPosition(-1.0 + dbPanelWidth, - 1.0 + dbPanelHeight);
    }
}

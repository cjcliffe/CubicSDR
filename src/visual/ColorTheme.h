#pragma once

#include "Gradient.h"

#include <map>
#include <vector>
#include <string>

#define COLOR_THEME_DEFAULT 0
#define COLOR_THEME_BW 1
#define COLOR_THEME_SHARP 2
#define COLOR_THEME_RAD 3
#define COLOR_THEME_TOUCH 4
#define COLOR_THEME_HD 5
#define COLOR_THEME_RADAR 6
#define COLOR_THEME_MAX 7

class RGBA4f {
public:
    float r, g, b, a;
    RGBA4f(float r, float g, float b, float a = 1.0) :
            r(r), g(g), b(b), a(a) {
    }

    RGBA4f() :
            RGBA4f(0, 0, 0) {
    }

    ~RGBA4f() {
    }

    RGBA4f & operator=(const RGBA4f &other) {
        r = other.r;
        g = other.g;
        b = other.b;
        a = other.a;
        return *this;
    }
    
    RGBA4f operator*(float v) { return RGBA4f(r*v, g*v, b*v); }

};

class ColorTheme {
public:
    RGBA4f waterfallHighlight;
    RGBA4f waterfallNew;
    RGBA4f wfHighlight;
    RGBA4f waterfallHover;
    RGBA4f waterfallDestroy;
    RGBA4f fftLine;
    RGBA4f fftHighlight;
    RGBA4f scopeLine;
    RGBA4f tuningBarLight;
    RGBA4f tuningBarDark;
    RGBA4f tuningBarUp;
    RGBA4f tuningBarDown;
    RGBA4f meterLevel;
    RGBA4f meterValue;
    RGBA4f text;
    RGBA4f freqLine;
    RGBA4f button;
    RGBA4f buttonHighlight;

    RGBA4f scopeBackground;
    RGBA4f fftBackground;
    RGBA4f generalBackground;

    Gradient waterfallGradient;

    std::string name;
};

class ThemeMgr {
public:
    ThemeMgr();
    ~ThemeMgr();
    ColorTheme *currentTheme;
    std::map<int, ColorTheme *> themes;
    void setTheme(int themeId);
    int getTheme();
    int themeId;

    static ThemeMgr mgr;
};

class DefaultColorTheme: public ColorTheme {
public:
    DefaultColorTheme();
};

class BlackAndWhiteColorTheme: public ColorTheme {
public:
    BlackAndWhiteColorTheme();
};

class SharpColorTheme: public ColorTheme {
public:
    SharpColorTheme();
};

class RadColorTheme: public ColorTheme {
public:
    RadColorTheme();
};

class TouchColorTheme: public ColorTheme {
public:
    TouchColorTheme();
};

class HDColorTheme: public ColorTheme {
public:
    HDColorTheme();
};

class RadarColorTheme: public ColorTheme {
public:
    RadarColorTheme();
};


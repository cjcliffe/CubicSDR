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

class RGB3f {
public:
    float r, g, b;
    RGB3f(float r, float g, float b) :
            r(r), g(g), b(b) {
    }

    RGB3f() :
            RGB3f(0, 0, 0) {
    }

    ~RGB3f() {
    }

    RGB3f & operator=(const RGB3f &other) {
        r = other.r;
        g = other.g;
        b = other.b;
        return *this;
    }
    
    RGB3f operator*(float v) { return RGB3f(r*v, g*v, b*v); }

};

class ColorTheme {
public:
    RGB3f waterfallHighlight;
    RGB3f waterfallNew;
    RGB3f wfHighlight;
    RGB3f waterfallHover;
    RGB3f waterfallDestroy;
    RGB3f fftLine;
    RGB3f fftHighlight;
    RGB3f scopeLine;
    RGB3f tuningBarLight;
    RGB3f tuningBarDark;
    RGB3f tuningBarUp;
    RGB3f tuningBarDown;
    RGB3f meterLevel;
    RGB3f meterValue;
    RGB3f text;
    RGB3f freqLine;
    RGB3f button;
    RGB3f buttonHighlight;

    RGB3f scopeBackground;
    RGB3f fftBackground;
    RGB3f generalBackground;

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


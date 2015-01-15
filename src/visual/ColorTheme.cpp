#include "ColorTheme.h"
#include "CubicSDR.h"
#include "CubicSDRDefs.h"

ThemeMgr ThemeMgr::mgr;

void ThemeMgr::setTheme(int themeId) {
    currentTheme = themes[themeId];
    this->themeId = themeId;
}

int ThemeMgr::getTheme() {
    return themeId;
}

ThemeMgr::ThemeMgr() {
    themes[COLOR_THEME_DEFAULT] = new DefaultColorTheme;
    themes[COLOR_THEME_BW] = new BlackAndWhiteColorTheme;
    themes[COLOR_THEME_SHARP] = new SharpColorTheme;
    themes[COLOR_THEME_RAD] = new RadColorTheme;
    themes[COLOR_THEME_TOUCH] = new TouchColorTheme;
    themes[COLOR_THEME_HD] = new HDColorTheme;
    themes[COLOR_THEME_RADAR] = new RadarColorTheme;

    currentTheme = themes[COLOR_THEME_DEFAULT];
    themeId = COLOR_THEME_DEFAULT;
}

ThemeMgr::~ThemeMgr() {
    std::map<int, ColorTheme *>::iterator i;
    for (i = themes.begin(); i != themes.end(); i++) {
        delete i->second;
    }
}

DefaultColorTheme::DefaultColorTheme() {
    name = "Default";
    waterfallGradient.addColor(GradientColor(0, 0, 0));
    waterfallGradient.addColor(GradientColor(0, 0, 1.0));
    waterfallGradient.addColor(GradientColor(0, 1.0, 0));
    waterfallGradient.addColor(GradientColor(1.0, 1.0, 0));
    waterfallGradient.addColor(GradientColor(1.0, 0.2, 0.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBColor(1, 1, 1);
    waterfallNew = RGBColor(0, 1, 0);
    waterfallHover = RGBColor(1, 1, 0);
    waterfallDestroy = RGBColor(1, 0, 0);
    fftLine = RGBColor(0.9, 0.9, 0.9);
    fftHighlight = RGBColor(1, 1, 1);
    scopeLine = RGBColor(0.9, 0.9, 0.9);
    tuningBar = RGBColor(0.2, 0.2, 0.9);
    meterLevel = RGBColor(0.1, 0.75, 0.1);
    meterValue = RGBColor(0.75, 0.1, 0.1);
    text = RGBColor(1, 1, 1);
    freqLine = RGBColor(1, 1, 1);
    button = RGBColor(0.65, 0.65, 0.65);
    buttonHighlight = RGBColor(1, 1, 0);

    scopeBackground = RGBColor(0.1, 0.1, 0.1);
    fftBackground = RGBColor(0.1, 0.1, 0.1);
    generalBackground = RGBColor(0.1, 0.1, 0.1);
}

BlackAndWhiteColorTheme::BlackAndWhiteColorTheme() {
    name = "Black & White";
    waterfallGradient.addColor(GradientColor(0, 0, 0));
    waterfallGradient.addColor(GradientColor(0.75, 0.75, 0.75));
    waterfallGradient.addColor(GradientColor(1.0, 1.0, 1.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBColor(1, 1, 0.9);
    waterfallNew = RGBColor(0, 1, 0);
    waterfallHover = RGBColor(1, 1, 0);
    waterfallDestroy = RGBColor(1, 0, 0);
    fftLine = RGBColor(0.9, 0.9, 0.9);
    fftHighlight = RGBColor(1, 1, 0.9);
    scopeLine = RGBColor(0.9, 0.9, 0.9);
    tuningBar = RGBColor(0.4, 0.4, 0.4);
    meterLevel = RGBColor(0.5, 0.5, 0.5);
    meterValue = RGBColor(0.5, 0.5, 0.5);
    text = RGBColor(1, 1, 1);
    freqLine = RGBColor(1, 1, 1);
    button = RGBColor(0.65, 0.65, 0.65);
    buttonHighlight = RGBColor(1, 1, 1);

    scopeBackground = RGBColor(0.1, 0.1, 0.1);
    fftBackground = RGBColor(0.1, 0.1, 0.1);
    generalBackground = RGBColor(0.1, 0.1, 0.1);

}

SharpColorTheme::SharpColorTheme() {
    name = "Sharp";
    waterfallGradient.addColor(GradientColor(0, 0, 0));
    waterfallGradient.addColor(GradientColor(0.0, 0, 0.5));
    waterfallGradient.addColor(GradientColor(0.0, 0.0, 1.0));
    waterfallGradient.addColor(GradientColor(65.0 / 255.0, 161.0 / 255.0, 1.0));
    waterfallGradient.addColor(GradientColor(1.0, 1.0, 1.0));
    waterfallGradient.addColor(GradientColor(1.0, 1.0, 1.0));
    waterfallGradient.addColor(GradientColor(1.0, 1.0, 0.5));
    waterfallGradient.addColor(GradientColor(1.0, 1.0, 0.0));
    waterfallGradient.addColor(GradientColor(1.0, 0.5, 0.0));
    waterfallGradient.addColor(GradientColor(1.0, 0.25, 0.0));
    waterfallGradient.addColor(GradientColor(0.5, 0.1, 0.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBColor(0.9, 0.9, 1.0);
    waterfallNew = RGBColor(0, 1, 0);
    waterfallHover = RGBColor(1, 1, 0);
    waterfallDestroy = RGBColor(1, 0, 0);
    fftLine = RGBColor(0.9, 0.9, 1.0);
    fftHighlight = RGBColor(0.9, 0.9, 1.0);
    scopeLine = RGBColor(0.9, 0.9, 1.0);
    tuningBar = RGBColor(0.2, 0.2, 0.9);
    meterLevel = RGBColor(28.0/255.0,106.0/255.0,179.0/255.0);
    meterValue = RGBColor(190.0/255.0,190.0/255.0,60.0/255.0);
    text = RGBColor(0.9, 0.9, 1);
    freqLine = RGBColor(0.9, 0.9, 1);
    button  = RGBColor(217.0/255.0,218.0/255.0,228.0/255.0);
    buttonHighlight= RGBColor(208.0/255.0,249.0/255.0,255.0/255.0);

    scopeBackground = RGBColor(0.05, 0.05, 0.15);
    fftBackground = RGBColor(0.05, 0.05, 0.15);
    generalBackground = RGBColor(0.05, 0.05, 0.15);
}

RadColorTheme::RadColorTheme() {
    name = "Rad";
    waterfallGradient.addColor(GradientColor(0, 0, 0.5));
    waterfallGradient.addColor(GradientColor(25.0 / 255.0, 154.0 / 255.0, 0.0));
    waterfallGradient.addColor(GradientColor(201.0 / 255.0, 115.0 / 255.0, 0.0));
    waterfallGradient.addColor(GradientColor(1.0, 40.0 / 255.0, 40.0 / 255.0));
    waterfallGradient.addColor(GradientColor(1.0, 1.0, 1.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBColor(1, 1, 1);
    waterfallNew = RGBColor(0, 1, 0);
    waterfallHover = RGBColor(1, 1, 0);
    waterfallDestroy = RGBColor(1, 0, 0);
    fftLine = RGBColor(0.9, 0.9, 0.9);
    fftHighlight = RGBColor(1, 1, 1);
    scopeLine = RGBColor(0.9, 0.9, 0.9);
    tuningBar = RGBColor(0.2, 0.2, 0.9);
    meterLevel = RGBColor(0, 1, 0);
    meterValue = RGBColor(1, 0, 0);
    text = RGBColor(1, 1, 1);
    freqLine = RGBColor(1, 1, 1);
    button = RGBColor(0.65, 0.65, 0.65);
    buttonHighlight = RGBColor(1, 1, 0);
}

TouchColorTheme::TouchColorTheme() {
    name = "Touch";
    waterfallGradient.addColor(GradientColor(0, 0, 0));
    waterfallGradient.addColor(GradientColor(55.0 / 255.0, 40.0 / 255.0, 55.0 / 255.0));
    waterfallGradient.addColor(GradientColor(60.0 / 255.0, 60.0 / 255.0, 90.0 / 255.0));
    waterfallGradient.addColor(GradientColor(0.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0));
    waterfallGradient.addColor(GradientColor(10.0 / 255.0, 255.0 / 255.0, 85.0 / 255.0));
    waterfallGradient.addColor(GradientColor(255.0 / 255.0, 255.0 / 255.0, 75.0 / 255.0));
    waterfallGradient.addColor(GradientColor(255.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0));
    waterfallGradient.addColor(GradientColor(255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBColor(1, 1, 1);
    waterfallNew = RGBColor(0, 1, 0);
    waterfallHover = RGBColor(1, 1, 0);
    waterfallDestroy = RGBColor(1, 0, 0);
    fftLine = RGBColor(0.9, 0.9, 0.9);
    fftHighlight = RGBColor(1, 1, 1);
    scopeLine = RGBColor(0.9, 0.9, 0.9);
    tuningBar = RGBColor(0.2, 0.2, 0.9);
    meterLevel = RGBColor(0, 1, 0);
    meterValue = RGBColor(1, 0, 0);
    text = RGBColor(1, 1, 1);
    freqLine = RGBColor(1, 1, 1);
    button = RGBColor(0.65, 0.65, 0.65);
    buttonHighlight = RGBColor(1, 1, 0);
}

HDColorTheme::HDColorTheme() {
    name = "HD";
    waterfallGradient.addColor(GradientColor(5.0 / 255.0, 5.0 / 255.0, 60.0 / 255.0));
    waterfallGradient.addColor(GradientColor(5.0 / 255.0, 20.0 / 255.0, 120.0 / 255.0));
    waterfallGradient.addColor(GradientColor(50.0 / 255.0, 100.0 / 255.0, 200.0 / 255.0));
    waterfallGradient.addColor(GradientColor(75.0 / 255.0, 190.0 / 255.0, 100.0 / 255.0));
    waterfallGradient.addColor(GradientColor(240.0 / 255.0, 55.0 / 255.0, 5.0 / 255.0));
    waterfallGradient.addColor(GradientColor(255.0 / 255.0, 55.0 / 255.0, 100.0 / 255.0));
    waterfallGradient.addColor(GradientColor(255.0 / 255.0, 235.0 / 255.0, 100.0 / 255.0));
    waterfallGradient.addColor(GradientColor(250.0 / 255.0, 250.0 / 255.0, 250.0 / 255.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBColor(1, 1, 1);
    waterfallNew = RGBColor(0, 1, 0);
    waterfallHover = RGBColor(1, 1, 0);
    waterfallDestroy = RGBColor(1, 0, 0);
    fftLine = RGBColor(0.9, 0.9, 0.9);
    fftHighlight = RGBColor(1, 1, 1);
    scopeLine = RGBColor(0.9, 0.9, 0.9);
    tuningBar = RGBColor(0.2, 0.2, 0.9);
    meterLevel = RGBColor(0, 1, 0);
    meterValue = RGBColor(1, 0, 0);
    text = RGBColor(1, 1, 1);
    freqLine = RGBColor(1, 1, 1);
    button = RGBColor(0.65, 0.65, 0.65);
    buttonHighlight = RGBColor(1, 1, 0);
}

RadarColorTheme::RadarColorTheme() {
    name = "Rad";
    waterfallGradient.addColor(GradientColor(5.0 / 255.0, 45.0 / 255.0, 10.0 / 255.0));
    waterfallGradient.addColor(GradientColor(30.0 / 255.0, 150.0 / 255.0, 40.0 / 255.0));
    waterfallGradient.addColor(GradientColor(40.0 / 255.0, 240.0 / 255.0, 60.0 / 255.0));
    waterfallGradient.addColor(GradientColor(250.0 / 255.0, 250.0 / 255.0, 250.0 / 255.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBColor(1, 1, 1);
    waterfallNew = RGBColor(0, 1, 0);
    waterfallHover = RGBColor(1, 1, 0);
    waterfallDestroy = RGBColor(1, 0, 0);
    fftLine = RGBColor(0.8, 1.0, 0.8);
    fftHighlight = RGBColor(1, 1, 1);
    scopeLine = RGBColor(0.8, 1.0, 0.8);
    tuningBar = RGBColor(0.2, 0.9, 0.2);
    meterLevel = RGBColor(0, 0.5, 0);
    meterValue = RGBColor(0, 0.5, 0);
    text = RGBColor(0.8, 1.0, 0.8);
    freqLine = RGBColor(1, 1, 1);
    button = RGBColor(0.65, 0.75, 0.65);
    buttonHighlight = RGBColor(0.65, 1.0, 0.65);

    scopeBackground = RGBColor(0.05, 0.1, 0.05);
    fftBackground = RGBColor(0.05, 0.1, 0.05);
    generalBackground = RGBColor(0.05, 0.1, 0.05);
}


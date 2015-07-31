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
    waterfallHighlight = RGB(1, 1, 1);
    waterfallNew = RGB(0, 1, 0);
    waterfallHover = RGB(1, 1, 0);
    waterfallDestroy = RGB(1, 0, 0);
    fftLine = RGB(0.9, 0.9, 0.9);
    fftHighlight = RGB(1, 1, 1);
    scopeLine = RGB(0.9, 0.9, 0.9);
    tuningBarLight = RGB(0.2, 0.2, 0.9);
    tuningBarDark = RGB(0.0, 0.0, 0.35);
    tuningBarUp = RGB(1.0, 139.0/255.0, 96.0/255.0);
    tuningBarDown = RGB(148.0/255.0, 148.0/255.0, 1.0);
    meterLevel = RGB(0.1, 0.75, 0.1);
    meterValue = RGB(0.75, 0.1, 0.1);
    text = RGB(1, 1, 1);
    freqLine = RGB(1, 1, 1);
    button = RGB(0.65, 0.65, 0.65);
    buttonHighlight = RGB(1, 1, 0);

    scopeBackground = RGB(0.1, 0.1, 0.1);
    fftBackground = RGB(0.1, 0.1, 0.1);
    generalBackground = RGB(0.1, 0.1, 0.1);
}


RadarColorTheme::RadarColorTheme() {
    name = "Rad";
    waterfallGradient.addColor(GradientColor(5.0 / 255.0, 45.0 / 255.0, 10.0 / 255.0));
    waterfallGradient.addColor(GradientColor(30.0 / 255.0, 150.0 / 255.0, 40.0 / 255.0));
    waterfallGradient.addColor(GradientColor(40.0 / 255.0, 240.0 / 255.0, 60.0 / 255.0));
    waterfallGradient.addColor(GradientColor(250.0 / 255.0, 250.0 / 255.0, 250.0 / 255.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGB(1, 1, 1);
    waterfallNew = RGB(0, 1, 0);
    waterfallHover = RGB(1, 1, 0);
    waterfallDestroy = RGB(1, 0, 0);
    fftLine = RGB(0.8, 1.0, 0.8);
    fftHighlight = RGB(1, 1, 1);
    scopeLine = RGB(0.8, 1.0, 0.8);
    tuningBarLight = RGB(0.0, 0.45, 0.0);
    tuningBarDark = RGB(0.0, 0.1, 0.0);
    tuningBarUp = RGB(1.0, 139.0/255.0, 96.0/255.0);
    tuningBarDown = RGB(148.0/255.0, 0.0, 0.0);
    meterLevel = RGB(0, 0.5, 0);
    meterValue = RGB(0, 0.5, 0);
    text = RGB(0.8, 1.0, 0.8);
    freqLine = RGB(1, 1, 1);
    button = RGB(0.65, 0.75, 0.65);
    buttonHighlight = RGB(0.65, 1.0, 0.65);

    scopeBackground = RGB(0.05, 0.1, 0.05);
    fftBackground = RGB(0.05, 0.1, 0.05);
    generalBackground = RGB(0.05, 0.1, 0.05);
}

BlackAndWhiteColorTheme::BlackAndWhiteColorTheme() {
    name = "Black & White";
    waterfallGradient.addColor(GradientColor(0, 0, 0));
    waterfallGradient.addColor(GradientColor(0.75, 0.75, 0.75));
    waterfallGradient.addColor(GradientColor(1.0, 1.0, 1.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGB(1, 1, 0.9);
    waterfallNew = RGB(0, 1, 0);
    waterfallHover = RGB(1, 1, 0);
    waterfallDestroy = RGB(1, 0, 0);
    fftLine = RGB(0.9, 0.9, 0.9);
    fftHighlight = RGB(1, 1, 0.9);
    scopeLine = RGB(0.9, 0.9, 0.9);
    tuningBarLight = RGB(0.4, 0.4, 0.4);
    tuningBarDark = RGB(0.1, 0.1, 0.1);
    tuningBarUp = RGB(0.8, 0.8, 0.8);
    tuningBarDown = RGB(0.4, 0.4, 0.4);
    meterLevel = RGB(0.5, 0.5, 0.5);
    meterValue = RGB(0.5, 0.5, 0.5);
    text = RGB(1, 1, 1);
    freqLine = RGB(1, 1, 1);
    button = RGB(0.65, 0.65, 0.65);
    buttonHighlight = RGB(1, 1, 1);

    scopeBackground = RGB(0.1, 0.1, 0.1);
    fftBackground = RGB(0.1, 0.1, 0.1);
    generalBackground = RGB(0.1, 0.1, 0.1);

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
    waterfallHighlight = RGB(0.9, 0.9, 1.0);
    waterfallNew = RGB(0, 1, 0);
    waterfallHover = RGB(1, 1, 0);
    waterfallDestroy = RGB(1, 0, 0);
    fftLine = RGB(0.9, 0.9, 1.0);
    fftHighlight = RGB(0.9, 0.9, 1.0);
    scopeLine = RGB(0.85, 0.85, 1.0);
    tuningBarLight = RGB(28.0 / 255.0, 106.0 / 255.0, 179.0 / 255.0);
    tuningBarDark = RGB(14.0 / 255.0, 53.0 / 255.0, 89.5 / 255.0);
    tuningBarUp = RGB(0.7, 0.7, 0.7);
    tuningBarDown = RGB(1.0, 0.0, 0.0);
    meterLevel = RGB(28.0 / 255.0, 106.0 / 255.0, 179.0 / 255.0);
    meterValue = RGB(190.0 / 255.0, 190.0 / 255.0, 60.0 / 255.0);
    text = RGB(0.9, 0.9, 1);
    freqLine = RGB(0.85, 0.85, 1.0);
    button = RGB(217.0 / 255.0, 218.0 / 255.0, 228.0 / 255.0);
    buttonHighlight = RGB(208.0 / 255.0, 249.0 / 255.0, 255.0 / 255.0);

    scopeBackground = RGB(0.05, 0.05, 0.15);
    fftBackground = RGB(0.05, 0.05, 0.15);
    generalBackground = RGB(0.05, 0.05, 0.15);
}

RadColorTheme::RadColorTheme() {
    name = "Rad";
    waterfallGradient.addColor(GradientColor(0, 0, 0.5));
    waterfallGradient.addColor(GradientColor(25.0 / 255.0, 154.0 / 255.0, 0.0));
    waterfallGradient.addColor(GradientColor(201.0 / 255.0, 115.0 / 255.0, 0.0));
    waterfallGradient.addColor(GradientColor(1.0, 40.0 / 255.0, 40.0 / 255.0));
    waterfallGradient.addColor(GradientColor(1.0, 1.0, 1.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGB(1, 1, 1);
    waterfallNew = RGB(0, 1, 0);
    waterfallHover = RGB(1, 1, 0);
    waterfallDestroy = RGB(1, 0, 0);
    fftLine = RGB(1.0, 0.9, 0.9);
    fftHighlight = RGB(1, 1, 1);
    scopeLine = RGB(1.0, 0.9, 0.9);
    tuningBarLight = RGB(0.0, 0.45, 0.0);
    tuningBarDark = RGB(0.0, 0.1, 0.0);
    tuningBarUp = RGB(1.0, 0.0, 0.0);
    tuningBarDown = RGB(0.0, 0.5, 1.0);
    meterLevel = RGB(0, 0.5, 0);
    meterValue = RGB(0.5, 0, 0);
    text = RGB(1, 1, 1);
    freqLine = RGB(1, 1, 1);
    button = RGB(0.65, 0.65, 0.65);
    buttonHighlight = RGB(0.76, 0.65, 0);

    scopeBackground = RGB(13.0 / 255.0, 47.0 / 255.0, 9.0 / 255.0);
    fftBackground = RGB(0, 0, 50.0 / 255.0);
    generalBackground = RGB(13.0 / 255.0, 47.0 / 255.0, 9.0 / 255.0);
}

TouchColorTheme::TouchColorTheme() {
    name = "Touch";
    waterfallGradient.addColor(GradientColor(0, 0, 0));
    waterfallGradient.addColor(GradientColor(55.0 / 255.0, 40.0 / 255.0, 55.0 / 255.0));
    waterfallGradient.addColor(GradientColor(61.0 / 255.0, 57.0 / 255.0, 88.0 / 255.0));
    waterfallGradient.addColor(GradientColor(0.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0));
    waterfallGradient.addColor(GradientColor(10.0 / 255.0, 255.0 / 255.0, 85.0 / 255.0));
    waterfallGradient.addColor(GradientColor(255.0 / 255.0, 255.0 / 255.0, 75.0 / 255.0));
    waterfallGradient.addColor(GradientColor(255.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0));
    waterfallGradient.addColor(GradientColor(255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGB(1, 1, 1);
    waterfallNew = RGB(0, 1, 0);
    waterfallHover = RGB(1, 1, 0);
    waterfallDestroy = RGB(1, 0, 0);
    fftLine = RGB(234.0 / 255.0, 232.0 / 255.0, 247.0 / 255.0);
    fftHighlight = RGB(1.0, 1.0, 1.0);
    scopeLine = RGB(234.0 / 255.0, 232.0 / 255.0, 247.0 / 255.0);
    tuningBarLight = RGB(0.2, 0.2, 0.7);
    tuningBarDark = RGB(0.1, 0.1, 0.45);
    tuningBarUp = RGB(0.5, 139.0/255.0, 96.0/255.0);
    tuningBarDown = RGB(0.6, 108.0/255.0, 1.0);
    meterLevel = RGB(61.0 / 255.0, 57.0 / 255.0, 88.0 / 255.0);
    meterValue = RGB(61.0 / 255.0, 57.0 / 255.0, 88.0 / 255.0);
    text = RGB(1, 1, 1);
    freqLine = RGB(1, 1, 1);
    button = RGB(1.0, 1.0, 1.0);
    buttonHighlight = RGB(208.0 / 255.0, 202.0 / 255.0, 247.0 / 255.0);

    scopeBackground = RGB(39.0 / 255.0, 36.0 / 255.0, 56.0 / 255.0);
    fftBackground = RGB(39.0 / 255.0, 36.0 / 255.0, 56.0 / 255.0);
    generalBackground = RGB(61.0 / 255.0, 57.0 / 255.0, 88.0 / 255.0);

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
    waterfallHighlight = RGB(1, 1, 1);
    waterfallNew = RGB(0, 1, 0);
    waterfallHover = RGB(1, 1, 0);
    waterfallDestroy = RGB(1, 0, 0);
    fftLine = RGB(0.9, 0.9, 0.9);
    fftHighlight = RGB(1, 1, 1);
    scopeLine = RGB(0.9, 0.9, 0.9);
    tuningBarLight = RGB(0.4, 0.4, 1.0);
    tuningBarDark = RGB(0.1, 0.1, 0.45);
    tuningBarUp = RGB(1.0, 139.0/255.0, 96.0/255.0);
    tuningBarDown = RGB(148.0/255.0, 148.0/255.0, 1.0);
    meterLevel = RGB(0, 0.5, 0);
    meterValue = RGB(0.0, 0.0, 1.0);
    text = RGB(1, 1, 1);
    freqLine = RGB(1, 1, 1);
    button = RGB(0, 0.7, 0.7);
    buttonHighlight = RGB(1, 1, 1);

    scopeBackground = RGB(0.0, 0.0, 48.0 / 255.0);
    fftBackground = RGB(0.0, 0.0, 48.0 / 255.0);
    generalBackground = RGB(0.0, 0.0, 0.0);

}


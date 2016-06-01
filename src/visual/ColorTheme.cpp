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
    waterfallGradient.addColor(GradientColor(1.0, 0.2f, 0.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBA4f(1, 1, 1);
    waterfallNew = RGBA4f(0, 1, 0);
    waterfallHover = RGBA4f(1, 1, 0);
    waterfallDestroy = RGBA4f(1, 0, 0);
    fftLine = RGBA4f(0.9f, 0.9f, 0.9f);
    fftHighlight = RGBA4f(1, 1, 1);
    scopeLine = RGBA4f(0.9f, 0.9f, 0.9f);
    tuningBarLight = RGBA4f(0.2f, 0.2f, 0.9f);
    tuningBarDark = RGBA4f(0.0f, 0.0f, 0.35f);
    tuningBarUp = RGBA4f(1.0f, 139.0f/255.0f, 96.0f/255.0f);
    tuningBarDown = RGBA4f(148.0f/255.0f, 148.0f/255.0f, 1.0f);
    meterLevel = RGBA4f(0.1f, 0.75f, 0.1f);
    meterValue = RGBA4f(0.75f, 0.1f, 0.1f);
    text = RGBA4f(1, 1, 1);
    freqLine = RGBA4f(1, 1, 1);
    button = RGBA4f(0.65f, 0.65f, 0.65f);
    buttonHighlight = RGBA4f(1, 1, 0);

    scopeBackground = RGBA4f(0.1f, 0.1f, 0.1f);
    fftBackground = RGBA4f(0.1f, 0.1f, 0.1f);
    generalBackground = RGBA4f(0.1f, 0.1f, 0.1f);
}


RadarColorTheme::RadarColorTheme() {
    name = "Rad";
    waterfallGradient.addColor(GradientColor(5.0f / 255.0f, 45.0f / 255.0f, 10.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(30.0f / 255.0f, 150.0f / 255.0f, 40.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(40.0f / 255.0f, 240.0f / 255.0f, 60.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(250.0f / 255.0f, 250.0f / 255.0f, 250.0f / 255.0f));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBA4f(1, 1, 1);
    waterfallNew = RGBA4f(0, 1, 0);
    waterfallHover = RGBA4f(1, 1, 0);
    waterfallDestroy = RGBA4f(1, 0, 0);
    fftLine = RGBA4f(0.8f, 1.0f, 0.8f);
    fftHighlight = RGBA4f(1, 1, 1);
    scopeLine = RGBA4f(0.8f, 1.0f, 0.8f);
    tuningBarLight = RGBA4f(0.0, 0.45f, 0.0);
    tuningBarDark = RGBA4f(0.0, 0.1f, 0.0);
    tuningBarUp = RGBA4f(1.0f, 139.0f/255.0f, 96.0f/255.0f);
    tuningBarDown = RGBA4f(148.0f/255.0f, 0.0, 0.0);
    meterLevel = RGBA4f(0, 0.5f, 0);
    meterValue = RGBA4f(0, 0.5f, 0);
    text = RGBA4f(0.8f, 1.0, 0.8f);
    freqLine = RGBA4f(1, 1, 1);
    button = RGBA4f(0.65f, 0.75f, 0.65f);
    buttonHighlight = RGBA4f(0.65f, 1.0f, 0.65f);

    scopeBackground = RGBA4f(0.05f, 0.1f, 0.05f);
    fftBackground = RGBA4f(0.05f, 0.1f, 0.05f);
    generalBackground = RGBA4f(0.05f, 0.1f, 0.05f);
}

BlackAndWhiteColorTheme::BlackAndWhiteColorTheme() {
    name = "Black & White";
    waterfallGradient.addColor(GradientColor(0, 0, 0));
    waterfallGradient.addColor(GradientColor(0.75f, 0.75f, 0.75f));
    waterfallGradient.addColor(GradientColor(1.0f, 1.0f, 1.0f));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBA4f(1, 1, 0.9f);
    waterfallNew = RGBA4f(0, 1, 0);
    waterfallHover = RGBA4f(1, 1, 0);
    waterfallDestroy = RGBA4f(1, 0, 0);
    fftLine = RGBA4f(0.9f, 0.9f, 0.9f);
    fftHighlight = RGBA4f(1, 1, 0.9f);
    scopeLine = RGBA4f(0.9f, 0.9f, 0.9f);
    tuningBarLight = RGBA4f(0.4f, 0.4f, 0.4f);
    tuningBarDark = RGBA4f(0.1f, 0.1f, 0.1f);
    tuningBarUp = RGBA4f(0.8f, 0.8f, 0.8f);
    tuningBarDown = RGBA4f(0.4f, 0.4f, 0.4f);
    meterLevel = RGBA4f(0.5f, 0.5f, 0.5f);
    meterValue = RGBA4f(0.5f, 0.5f, 0.5f);
    text = RGBA4f(1, 1, 1);
    freqLine = RGBA4f(1, 1, 1);
    button = RGBA4f(0.65f, 0.65f, 0.65f);
    buttonHighlight = RGBA4f(1, 1, 1);

    scopeBackground = RGBA4f(0.1f, 0.1f, 0.1f);
    fftBackground = RGBA4f(0.1f, 0.1f, 0.1f);
    generalBackground = RGBA4f(0.1f, 0.1f, 0.1f);

}

SharpColorTheme::SharpColorTheme() {
    name = "Sharp";
    waterfallGradient.addColor(GradientColor(0, 0, 0));
    waterfallGradient.addColor(GradientColor(0.0, 0, 0.5f));
    waterfallGradient.addColor(GradientColor(0.0, 0.0, 1.0f));
    waterfallGradient.addColor(GradientColor(65.0f / 255.0f, 161.0f / 255.0f, 1.0f));
    waterfallGradient.addColor(GradientColor(1.0f, 1.0f, 1.0f));
    waterfallGradient.addColor(GradientColor(1.0f, 1.0f, 1.0f));
    waterfallGradient.addColor(GradientColor(1.0f, 1.0f, 0.5f));
    waterfallGradient.addColor(GradientColor(1.0f, 1.0f, 0.0f));
    waterfallGradient.addColor(GradientColor(1.0f, 0.5f, 0.0f));
    waterfallGradient.addColor(GradientColor(1.0f, 0.25f, 0.0f));
    waterfallGradient.addColor(GradientColor(0.5f, 0.1f, 0.0f));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBA4f(0.9f, 0.9f, 1.0f);
    waterfallNew = RGBA4f(0, 1, 0);
    waterfallHover = RGBA4f(1, 1, 0);
    waterfallDestroy = RGBA4f(1, 0, 0);
    fftLine = RGBA4f(0.9f, 0.9f, 1.0);
    fftHighlight = RGBA4f(0.9f, 0.9f, 1.0);
    scopeLine = RGBA4f(0.85f, 0.85f, 1.0);
    tuningBarLight = RGBA4f(28.0f / 255.0f, 106.0f / 255.0f, 179.0f / 255.0f);
    tuningBarDark = RGBA4f(14.0f / 255.0f, 53.0f / 255.0f, 89.5f / 255.0f);
    tuningBarUp = RGBA4f(0.7f, 0.7f, 0.7f);
    tuningBarDown = RGBA4f(1.0f, 0.0, 0.0);
    meterLevel = RGBA4f(28.0f / 255.0f, 106.0f / 255.0f, 179.0f / 255.0f);
    meterValue = RGBA4f(190.0f / 255.0f, 190.0f / 255.0f, 60.0f / 255.0f);
    text = RGBA4f(0.9f, 0.9f, 1);
    freqLine = RGBA4f(0.85f, 0.85f, 1.0f);
    button = RGBA4f(217.0f / 255.0f, 218.0f / 255.0f, 228.0f / 255.0f);
    buttonHighlight = RGBA4f(208.0f / 255.0f, 249.0f / 255.0f, 255.0f / 255.0f);

    scopeBackground = RGBA4f(0.05f, 0.05f, 0.15f);
    fftBackground = RGBA4f(0.05f, 0.05f, 0.15f);
    generalBackground = RGBA4f(0.05f, 0.05f, 0.15f);
}

RadColorTheme::RadColorTheme() {
    name = "Rad";
    waterfallGradient.addColor(GradientColor(0, 0, 0.5f));
    waterfallGradient.addColor(GradientColor(25.0f / 255.0f, 154.0f / 255.0f, 0.0));
    waterfallGradient.addColor(GradientColor(201.0f / 255.0f, 115.0f / 255.0f, 0.0));
    waterfallGradient.addColor(GradientColor(1.0, 40.0f / 255.0f, 40.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(1.0, 1.0, 1.0));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBA4f(1, 1, 1);
    waterfallNew = RGBA4f(0, 1, 0);
    waterfallHover = RGBA4f(1, 1, 0);
    waterfallDestroy = RGBA4f(1, 0, 0);
    fftLine = RGBA4f(1.0, 0.9f, 0.9f);
    fftHighlight = RGBA4f(1, 1, 1);
    scopeLine = RGBA4f(1.0, 0.9f, 0.9f);
    tuningBarLight = RGBA4f(0.0, 0.45f, 0.0);
    tuningBarDark = RGBA4f(0.0, 0.1f, 0.0);
    tuningBarUp = RGBA4f(1.0, 0.0, 0.0);
    tuningBarDown = RGBA4f(0.0, 0.5f, 1.0);
    meterLevel = RGBA4f(0, 0.5f, 0);
    meterValue = RGBA4f(0.5f, 0, 0);
    text = RGBA4f(1, 1, 1);
    freqLine = RGBA4f(1, 1, 1);
    button = RGBA4f(0.65f, 0.65f, 0.65f);
    buttonHighlight = RGBA4f(0.76f, 0.65f, 0);

    scopeBackground = RGBA4f(13.0f / 255.0f, 47.0f / 255.0f, 9.0f / 255.0f);
    fftBackground = RGBA4f(0, 0, 50.0f / 255.0f);
    generalBackground = RGBA4f(13.0f / 255.0f, 47.0f / 255.0f, 9.0f / 255.0f);
}

TouchColorTheme::TouchColorTheme() {
    name = "Touch";
    waterfallGradient.addColor(GradientColor(0, 0, 0));
    waterfallGradient.addColor(GradientColor(55.0f / 255.0f, 40.0f / 255.0f, 55.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(61.0f / 255.0f, 57.0f / 255.0f, 88.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(10.0f / 255.0f, 255.0f / 255.0f, 85.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(255.0f / 255.0f, 255.0f / 255.0f, 75.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBA4f(1, 1, 1);
    waterfallNew = RGBA4f(0, 1, 0);
    waterfallHover = RGBA4f(1, 1, 0);
    waterfallDestroy = RGBA4f(1, 0, 0);
    fftLine = RGBA4f(234.0f / 255.0f, 232.0f / 255.0f, 247.0f / 255.0f);
    fftHighlight = RGBA4f(1.0f, 1.0f, 1.0f);
    scopeLine = RGBA4f(234.0f / 255.0f, 232.0f / 255.0f, 247.0f / 255.0f);
    tuningBarLight = RGBA4f(0.2f, 0.2f, 0.7f);
    tuningBarDark = RGBA4f(0.1f, 0.1f, 0.45f);
    tuningBarUp = RGBA4f(0.5f, 139.0f/255.0f, 96.0f/255.0f);
    tuningBarDown = RGBA4f(0.6f, 108.0f/255.0f, 1.0f);
    meterLevel = RGBA4f(61.0f / 255.0f, 57.0f / 255.0f, 88.0f / 255.0f);
    meterValue = RGBA4f(61.0f / 255.0f, 57.0f / 255.0f, 88.0f / 255.0f);
    text = RGBA4f(1, 1, 1);
    freqLine = RGBA4f(1, 1, 1);
    button = RGBA4f(1.0f, 1.0f, 1.0f);
    buttonHighlight = RGBA4f(208.0f / 255.0f, 202.0f / 255.0f, 247.0f / 255.0f);

    scopeBackground = RGBA4f(39.0f / 255.0f, 36.0f / 255.0f, 56.0f / 255.0f);
    fftBackground = RGBA4f(39.0f / 255.0f, 36.0f / 255.0f, 56.0f / 255.0f);
    generalBackground = RGBA4f(61.0f / 255.0f, 57.0f / 255.0f, 88.0f / 255.0f);

}

HDColorTheme::HDColorTheme() {
    name = "HD";
    waterfallGradient.addColor(GradientColor(5.0f / 255.0f, 5.0f / 255.0f, 60.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(5.0f / 255.0f, 20.0f / 255.0f, 120.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(50.0f / 255.0f, 100.0f / 255.0f, 200.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(75.0f / 255.0f, 190.0f / 255.0f, 100.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(240.0f / 255.0f, 55.0f / 255.0f, 5.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(255.0f / 255.0f, 55.0f / 255.0f, 100.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(255.0f / 255.0f, 235.0f / 255.0f, 100.0f / 255.0f));
    waterfallGradient.addColor(GradientColor(250.0f / 255.0f, 250.0f / 255.0f, 250.0f / 255.0f));
    waterfallGradient.generate(256);
    waterfallHighlight = RGBA4f(1, 1, 1);
    waterfallNew = RGBA4f(0, 1, 0);
    waterfallHover = RGBA4f(1, 1, 0);
    waterfallDestroy = RGBA4f(1, 0, 0);
    fftLine = RGBA4f(0.9f, 0.9f, 0.9f);
    fftHighlight = RGBA4f(1, 1, 1);
    scopeLine = RGBA4f(0.9f, 0.9f, 0.9f);
    tuningBarLight = RGBA4f(0.4f, 0.4f, 1.0);
    tuningBarDark = RGBA4f(0.1f, 0.1f, 0.45f);
    tuningBarUp = RGBA4f(1.0, 139.0f/255.0f, 96.0f/255.0f);
    tuningBarDown = RGBA4f(148.0f/255.0f, 148.0f/255.0f, 1.0f);
    meterLevel = RGBA4f(0, 0.5f, 0);
    meterValue = RGBA4f(0.0, 0.0, 1.0);
    text = RGBA4f(1, 1, 1);
    freqLine = RGBA4f(1, 1, 1);
    button = RGBA4f(0, 0.7f, 0.7f);
    buttonHighlight = RGBA4f(1, 1, 1);

    scopeBackground = RGBA4f(0.0, 0.0, 48.0f / 255.0f);
    fftBackground = RGBA4f(0.0, 0.0, 48.0f / 255.0f);
    generalBackground = RGBA4f(0.0, 0.0, 0.0);

}


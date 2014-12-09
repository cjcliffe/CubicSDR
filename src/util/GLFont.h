#pragma once

#include <map>
#include <string>
#include <sstream>
#include "lodepng.h"
#include "wx/glcanvas.h"


class GLFontChar {
public:
    GLFontChar();
    ~GLFontChar();

    void setId(int idval);
    int getId();

    void setXOffset(int xofs);
    int getXOffset();

    void setYOffset(int yofs);
    int getYOffset();

    void setX(int xpos);
    int getX();

    void setY(int ypos);
    int getY();

    void setWidth(int w);
    int getWidth();

    void setHeight(int h);
    int getHeight();

    void setXAdvance(int xadv);
    int getXAdvance();

    float getAspect();

    void setIndex(unsigned int idx);
    int getIndex();

private:
    int id;
    int x, y, width, height;
    int xoffset, yoffset;
    int xadvance;
    float aspect;
    int index;
};

class GLFont {
public:
    GLFont();
    ~GLFont();
    void loadFont(std::string fontFile);

    float getStringWidth(std::string str, float size, float viewAspect);
    void drawString(std::string str, float xpos, float ypos, float height, float viewAspect);

private:
    std::string nextParam(std::istringstream &str);
    std::string getParamKey(std::string param_str);
    std::string getParamValue(std::string param_str);

    int numCharacters;
    int lineHeight;
    int base;
    int imageWidth, imageHeight;

    std::map<int, GLFontChar *> characters;

    std::vector<float> gl_vertices;
    std::vector<float> gl_uv;

    std::string fontName;
    std::string imageFile;
    std::string fontFileSource;
    GLuint texId;
};

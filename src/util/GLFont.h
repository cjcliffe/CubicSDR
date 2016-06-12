#pragma once

#include <map>
#include <string>
#include <sstream>
#include <mutex>
#include <atomic>
#include "lodepng.h"
#include "wx/glcanvas.h"
#include "wx/filename.h"
#include "wx/stdpaths.h"

class GLFontStringCache {
public:
    GLFontStringCache();
    int drawlen;
    int vpx, vpy;
    int pxHeight;
    float msgWidth;
    std::atomic_int gc;
    std::vector<float> gl_vertices;
    std::vector<float> gl_uv;
};

class GLFontChar {
public:
    GLFontChar();
    ~GLFontChar();

    void setId(int idval);
    
    // Returns the code point of the 16bit character, supposely Unicode.    
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
    // this is the code point of the 16bit character, supposely Unicode.
    int id; 
    int x, y, width, height;
    int xoffset, yoffset;
    int xadvance;
    float aspect;
    int index;
};

class GLFont {
public:
    enum Align {
        GLFONT_ALIGN_LEFT, GLFONT_ALIGN_RIGHT, GLFONT_ALIGN_CENTER, GLFONT_ALIGN_TOP, GLFONT_ALIGN_BOTTOM
    };
    enum GLFontSize {
        GLFONT_SIZE12, GLFONT_SIZE16, GLFONT_SIZE18, GLFONT_SIZE24, GLFONT_SIZE32, GLFONT_SIZE48, GLFONT_MAX
    };

    GLFont();
    ~GLFont();

    void loadFont(const std::wstring& fontFile);

    static GLFont &getFont(GLFontSize esize);
   
    //Public drawing font, 16 bit char version.
    void drawString(const std::wstring& str, float xpos, float ypos, int pxHeight, Align hAlign = GLFONT_ALIGN_LEFT, Align vAlign = GLFONT_ALIGN_TOP, int vpx=0, int vpy=0, bool cacheable = false);

    //Public drawing font, 8 bit char version.
    void drawString(const std::string& str, float xpos, float ypos, int pxHeight, Align hAlign = GLFONT_ALIGN_LEFT, Align vAlign = GLFONT_ALIGN_TOP, int vpx = 0, int vpy = 0, bool cacheable = false);
   
private:
   
    std::wstring nextParam(std::wistringstream &str);
    std::wstring getParamKey(std::wstring param_str);
    std::wstring getParamValue(std::wstring param_str);

   
    static GLFont fonts[GLFONT_MAX];
    

    GLFontStringCache *cacheString(const std::wstring& str, int pxHeight, int vpx, int vpy);
    void drawCacheString(GLFontStringCache *fc, float xpos, float ypos, Align hAlign, Align vAlign);

    void doCacheGC();

   
    bool isLoaded();

    float getStringWidth(const std::wstring& str, float size, float viewAspect);

    std::map<std::wstring, GLFontStringCache * > stringCache;

    int lineHeight;
    int base;
    int imageWidth, imageHeight;
    bool loaded;

    std::map<int, GLFontChar *> characters;

    std::vector<float> gl_vertices;
    std::vector<float> gl_uv;

    std::wstring fontName;
    std::wstring imageFile;
    std::wstring fontFileSource;
    GLuint texId;
    int gcCounter;
    std::mutex cache_busy;
};

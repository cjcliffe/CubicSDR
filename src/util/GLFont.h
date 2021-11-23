// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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

#include "SpinMutex.h"

class GLFontStringCache {
public:
    GLFontStringCache();
    int drawlen;
    int vpx, vpy;
    int pxHeight = 0;
    float msgWidth = 0.0f;
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
    int getId() const;

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
    int getHeight() const;

    void setXAdvance(int xadv);
    int getXAdvance();

    float getAspect() const;

    void setIndex(unsigned int idx);
    int getIndex() const;

private:
    // this is the code point of the 16bit character, supposly Unicode.
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
        GLFONT_SIZE12,
        GLFONT_SIZE16,
        GLFONT_SIZE18,
        GLFONT_SIZE24,
        GLFONT_SIZE27, //new
        GLFONT_SIZE32,
        GLFONT_SIZE36, //new
        GLFONT_SIZE48,
        GLFONT_SIZE64, //new
        GLFONT_SIZE72, //new
        GLFONT_SIZE96, //new
        GLFONT_SIZE_MAX
    };

    enum GLFontScale {
        GLFONT_SCALE_NORMAL,
        GLFONT_SCALE_MEDIUM, // x1.5
        GLFONT_SCALE_LARGE,  // x2
        GLFONT_SCALE_MAX
    };

    GLFont(GLFontSize size, std::wstring fontFileName);
    ~GLFont();

   
    //Called to change the scale of the rendered fonts
    static void setScale(GLFontScale scale);

    static GLFontScale getScale();

    //Mean current scale factor: 1.0 in normal, 1.5 medium, 2.0 for large
    static double getScaleFactor();
    
    //Return a valid font px height given the font size and scale factor
    static int getScaledPx(int basicFontSize, double scaleFactor);

   
private:

    std::wstring nextParam(std::wistringstream &str);
    std::wstring getParamKey(const std::wstring& param_str);
    std::wstring getParamValue(const std::wstring& param_str);

    //Repository of all loaded fonts
    static GLFont fonts[GLFontSize::GLFONT_SIZE_MAX];

    static std::atomic<GLFontScale> currentScale;

    //load a given font file, (lazy loading) 
    void loadFontOnce();

    //private drawing font, 16 bit char version, called by Drawer object
    void drawString(const std::wstring& str, int pxHeight, float xpos, float ypos, Align hAlign = GLFONT_ALIGN_LEFT, Align vAlign = GLFONT_ALIGN_TOP, int vpx = 0, int vpy = 0, bool cacheable = false);

    //private drawing font, 8 bit char version, called by Drawer object
    void drawString(const std::string& str, int pxHeight, float xpos, float ypos, Align hAlign = GLFONT_ALIGN_LEFT, Align vAlign = GLFONT_ALIGN_TOP, int vpx = 0, int vpy = 0, bool cacheable = false);

    GLFontStringCache *cacheString(const std::wstring& str, int pxHeight, int vpx, int vpy);
    void drawCacheString(GLFontStringCache *fc, float xpos, float ypos, Align hAlign, Align vAlign) const;

    void doCacheGC();
    void clearCache();

    //force GC of all available fonts
    static void clearAllCaches();

    float getStringWidth(const std::wstring& str, float size, float viewAspect);

    //the string cache is per-front (internal font)
    std::map<std::wstring, GLFontStringCache * > stringCache;

    int lineHeight;
    int base;
    int imageWidth, imageHeight, pixHeight;
    bool loaded;
    GLFontSize fontSizeClass;

    std::map<int, GLFontChar *> characters;

    std::vector<float> gl_vertices;
    std::vector<float> gl_uv;

    //The font name as written in the def file.
    std::wstring fontName;

    //The full path font PNG filename
    std::wstring imageFile;

    //the real path location of the font definition file
    std::wstring fontDefFileSource;

    GLuint texId;
    int gcCounter;
    SpinMutex cache_busy;

public:

    //Proxy class computing and caching the selection of the underlying fonts
    //depending of the user input and requested scale for the fonts.
    class Drawer {

    private:
       
        //result of the computation
        int renderingFontIndex = 0;

        double renderingFontScaleFactor = 1.0;

    public:

        Drawer(int basicFontSize, double scaleFactor);

        //Public drawing font, 16 bit char version.
        void drawString(const std::wstring& str, float xpos, float ypos, Align hAlign = GLFONT_ALIGN_LEFT, Align vAlign = GLFONT_ALIGN_TOP, int vpx = 0, int vpy = 0, bool cacheable = false) const;

        //Public drawing font, 8 bit char version.
        void drawString(const std::string& str, float xpos, float ypos, Align hAlign = GLFONT_ALIGN_LEFT, Align vAlign = GLFONT_ALIGN_TOP, int vpx = 0, int vpy = 0, bool cacheable = false) const;
          
    }; //end class Drawer

    //The User request a font of size requestedSize to display, with an additional 
    //optional scale factor scaleFactor.
    static GLFont::Drawer getFont(int requestedSize, double scaleFactor = 1.0);

};

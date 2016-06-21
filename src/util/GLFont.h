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

    //The User request a font to display, but internally
    //it will be translated to another font depending of the scale level
    static GLFont& getFont(GLFontSize esize);

    //Called to change the scale of the rendered fonts
    static void setScale(GLFontScale scale);

    static GLFontScale getScale();

    //Public drawing font, 16 bit char version.
    void drawString(const std::wstring& str, float xpos, float ypos, Align hAlign = GLFONT_ALIGN_LEFT, Align vAlign = GLFONT_ALIGN_TOP, int vpx=0, int vpy=0, bool cacheable = false);

    //Public drawing font, 8 bit char version.
    void drawString(const std::string& str, float xpos, float ypos, Align hAlign = GLFONT_ALIGN_LEFT, Align vAlign = GLFONT_ALIGN_TOP, int vpx = 0, int vpy = 0, bool cacheable = false);
   
private:
   
    std::wstring nextParam(std::wistringstream &str);
    std::wstring getParamKey(const std::wstring& param_str);
    std::wstring getParamValue(const std::wstring& param_str);

    //Repository of all loaded fonts
    static GLFont fonts[GLFontSize::GLFONT_SIZE_MAX];

    //Map of user requested font to internal font, which changes
    //changes with the requested scale.
    // this is rebuilt by the user calling setScale(GLFontScale) and changed atomically,
    //which map a user-requested font to a final one depending of the zoom level.
    static GLFontSize userFontZoomMapping[GLFontSize::GLFONT_SIZE_MAX];

    static GLFontScale currentScaleFactor;

    //load a given font file, (lazy loading) 
    void loadFontOnce();

    GLFontStringCache *cacheString(const std::wstring& str, int pxHeight, int vpx, int vpy);
    void drawCacheString(GLFontStringCache *fc, float xpos, float ypos, Align hAlign, Align vAlign);

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

    std::wstring fontName;
    std::wstring imageFile;
    std::wstring fontFileSource;
    GLuint texId;
    int gcCounter;
    std::mutex cache_busy;

    static std::mutex g_userFontZoomMappingMutex;
};

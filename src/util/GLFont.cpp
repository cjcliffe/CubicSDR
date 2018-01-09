// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "GLFont.h"

#include <wx/string.h>

#include <iostream>
#include <fstream>
#include <algorithm>
#include "cubic_math.h"

#ifdef _OSX_APP_
#include "CoreFoundation/CoreFoundation.h"
#endif

#ifndef RES_FOLDER
#define RES_FOLDER ""
#endif

#define GC_DRAW_COUNT_PERIOD 50
#define GC_DRAW_COUNT_LIMIT 10

GLFontStringCache::GLFontStringCache() {
    gc = 0;
}

//Static initialization of all available fonts,
//using aggregate syntax (Cx11+)

//Fonts must be listed in increasing size for Drawer to work !
GLFont GLFont::fonts[GLFont::GLFontSize::GLFONT_SIZE_MAX] = {

    { GLFont::GLFontSize::GLFONT_SIZE12, L"fonts/vera_sans_mono12.fnt" },
    { GLFont::GLFontSize::GLFONT_SIZE16, L"fonts/vera_sans_mono16.fnt" },
    { GLFont::GLFontSize::GLFONT_SIZE18, L"fonts/vera_sans_mono18.fnt" },
    { GLFont::GLFontSize::GLFONT_SIZE24, L"fonts/vera_sans_mono24.fnt" },
    { GLFont::GLFontSize::GLFONT_SIZE27, L"fonts/vera_sans_mono27.fnt" },
    { GLFont::GLFontSize::GLFONT_SIZE32, L"fonts/vera_sans_mono32.fnt" },
    { GLFont::GLFontSize::GLFONT_SIZE36, L"fonts/vera_sans_mono36.fnt" },
    { GLFont::GLFontSize::GLFONT_SIZE48, L"fonts/vera_sans_mono48.fnt" },
    { GLFont::GLFontSize::GLFONT_SIZE64, L"fonts/vera_sans_mono64.fnt" },
    { GLFont::GLFontSize::GLFONT_SIZE72, L"fonts/vera_sans_mono72.fnt" },
    { GLFont::GLFontSize::GLFONT_SIZE96, L"fonts/vera_sans_mono96.fnt" },

};


std::atomic<GLFont::GLFontScale> GLFont::currentScale{ GLFont::GLFontScale::GLFONT_SCALE_NORMAL };


GLFontChar::GLFontChar() :
        id(0), x(0), y(0), width(0), height(0), xoffset(0), yoffset(0), xadvance(0), aspect(1), index(0) {

}

GLFontChar::~GLFontChar() {

}

void GLFontChar::setId(int idval) {
    id = idval;
}

int GLFontChar::getId() {
    return id;
}

void GLFontChar::setXOffset(int xofs) {
    xoffset = xofs;
}

int GLFontChar::getXOffset() {
    return xoffset;
}

void GLFontChar::setYOffset(int yofs) {
    yoffset = yofs;
}

int GLFontChar::getYOffset() {
    return yoffset;
}

void GLFontChar::setX(int xpos) {
    x = xpos;
}

int GLFontChar::getX() {
    return x;
}

void GLFontChar::setY(int ypos) {
    y = ypos;
}

int GLFontChar::getY() {
    return y;
}

void GLFontChar::setWidth(int w) {
    width = w;
    if (width && height) {
        aspect = (float) width / (float) height;
    }
}

int GLFontChar::getWidth() {
    return width;
}

void GLFontChar::setHeight(int h) {
    height = h;
    if (width && height) {
        aspect = (float) width / (float) height;
    }
}

int GLFontChar::getHeight() {
    return height;
}

void GLFontChar::setXAdvance(int xadv) {
    xadvance = xadv;
}

int GLFontChar::getXAdvance() {
    return xadvance;
}

float GLFontChar::getAspect() {
    return aspect;
}

void GLFontChar::setIndex(unsigned int idx) {
    index = idx;
}

int GLFontChar::getIndex() {
    return index;
}

GLFont::GLFont(GLFontSize size, std::wstring defFileName):
        lineHeight(0), base(0), imageWidth(0), imageHeight(0), loaded(false), texId(0), gcCounter(0) {

    fontSizeClass = size;
  
    fontDefFileSource = defFileName;
}

GLFont::~GLFont() {

}

std::wstring GLFont::nextParam(std::wistringstream &str) {
    std::wstring param_str;

    str >> param_str;

    if (param_str.find(L'"') != std::wstring::npos) {
        std::wstring rest;
        while (!str.eof() && (std::count(param_str.begin(), param_str.end(), L'"') % 2)) {
            str >> rest;
            param_str.append(L" " + rest);
        }
    }

    return param_str;
}

std::wstring GLFont::getParamKey(const std::wstring& param_str) {
    std::wstring keyName;

    size_t eqpos = param_str.find(L"=");

    if (eqpos != std::wstring::npos) {
        keyName = param_str.substr(0, eqpos);
    }

    return keyName;
}

std::wstring GLFont::getParamValue(const std::wstring& param_str) {
    std::wstring value;

    size_t eqpos = param_str.find(L"=");

    if (eqpos != std::wstring::npos) {
        value = param_str.substr(eqpos + 1);
    }

    if (value[0] == L'"' && value[value.length() - 1] == L'"') {
        value = value.substr(1, value.length() - 2);
    }

    return value;
}

void GLFont::loadFontOnce() {

    if (loaded) {
        return;
    }

#if _OSX_APP_
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
    {
        // error!
    }
    CFRelease(resourcesURL);
    wxString resourceFolder = std::string(path) + "/";
    
#else
    wxString resourceFolder = RES_FOLDER;
#endif
    
    //full font file path
    wxFileName fontDefFileName = wxFileName(resourceFolder + L"/" + fontDefFileSource);
    
    if (!fontDefFileName.Exists()) {
        wxFileName exePath = wxFileName(wxStandardPaths::Get().GetExecutablePath());
       
        //Full Path where the fonts are, including file name
        fontDefFileName = wxFileName(exePath.GetPath() + L"/"+ fontDefFileSource);

        if (!fontDefFileName.FileExists()) {
            std::cout << "Font file " << fontDefFileName.GetFullPath() << " does not exist?" << std::endl;
            return;
        }

        if (!fontDefFileName.IsFileReadable()) {
            std::cout << "Font file " << fontDefFileName.GetFullPath() << " is not readable?" << std::endl;
            return;
        }
    }
    else {

        if (!fontDefFileName.IsFileReadable()) {
            std::cout << "Font file " << fontDefFileName.GetFullPath() << " is not readable?" << std::endl;
            return;
        }
    }

    //Re-compute the resource dir.
    resourceFolder = fontDefFileName.GetPath();

    std::string fontDefFileNamePath = fontDefFileName.GetFullPath(wxPATH_NATIVE).ToStdString();
    
    std::wifstream input;
   
    input.open(fontDefFileNamePath, std::ios::in);

    std::wstring op;

    while (!input.eof()) {
        input >> op;
        if (op == L"info") {
            std::wstring info_param_str;
            getline(input, info_param_str);
            std::wistringstream info_param(info_param_str);

            while (!info_param.eof()) {
                std::wstring param = nextParam(info_param);

                std::wstring paramKey = getParamKey(param);               
                if (paramKey == L"face") {
                    fontName = getParamValue(param);
                }

                param = nextParam(info_param);
                paramKey = getParamKey(param);
                if (paramKey == L"size") {

                    std::wistringstream paramValue(getParamValue(param));
                    paramValue >> pixHeight;
                }

//                std::cout << "[" << paramKey << "] = '" << paramValue << "'" << std::endl;
            }
        } else if (op == L"common") {
            std::wstring common_param_str;
            getline(input, common_param_str);
            std::wistringstream common_param(common_param_str);

            while (!common_param.eof()) {
                std::wstring param = nextParam(common_param);

                std::wstring paramKey = getParamKey(param);
                std::wistringstream paramValue(getParamValue(param));

                if (paramKey == L"lineHeight") {
                    paramValue >> lineHeight;
                } else if (paramKey == L"base") {
                    paramValue >> base;
                } else if (paramKey == L"scaleW") {
                    paramValue >> imageWidth;
                } else if (paramKey == L"scaleH") {
                    paramValue >> imageHeight;
                }
//                std::cout << "[" << paramKey << "] = '" << getParamValue(param) << "'" << std::endl;
            }
        } else if (op == L"page") {
            std::wstring page_param_str;
            getline(input, page_param_str);
            std::wistringstream page_param(page_param_str);

            while (!page_param.eof()) {
                std::wstring param = nextParam(page_param);

                std::wstring paramKey = getParamKey(param);
                std::wstring paramValue = getParamValue(param);

                if (paramKey == L"file") {
                    wxFileName imgFileName = wxFileName(resourceFolder, paramValue);
                    imageFile = imgFileName.GetFullPath(wxPATH_NATIVE).ToStdWstring();
                }
//                std::cout << "[" << paramKey << "] = '" << paramValue << "'" << std::endl;
            }

        } else if (op == L"char") {
            std::wstring char_param_str;
            getline(input, char_param_str);
            std::wistringstream char_param(char_param_str);

            GLFontChar *newChar = new GLFontChar;

            while (!char_param.eof()) {
                std::wstring param = nextParam(char_param);

                std::wstring paramKey = getParamKey(param);
                std::wistringstream paramValue(getParamValue(param));

                int val;

                if (paramKey == L"id") {
                    paramValue >> val;
                    newChar->setId(val);
                } else if (paramKey == L"x") {
                    paramValue >> val;
                    newChar->setX(val);
                } else if (paramKey == L"y") {
                    paramValue >> val;
                    newChar->setY(val);
                } else if (paramKey == L"width") {
                    paramValue >> val;
                    newChar->setWidth(val);
                } else if (paramKey == L"height") {
                    paramValue >> val;
                    newChar->setHeight(val);
                } else if (paramKey == L"xoffset") {
                    paramValue >> val;
                    newChar->setXOffset(val);
                } else if (paramKey == L"yoffset") {
                    paramValue >> val;
                    newChar->setYOffset(val);
                } else if (paramKey == L"xadvance") {
                    paramValue >> val;
                    newChar->setXAdvance(val);
                }

//                std::cout << "[" << paramKey << "] = '" << getParamValue(param) << "'" << std::endl;
            }

            characters[newChar->getId()] = newChar;

        } else {
            std::wstring dummy;
            getline(input, dummy);
        }
    }

    if (imageFile != "" && imageWidth && imageHeight && characters.size()) {

        // Load file and decode image.
        std::vector<unsigned char> image;

        unsigned int imgWidth = imageWidth, imgHeight = imageHeight;

        //1) First load the raw file to memory using wstring filenames
        wxFile png_file(imageFile);

        int png_size = png_file.Length();
        
        unsigned char* raw_image = new unsigned char[png_size];

        if (png_size > 0) {

            int nbRead = png_file.Read((void*)raw_image, png_size);

            if (png_size != nbRead) {

                std::cout << "Error loading the full PNG image file in memory: '" << imageFile << "'" << std::endl;
            }
        }

        //2) then load from memory
        unsigned error = lodepng::decode(image, imgWidth, imgHeight, raw_image, png_size);

        delete[] raw_image;
        png_file.Close();

        if (error) {
            std::cout << "Error decoding PNG image file: '" << imageFile << "'" << std::endl;
        }
 
        glGenTextures(1, &texId);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, error?nullptr:(&image[0]));
        glDisable(GL_TEXTURE_2D);

        std::map<int, GLFontChar *>::iterator char_i;

        gl_vertices.resize(characters.size() * 8); // one quad per char
        gl_uv.resize(characters.size() * 8);

        unsigned int ofs = 0;
        for (char_i = characters.begin(); char_i != characters.end(); char_i++) {
//            int charId = (*char_i).first;
            GLFontChar *fchar = (*char_i).second;

            float faspect = fchar->getAspect();

            float uv_xpos = (float) fchar->getX() / (float) imageWidth;
            float uv_ypos = ((float) fchar->getY() / (float) imageHeight);
            float uv_xofs = (float) fchar->getWidth() / (float) imageWidth;
            float uv_yofs = ((float) fchar->getHeight() / (float) imageHeight);

            gl_vertices[ofs] = 0;
            gl_vertices[ofs + 1] = 0;
            gl_uv[ofs] = uv_xpos;
            gl_uv[ofs + 1] = uv_ypos + uv_yofs;

            gl_vertices[ofs + 2] = faspect;
            gl_vertices[ofs + 3] = 0;
            gl_uv[ofs + 2] = uv_xpos + uv_xofs;
            gl_uv[ofs + 3] = uv_ypos + uv_yofs;

            gl_vertices[ofs + 4] = faspect;
            gl_vertices[ofs + 5] = 1;
            gl_uv[ofs + 4] = uv_xpos + uv_xofs;
            gl_uv[ofs + 5] = uv_ypos;

            gl_vertices[ofs + 6] = 0;
            gl_vertices[ofs + 7] = 1;
            gl_uv[ofs + 6] = uv_xpos;
            gl_uv[ofs + 7] = uv_ypos;

            fchar->setIndex(ofs);

            ofs += 8;
        }

        std::cout << "Loaded font '" << fontName << "' from '" << imageFile << "', parsed " << characters.size() << " characters." << std::endl;

        loaded = true;
    } else {
        std::cout << "Error loading font file " << imageFile << std::endl;
    }

    input.close();
    loaded = true;
}

float GLFont::getStringWidth(const std::wstring& str, float size, float viewAspect) {

    float scalex = size / viewAspect;

    float width = 0;

    for (int i = 0, iMax = str.length(); i < iMax; i++) {
        int charId = str.at(i);

        if (characters.find(charId) == characters.end()) {
            continue;
        }

        GLFontChar *fchar = characters[charId];

        float ofsx = (float) fchar->getXOffset() / (float) imageWidth;
        float advx = (float) fchar->getXAdvance() / (float) imageWidth;

        if (charId == 32) {
            advx = characters[L'_']->getAspect();
        }

        width += fchar->getAspect() + advx + ofsx;
    }

    width *= scalex;

    return width;
}

// Draw string, immediate
void GLFont::drawString(const std::wstring& str, int pxHeight, float xpos, float ypos, Align hAlign, Align vAlign, int vpx, int vpy, bool cacheable) {

    pxHeight *= 2;
  
    if (!vpx || !vpy) {
        GLint vp[4];
        glGetIntegerv( GL_VIEWPORT, vp);
        vpx = vp[2];
        vpy = vp[3];
    }
    
    if (cacheable) {
        gcCounter++;

        std::lock_guard<std::mutex> lock(cache_busy);
        
        if (gcCounter > GC_DRAW_COUNT_PERIOD) {
            
            doCacheGC();
            gcCounter = 0;
        }
        
        GLFontStringCache *fc = nullptr;

        std::map<std::wstring, GLFontStringCache * >::iterator cache_iter;
        
        std::wstringstream sscacheIdx;
        
        sscacheIdx << vpx << "." << vpy << "." << pxHeight << "." << str;
        
        std::wstring cacheIdx(sscacheIdx.str());
        
        cache_iter = stringCache.find(cacheIdx);
        if (cache_iter != stringCache.end()) {
            fc = cache_iter->second;
            fc->gc = 0;
        }
        
        if (fc == nullptr) {
//            std::cout << "cache miss" << std::endl;
            fc = cacheString(str, pxHeight, vpx, vpy);
            stringCache[cacheIdx] = fc;
        }

        drawCacheString(fc, xpos, ypos, hAlign, vAlign);
        
        return;
    }
    
    float size = (float) pxHeight / (float) vpy;
    float viewAspect = (float) vpx / (float) vpy;
    float msgWidth = getStringWidth(str, size, viewAspect);

    glPushMatrix();
    glTranslatef(xpos, ypos, 0.0f);

    switch (vAlign) {
    case GLFONT_ALIGN_TOP:
        glTranslatef(0.0, -size, 0.0);
        break;
    case GLFONT_ALIGN_CENTER:
        glTranslatef(0.0, -size/2.0, 0.0);
        break;
    default:
        break;
    }

    switch (hAlign) {
    case GLFONT_ALIGN_RIGHT:
        glTranslatef(-msgWidth, 0.0, 0.0);
        break;
    case GLFONT_ALIGN_CENTER:
        glTranslatef(-msgWidth / 2.0, 0.0, 0.0);
        break;
    default:
        break;
    }

    glPushMatrix();
    glScalef(size / viewAspect, size, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texId);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, &gl_vertices[0]);
    glTexCoordPointer(2, GL_FLOAT, 0, &gl_uv[0]);

    for (int i = 0, iMax = str.length(); i < iMax; i++) {
        int charId = str.at(i);

        if (characters.find(charId) == characters.end()) {
            continue;
        }

        GLFontChar *fchar = characters[charId];

        float ofsx = (float) fchar->getXOffset() / (float) imageWidth;
        float advx = (float) fchar->getXAdvance() / (float) imageWidth;

        if (charId == 32) {
            advx = characters[L'_']->getAspect();
        }

        glTranslatef(ofsx, 0.0, 0.0);
        glDrawArrays(GL_QUADS, fchar->getIndex() / 2, 4);
        glTranslatef(fchar->getAspect() + advx, 0.0, 0.0);
    }

    glVertexPointer(2, GL_FLOAT, 0, nullptr);
    glTexCoordPointer(2, GL_FLOAT, 0, nullptr);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glPopMatrix();
    glPopMatrix();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

// Draw string, immediate, 8 bit version
void GLFont::drawString(const std::string& str, int pxHeight, float xpos, float ypos, Align hAlign, Align vAlign, int vpx, int vpy, bool cacheable) {

    //Displayed string is wstring, so use wxString to do the heavy lifting of converting  str...
    wxString wsTmp;
    
    wsTmp.assign(str);

    drawString(wsTmp.ToStdWstring(), pxHeight, xpos, ypos, hAlign, vAlign, vpx, vpy, cacheable);
}

// Draw cached GLFontCacheString
void GLFont::drawCacheString(GLFontStringCache *fc, float xpos, float ypos, Align hAlign, Align vAlign) {
    
    float size = (float) fc->pxHeight / (float) fc->vpy;
    
    glPushMatrix();
    glTranslatef(xpos, ypos, 0.0f);
    
    switch (vAlign) {
        case GLFONT_ALIGN_TOP:
            glTranslatef(0.0, -size, 0.0);
            break;
        case GLFONT_ALIGN_CENTER:
            glTranslatef(0.0, -size/2.0, 0.0);
            break;
        default:
            break;
    }
    
    switch (hAlign) {
        case GLFONT_ALIGN_RIGHT:
            glTranslatef(-fc->msgWidth, 0.0, 0.0);
            break;
        case GLFONT_ALIGN_CENTER:
            glTranslatef(-fc->msgWidth / 2.0, 0.0, 0.0);
            break;
        default:
            break;
    }
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texId);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, &fc->gl_vertices[0]);
    glTexCoordPointer(2, GL_FLOAT, 0, &fc->gl_uv[0]);
    
    glDrawArrays(GL_QUADS, 0, 4 * fc->drawlen);
    
    glVertexPointer(2, GL_FLOAT, 0, nullptr);
    glTexCoordPointer(2, GL_FLOAT, 0, nullptr);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glPopMatrix();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

// Compile optimized GLFontCacheString
GLFontStringCache *GLFont::cacheString(const std::wstring& str, int pxHeight, int vpx, int vpy) {

    GLFontStringCache *fc = new GLFontStringCache;
    
    fc->pxHeight = pxHeight;
    fc->vpx = vpx;
    fc->vpy = vpy;
    
    float size = (float) pxHeight / (float) vpy;
    float viewAspect = (float) vpx / (float) vpy;
    
    fc->msgWidth = getStringWidth(str, size, viewAspect);

    int nChar = 0;
    for (int i = 0, iMax = str.length(); i < iMax; i++) {
        int charId = str.at(i);

        if (characters.find(charId) == characters.end()) {
            continue;
        }
        nChar++;
    }
    
    fc->drawlen = nChar;
    fc->gl_vertices.resize(nChar*8);
    fc->gl_uv.resize(nChar*8);
    
    
    CubicVR::mat4 trans = CubicVR::mat4::scale(size / viewAspect, size, 1.0f);
    
    int c = 0;
    for (int i = 0, iMax = str.length(); i < iMax; i++) {
        int charId = str.at(i);
        
        if (characters.find(charId) == characters.end()) {
            continue;
        }
        
        GLFontChar *fchar = characters[charId];
        
        float ofsx = (float) fchar->getXOffset() / (float) imageWidth;
        float advx = (float) fchar->getXAdvance() / (float) imageWidth;
        
        if (charId == 32) {
            advx = characters[L'_']->getAspect();
        }
        
        // freeze transform to buffer
        trans *= CubicVR::mat4::translate(ofsx, 0.0, 0.0);
        int charIdx = fchar->getIndex();
        for (int j = 0; j < 8; j+=2) {
            CubicVR::vec3 pt(gl_vertices[charIdx + j],gl_vertices[charIdx + j + 1], 0.0);
            pt = CubicVR::mat4::multiply(trans, pt, true);
            fc->gl_vertices[c * 8 + j] = pt[0];
            fc->gl_vertices[c * 8 + j + 1] = pt[1];
            fc->gl_uv[c * 8 + j] = gl_uv[charIdx + j];
            fc->gl_uv[c * 8 + j + 1] = gl_uv[charIdx + j + 1];
        }
        trans *= CubicVR::mat4::translate(fchar->getAspect() + advx, 0.0, 0.0);
        c++;
    }
    
    return fc;
}

void GLFont::doCacheGC() {

    std::map<std::wstring, GLFontStringCache * >::iterator cache_iter;

    bool flushDone = false;

    //do aging and remove in one pass.
    cache_iter = stringCache.begin();

    while (cache_iter != stringCache.end()) {

        //aging
        cache_iter->second->gc--;

        //only flush 1 element per call
        if (!flushDone && cache_iter->second->gc < -GC_DRAW_COUNT_LIMIT) {

            delete cache_iter->second;
            cache_iter = stringCache.erase(cache_iter);
            flushDone = true;
        }
        else {
            cache_iter++;
        }
    } //end while
}

void GLFont::clearCache() {

    std::lock_guard<std::mutex> lock(cache_busy);

    std::map<std::wstring, GLFontStringCache * >::iterator cache_iter;

    cache_iter = stringCache.begin();

    while (cache_iter != stringCache.end()) {
        
        delete cache_iter->second;
        cache_iter = stringCache.erase(cache_iter);
               
    }
}

void GLFont::clearAllCaches() {

    for (int i = 0; i < GLFont::GLFONT_SIZE_MAX; i++) {

        fonts[i].clearCache();
    }
}


GLFont::Drawer GLFont::getFont(int requestedSize, double scaleFactor) {

    return GLFont::Drawer(requestedSize, scaleFactor);
}



void GLFont::setScale(GLFontScale scale) {
    
    //safety vs. inputs
    if (scale < GLFONT_SCALE_NORMAL || scale > GLFONT_SCALE_LARGE) {

        scale = GLFontScale::GLFONT_SCALE_NORMAL;
    }
    
    currentScale.store(scale);

    //Flush all the GC stuff
    clearAllCaches();
}

GLFont::GLFontScale GLFont::getScale() {

    return currentScale.load();
}

double GLFont::getScaleFactor() {

    GLFontScale scale = currentScale.load();

    if (scale == GLFONT_SCALE_MEDIUM) {

        return 1.5;
    }
    else if (scale == GLFONT_SCALE_LARGE) {

        return 2.0;
    }

    return 1.0;
}

int GLFont::getScaledPx(int basicFontSize, double scaleFactor) {
    //try to align on an integer pixel size if the targetSize font is available
    int targetSize = round(basicFontSize * scaleFactor);
    int resultIndex = 0;
    
    fonts[0].loadFontOnce();
    
    for (int i = 0; i < GLFONT_SIZE_MAX - 1; i++) {
        
        fonts[i + 1].loadFontOnce();
        
        if (fonts[i + 1].pixHeight <= targetSize) {
            resultIndex = i + 1;
        }
        else {
            break;
        }
    } //end for
    
    // return font height px
    return fonts[resultIndex].pixHeight;
}


GLFont::Drawer::Drawer(int basicFontSize, double scaleFactor) {

    //Selection of the final font: scan GLFont::fonts to find the biggest font such as 
    // its pixHeight <= basicFontSize * scaleFactor.
    //then compute finalScaleFactor the zooming factor of renderingFont to reach a 
    //final font size of  basicFontSize* scaleFactor:
    renderingFontIndex = 0;

    //try to align on an integer pixel size if the targetSize font is available
    int targetSize = round(basicFontSize * scaleFactor);

    fonts[0].loadFontOnce();

    for (int i = 0; i < GLFONT_SIZE_MAX - 1; i++) {

        fonts[i + 1].loadFontOnce();

        if (fonts[i + 1].pixHeight <= targetSize) {

            renderingFontIndex = i + 1;
        }
        else {
            break;
        }
    } //end for

      //
    int rawSize = fonts[renderingFontIndex].pixHeight;

    //targetSize may not be reached yet, so the effective rendering font: fonts[renderingFontIndex] must be scaled up a bit.
    renderingFontScaleFactor = (double) targetSize / rawSize;
}

void GLFont::Drawer::drawString(const std::wstring& str, float xpos, float ypos, Align hAlign, Align vAlign, int vpx, int vpy, bool cacheable) {

    GLFont& appliedFont = fonts[renderingFontIndex];

    appliedFont.drawString(str, round(appliedFont.pixHeight * renderingFontScaleFactor), xpos, ypos, hAlign, vAlign, vpx, vpy, cacheable);
}

//Public drawing font, 8 bit char version.
void GLFont::Drawer::drawString(const std::string& str, float xpos, float ypos, Align hAlign, Align vAlign, int vpx, int vpy, bool cacheable) {

    GLFont& appliedFont = fonts[renderingFontIndex];

    appliedFont.drawString(str, round(appliedFont.pixHeight * renderingFontScaleFactor), xpos, ypos, hAlign, vAlign, vpx, vpy, cacheable);
}


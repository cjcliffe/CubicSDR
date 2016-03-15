#include "GLFont.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include "cubic_math.h"


#ifdef _MSC_VER
#include <windows.h>

static std::string getExePath(void)
{
    HMODULE hModule = GetModuleHandle(NULL);
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);

    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    _splitpath_s(path, drive, dir, fname, ext);

    return std::string(drive) + std::string(dir);
}
#endif

#ifndef RES_FOLDER
#define RES_FOLDER ""
#endif

GLFontStringCache::GLFontStringCache() {
    gc = 0;
}

GLFont GLFont::fonts[GLFONT_MAX];

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

GLFont::GLFont() :
        lineHeight(0), base(0), imageWidth(0), imageHeight(0), loaded(false), texId(0), gcCounter(0) {

}

GLFont::~GLFont() {

}

std::string GLFont::nextParam(std::istringstream &str) {
    std::string param_str;

    str >> param_str;

    if (param_str.find('"') != std::string::npos) {
        std::string rest;
        while (!str.eof() && (std::count(param_str.begin(), param_str.end(), '"') % 2)) {
            str >> rest;
            param_str.append(" " + rest);
        }
    }

    return param_str;
}

std::string GLFont::getParamKey(std::string param_str) {
    std::string keyName;

    size_t eqpos = param_str.find("=");

    if (eqpos != std::string::npos) {
        keyName = param_str.substr(0, eqpos);
    }

    return keyName;
}

std::string GLFont::getParamValue(std::string param_str) {
    std::string value;

    size_t eqpos = param_str.find("=");

    if (eqpos != std::string::npos) {
        value = param_str.substr(eqpos + 1);
    }

    if (value[0] == '"' && value[value.length() - 1] == '"') {
        value = value.substr(1, value.length() - 2);
    }

    return value;
}

void GLFont::loadFont(std::string fontFile) {
    
    std::string resourceFolder = RES_FOLDER;
    #ifdef _MSC_VER
    resourceFolder = getExePath() + "/" + resourceFolder;
    #endif

    wxFileName fontFileName = wxFileName(resourceFolder, fontFile);
    
    if (!fontFileName.Exists()) {
        wxFileName exePath = wxFileName(wxStandardPaths::Get().GetExecutablePath());
        fontFileName = wxFileName(exePath.GetPath(), fontFile);
        resourceFolder = exePath.GetPath();
    }

    fontFileSource = fontFileName.GetFullPath(wxPATH_NATIVE).ToStdString();
    
    if (!fontFileName.FileExists()) {
        std::cout << "Font file " << fontFileSource << " does not exist?" << std::endl;
        return;
    }
    
    if (!fontFileName.IsFileReadable()) {
        std::cout << "Font file " << fontFileSource << " is not readable?" << std::endl;
        return;
    }

    
    std::ifstream input;
    input.open(fontFileSource.c_str(), std::ios::in);

    std::string op;

    while (!input.eof()) {
        input >> op;

        if (op == "info") {
            std::string info_param_str;
            getline(input, info_param_str);
            std::istringstream info_param(info_param_str);

            while (!info_param.eof()) {
                std::string param = nextParam(info_param);

                std::string paramKey = getParamKey(param);
                std::string paramValue = getParamValue(param);

                if (paramKey == "face") {
                    fontName = paramValue;
                }

//                std::cout << "[" << paramKey << "] = '" << paramValue << "'" << std::endl;
            }
        } else if (op == "common") {
            std::string common_param_str;
            getline(input, common_param_str);
            std::istringstream common_param(common_param_str);

            while (!common_param.eof()) {
                std::string param = nextParam(common_param);

                std::string paramKey = getParamKey(param);
                std::istringstream paramValue(getParamValue(param));

                if (paramKey == "lineHeight") {
                    paramValue >> lineHeight;
                } else if (paramKey == "base") {
                    paramValue >> base;
                } else if (paramKey == "scaleW") {
                    paramValue >> imageWidth;
                } else if (paramKey == "scaleH") {
                    paramValue >> imageHeight;
                }
//                std::cout << "[" << paramKey << "] = '" << getParamValue(param) << "'" << std::endl;
            }
        } else if (op == "page") {
            std::string page_param_str;
            getline(input, page_param_str);
            std::istringstream page_param(page_param_str);

            while (!page_param.eof()) {
                std::string param = nextParam(page_param);

                std::string paramKey = getParamKey(param);
                std::string paramValue = getParamValue(param);

                if (paramKey == "file") {
                    wxFileName imgFileName = wxFileName(resourceFolder, paramValue);
                    imageFile = imgFileName.GetFullPath(wxPATH_NATIVE).ToStdString();
                }
//                std::cout << "[" << paramKey << "] = '" << paramValue << "'" << std::endl;
            }

        } else if (op == "char") {
            std::string char_param_str;
            getline(input, char_param_str);
            std::istringstream char_param(char_param_str);

            GLFontChar *newChar = new GLFontChar;

            while (!char_param.eof()) {
                std::string param = nextParam(char_param);

                std::string paramKey = getParamKey(param);
                std::istringstream paramValue(getParamValue(param));

                int val;

                if (paramKey == "id") {
                    paramValue >> val;
                    newChar->setId(val);
                } else if (paramKey == "x") {
                    paramValue >> val;
                    newChar->setX(val);
                } else if (paramKey == "y") {
                    paramValue >> val;
                    newChar->setY(val);
                } else if (paramKey == "width") {
                    paramValue >> val;
                    newChar->setWidth(val);
                } else if (paramKey == "height") {
                    paramValue >> val;
                    newChar->setHeight(val);
                } else if (paramKey == "xoffset") {
                    paramValue >> val;
                    newChar->setXOffset(val);
                } else if (paramKey == "yoffset") {
                    paramValue >> val;
                    newChar->setYOffset(val);
                } else if (paramKey == "xadvance") {
                    paramValue >> val;
                    newChar->setXAdvance(val);
                }

//                std::cout << "[" << paramKey << "] = '" << getParamValue(param) << "'" << std::endl;
            }

            characters[newChar->getId()] = newChar;

        } else {
            std::string dummy;
            getline(input, dummy);
        }
    }

    if (imageFile != "" && imageWidth && imageHeight && characters.size()) {

        // Load file and decode image.
        std::vector<unsigned char> image;
        unsigned int imgWidth = imageWidth, imgHeight = imageHeight;
        unsigned error = lodepng::decode(image, imgWidth, imgHeight, imageFile);

        if (error) {
            std::cout << "Error loading PNG image file: '" << imageFile << "'" << std::endl;
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

        std::cout << "Loaded font '" << fontName << "' from '" << fontFileSource << "', parsed " << characters.size() << " characters." << std::endl;
        loaded = true;
    } else {
        std::cout << "Error loading font file " << fontFileSource << std::endl;
    }

    input.close();
}

bool GLFont::isLoaded() {
    return loaded;
}

float GLFont::getStringWidth(std::string str, float size, float viewAspect) {

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
            advx = characters['_']->getAspect();
        }

        width += fchar->getAspect() + advx + ofsx;
    }

    width *= scalex;

    return width;
}

// Draw string, immediate
void GLFont::drawString(std::string str, float xpos, float ypos, int pxHeight, Align hAlign, Align vAlign, int vpx, int vpy, bool cacheable) {
    
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
        
        if (gcCounter > 200) {
            doCacheGC();
            gcCounter = 0;
        }
        
        GLFontStringCache *fc = nullptr;

        std::map<int, std::map<int, std::map<int, std::map<std::string, GLFontStringCache * > > > >::iterator i1;
        std::map<int, std::map<int, std::map<std::string, GLFontStringCache * > > >::iterator i2;
        std::map<int, std::map<std::string, GLFontStringCache * > >::iterator i3;
        std::map<std::string, GLFontStringCache * >::iterator i4;
        
        i1 = stringCache.find(vpx);
        if (i1 != stringCache.end()) {
            i2 = i1->second.find(vpy);
            if (i2 != i1->second.end()) {
                i3 = i2->second.find(pxHeight);
                if (i3 != i2->second.end()) {
                    i4 = i3->second.find(str);
                    if (i4 != i3->second.end()) {
                        fc = i4->second;
                        fc->gc = 0;
                    }
                }
            }
        }
        
        if (fc == nullptr) {
//            std::cout << "cache miss" << std::endl;
            fc = cacheString(str, pxHeight, vpx, vpy);
            stringCache[vpx][vpy][pxHeight][str] = fc;
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
            advx = characters['_']->getAspect();
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
GLFontStringCache *GLFont::cacheString(std::string str, int pxHeight, int vpx, int vpy) {
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
            advx = characters['_']->getAspect();
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
    std::map<int, std::map<int, std::map<int, std::map<std::string, GLFontStringCache * > > > >::iterator i1;
    std::map<int, std::map<int, std::map<std::string, GLFontStringCache * > > >::iterator i2;
    std::map<int, std::map<std::string, GLFontStringCache * > >::iterator i3;
    std::map<std::string, GLFontStringCache * >::iterator i4;
    
    std::vector<std::map<std::string, GLFontStringCache * >::iterator> removals;
    std::vector<std::map<std::string, GLFontStringCache * >::iterator>::iterator removals_i;
    
    for (i1 = stringCache.begin(); i1 != stringCache.end(); i1++) {
        for (i2 = i1->second.begin(); i2 != i1->second.end(); i2++) {
            for (i3 = i2->second.begin(); i3 != i2->second.end(); i3++) {
                for (i4 = i3->second.begin(); i4 != i3->second.end(); i4++) {
                    i4->second->gc--;
                    if (i4->second->gc < -10) {
                        removals.push_back(i4);
                    }
                }
            }
        }
    }

    if (removals.size()) {
        for (removals_i = removals.begin(); removals_i != removals.end(); removals_i++) {
//            std::cout << "gc'd " << (*removals_i)->first << std::endl;
            GLFontStringCache *fc = (*removals_i)->second;
            stringCache[fc->vpx][fc->vpy][fc->pxHeight].erase(*removals_i);
            delete (*removals_i)->second;
            
            if (!stringCache[fc->vpx][fc->vpy][fc->pxHeight].size()) {
                stringCache[fc->vpx][fc->vpy].erase(stringCache[fc->vpx][fc->vpy].find(fc->pxHeight));
                if (!stringCache[fc->vpx][fc->vpy].size()) {
                    stringCache[fc->vpx].erase(stringCache[fc->vpx].find(fc->vpy));
                    if (!stringCache[fc->vpx].size()) {
                        stringCache.erase(stringCache.find(fc->vpx));
                    }
                }
            }
        }
    }
}


GLFont &GLFont::getFont(GLFontSize esize) {
    if (!fonts[esize].isLoaded()) {
        
        std::string fontName;
        switch (esize) {
            case GLFONT_SIZE12:
                fontName = "vera_sans_mono12.fnt";
                break;
            case GLFONT_SIZE16:
                fontName = "vera_sans_mono16.fnt";
                break;
            case GLFONT_SIZE18:
                fontName = "vera_sans_mono18.fnt";
                break;
            case GLFONT_SIZE24:
                fontName = "vera_sans_mono24.fnt";
                break;
            case GLFONT_SIZE32:
                fontName = "vera_sans_mono32.fnt";
                break;
            case GLFONT_SIZE48:
                fontName = "vera_sans_mono48.fnt";
                break;
            default:
                fontName = "vera_sans_mono12.fnt";
                break;
        }
        
        fonts[esize].loadFont(fontName);
    }
    
    return fonts[esize];
}

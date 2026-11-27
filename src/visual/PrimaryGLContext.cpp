// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "PrimaryGLContext.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"

wxString PrimaryGLContext::glGetwxString(GLenum name) {
    const GLubyte *v = glGetString(name);
    if (v == nullptr) {
        // The error is not important. It is GL_INVALID_ENUM.
        // We just want to clear the error stack.
        glGetError();

        return wxString();
    }

    return wxString((const char*) v);
}

void PrimaryGLContext::CheckGLError() {
    GLenum errLast = GL_NO_ERROR;

    for (;;) {
        GLenum err = glGetError();
        if (err == GL_NO_ERROR)
            return;

        if (err == errLast) {
            std::cout << "OpenGL error state couldn't be reset." << std::endl;
            return;
        }

        errLast = err;

        std::cout << "OpenGL Error " << err << std::endl;
    }
}

bool PrimaryGLContext::SetCurrent(const wxGLCanvas& win) const {
    bool result = wxGLContext::SetCurrent(win);
#ifdef _WIN32
    // Swap interval is per-window state, and each context activates only its
    // own canvas, so one successful application lasts the context's life.
    if (result && wglSwapIntervalEXT && !swapIntervalApplied) {
        wglSwapIntervalEXT(0);
        swapIntervalApplied = true;
    }
#endif
    return result;
}

PrimaryGLContext::PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs* ctxAttrs) :
        wxGLContext(canvas, sharedContext, (const wxGLContextAttrs*) ctxAttrs) {



//#ifndef __linux__
//    SetCurrent(*canvas);
//    // Pre-load fonts
//    for (int i = 0; i < GLFONT_MAX; i++) {
//        getFont((GLFontSize) i);
//    }
//    CheckGLError();
//#endif
}

void PrimaryGLContext::BeginDraw(float r, float g, float b) {
    glClearColor(r,g,b, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void PrimaryGLContext::EndDraw() {
//    glFlush();

//    CheckGLError();
}

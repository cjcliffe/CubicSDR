#include "GLExt.h"
#include <cstring>
#include <iostream>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#endif

#ifdef __linux__
#include <dlfcn.h>
#endif

#ifdef _WIN32

PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = NULL;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT = NULL;

bool GLExtSupported(const char *extension_name) {
    const GLubyte *extensions = glGetString(GL_EXTENSIONS);

    return (std::strstr((const char *)extensions, extension_name) != NULL);
}

#endif


bool GLExt_initialized = false;

void initGLExtensions() {
    if (GLExt_initialized) {
        return;
    }

    const GLubyte *extensions = glGetString(GL_EXTENSIONS);

    std::cout << "Supported GL Extensions: " << extensions << std::endl;

    int interval = 2;

#ifdef _WIN32
    if (GLExtSupported("WGL_EXT_swap_control")) {
        std::cout << "Initializing WGL swap control extensions.." << std::endl;
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
        wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC) wglGetProcAddress("wglGetSwapIntervalEXT");

        wglSwapIntervalEXT(interval);
    }
#endif

#ifdef __APPLE__
    // OSX is just ON / OFF
    const GLint gl_interval = 1;
    CGLSetParameter (CGLGetCurrentContext(), kCGLCPSwapInterval, &gl_interval);
#endif

#ifdef __linux__
    dlopen("libglx.so",RTLD_LAZY);

    void (*glxSwapIntervalEXTFunc) (Display *dpy, GLXDrawable drawable, int interval);
    int (*glxSwapIntervalMESAFunc)(unsigned int interval);
    int (*glxSwapIntervalSGIFunc) (int interval);

    glxSwapIntervalEXTFunc = (void (*) (Display *dpy, GLXDrawable drawable, int interval)) dlsym(RTLD_DEFAULT,"glXSwapIntervalEXT");
    glxSwapIntervalMESAFunc = (int (*)(unsigned int interval)) dlsym(RTLD_DEFAULT,"glXSwapIntervalMESA");
    glxSwapIntervalSGIFunc = (int (*) (int interval)) dlsym(RTLD_DEFAULT,"glXSwapIntervalSGI");

    if (glxSwapIntervalEXTFunc) {
        Display *dpy = glXGetCurrentDisplay();
        GLXDrawable drawable = glXGetCurrentDrawable();
        glxSwapIntervalEXTFunc(dpy, drawable, interval);
    } else if (glxSwapIntervalMESAFunc) {
    	glxSwapIntervalMESAFunc(interval);
    } else if (glxSwapIntervalSGIFunc) {
    	glxSwapIntervalSGIFunc(interval);
    }
#endif

    GLExt_initialized = true;
}

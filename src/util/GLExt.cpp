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

//    const GLubyte *extensions = glGetString(GL_EXTENSIONS);
//    std::cout << std::endl << "Supported GL Extensions: " << std::endl << extensions << std::endl << std::endl;

#ifdef __APPLE__
    const GLint interval = 1;
#else
    const GLint interval = 2;
#endif

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
    CGLSetParameter (CGLGetCurrentContext(), kCGLCPSwapInterval, &interval);
#endif

#ifdef __linux__
    dlopen("libglx.so",RTLD_LAZY);

    void (*glxSwapIntervalEXTFunc) (Display *dpy, GLXDrawable drawable, int interval) = 0;
    int (*glxSwapIntervalMESAFunc)(unsigned int interval) = 0;
    int (*glxSwapIntervalSGIFunc) (int interval) = 0;
    void (*DRI2SwapIntervalFunc) (Display *dpy, XID drawable, int interval) = 0;

    glxSwapIntervalEXTFunc = (void (*) (Display *dpy, GLXDrawable drawable, int interval)) dlsym(RTLD_DEFAULT,"glXSwapIntervalEXT");
    glxSwapIntervalMESAFunc = (int (*)(unsigned int interval)) dlsym(RTLD_DEFAULT,"glXSwapIntervalMESA");
    glxSwapIntervalSGIFunc = (int (*) (int interval)) dlsym(RTLD_DEFAULT,"glXSwapIntervalSGI");
    DRI2SwapIntervalFunc = (void (*) (Display *dpy, XID drawable, int interval)) dlsym(RTLD_DEFAULT,"DRI2SwapInterval");

    std::cout << "Available vertical sync SwapInterval functions: " << std::endl;
    std::cout << "\tglxSwapIntervalEXT: " << ((glxSwapIntervalEXTFunc != 0)?"Yes":"No") << std::endl;
    std::cout << "\tDRI2SwapInterval: " << ((DRI2SwapIntervalFunc != 0)?"Yes":"No") << std::endl;
    std::cout << "\tglxSwapIntervalMESA: " << ((glxSwapIntervalMESAFunc != 0)?"Yes":"No") << std::endl;
    std::cout << "\tglxSwapIntervalSGI: " << ((glxSwapIntervalSGIFunc != 0)?"Yes":"No") << std::endl;

    if (glxSwapIntervalEXTFunc) {
        Display *dpy = glXGetCurrentDisplay();
        GLXDrawable drawable = glXGetCurrentDrawable();
        glxSwapIntervalEXTFunc(dpy, drawable, interval);
        std::cout << "Using glxSwapIntervalEXT." << std::endl << std::endl;
    } else if (DRI2SwapIntervalFunc) {
        Display *dpy = glXGetCurrentDisplay();
        GLXDrawable drawable = glXGetCurrentDrawable();
        DRI2SwapIntervalFunc(dpy, drawable, interval);
        std::cout << "Using DRI2SwapInterval." << std::endl << std::endl;
    } else if (glxSwapIntervalMESAFunc) {
    	glxSwapIntervalMESAFunc(interval);
        std::cout << "Using glxSwapIntervalMESA." << std::endl << std::endl;
    } else if (glxSwapIntervalSGIFunc) {
    	glxSwapIntervalSGIFunc(interval);
        std::cout << "Using glxSwapIntervalSGI." << std::endl << std::endl;
    } else {
        std::cout << "No vertical sync swap interval functions available." << std::endl;
    }
#endif

    GLExt_initialized = true;
}

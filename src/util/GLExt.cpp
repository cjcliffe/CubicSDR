#include "GLExt.h"
#include <cstring>
#include <iostream>

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
    long l_interval= interval;
    CGLSetParameter (CGLGetCurrentContext(), kCGLCPSwapInterval, &l_interval);
#endif

/*
#ifdef LINUX
    char* pcDummy = NULL;

    glXSwapIntervalSGIFunc glXSwapIntervalSGI = 0;
    glXSwapIntervalMESAFunc glXSwapIntervalMESA = 0;

    pcDummy = (char*)glXQueryExtensionsString (glXGetCurrentDisplay(), 0);
    if (strstr (pcDummy, "GLX_SGI_swap_control") != NULL) {
        glXSwapIntervalSGI = (glXSwapIntervalSGIFunc) dlsym(RTLD_DEFAULT,"glXSwapIntervalSGI") && glXSwapIntervalSGI (interval);

    } else if (strstr(pcDummy, "GLX_MESA_swap_control") != NULL) {
        glXSwapIntervalMESA = (glXSwapIntervalMESAFunc) dlsym(RTLD_DEFAULT,"glXSwapIntervalMESA") && glXSwapIntervalMESA (interval);
    }
#endif
*/

    GLExt_initialized = true;
}

#pragma once

#include "wx/glcanvas.h"

#ifdef _WIN32
#include <windows.h>
#include <gl/wglext.h>

extern PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT;
extern PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT;
extern PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT;

bool GLExtSupported(const char *extension_name);

#endif

extern bool GLExt_initialized;

void initGLExtensions();


// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/glcanvas.h"

#ifdef _WIN32
#include <windows.h>
#ifdef __MINGW32__
#include <gl/wglext.h>
#else
#include "wglext.h"
#endif

extern PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT;
extern PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT;
extern PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT;

bool GLExtSupported(const char *extension_name);

#endif

extern bool GLExt_initialized;

void initGLExtensions();


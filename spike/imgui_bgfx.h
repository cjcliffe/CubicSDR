// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <bgfx/bgfx.h>

struct ImFont;

struct ImguiInput {
    float mx = 0.0f;
    float my = 0.0f;
    bool left = false;
    bool right = false;
    bool middle = false;
    float wheelX = 0.0f;
    float wheelY = 0.0f;
};

bool imguiInit(bgfx::ViewId view);
void imguiShutdown();
void imguiBeginFrame(const ImguiInput &in, int width, int height, float dt);
void imguiEndFrame();
ImFont *imguiMonoFont();

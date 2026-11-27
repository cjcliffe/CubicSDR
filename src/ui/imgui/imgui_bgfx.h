// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>

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

bool imguiInitBackend();
void imguiShutdownBackend();
void imguiSetClearColor(uint32_t rgba);
void imguiBeginFrame(const ImguiInput &in, int width, int height, float dt, float fbScale = 1.0f);
void imguiEndFrame(bgfx::ViewId view);
ImFont *imguiMonoFont();

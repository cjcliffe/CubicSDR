// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// bgfx scope spike: an SDL3 window with bgfx rendering textured quads posed
// by the tested ScopeCarousel model with explicit glm matrices. Procedural
// quadrant texture; renderer pinned to OpenGL. No imgui yet (next spike).

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <bgfx/bgfx.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ScopeCarousel.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <glsl/vs.sc.bin.h>
#include <glsl/fs.sc.bin.h>
#include <essl/vs.sc.bin.h>
#include <essl/fs.sc.bin.h>

namespace {

const int START_W = 900;
const int START_H = 450;
const int TEX_SIZE = 256;
const float PANEL_SPACING = 0.4f;
const float FRUSTUM_HALF = 0.82842712f;
const float EYE_Z = -1.205f;

struct EmbeddedShader {
    bgfx::RendererType::Enum type;
    const char *name;
    const uint8_t *data;
    uint32_t size;
};

const EmbeddedShader kShaders[] = {
    {bgfx::RendererType::OpenGL, "vs", vs_glsl, (uint32_t)sizeof(vs_glsl)},
    {bgfx::RendererType::OpenGLES, "vs", vs_essl, (uint32_t)sizeof(vs_essl)},
    {bgfx::RendererType::OpenGL, "fs", fs_glsl, (uint32_t)sizeof(fs_glsl)},
    {bgfx::RendererType::OpenGLES, "fs", fs_essl, (uint32_t)sizeof(fs_essl)},
    {bgfx::RendererType::Count, nullptr, nullptr, 0},
};

bgfx::ShaderHandle loadShader(const char *name) {
    bgfx::RendererType::Enum type = bgfx::getRendererType();
    for (const EmbeddedShader *sh = kShaders; sh->type != bgfx::RendererType::Count; ++sh) {
        if (sh->type == type && std::strcmp(sh->name, name) == 0) {
            return bgfx::createShader(bgfx::makeRef(sh->data, sh->size));
        }
    }
    return BGFX_INVALID_HANDLE;
}

struct PosTexCoord0Vertex {
    float x, y, z;
    float u, v;
};

void buildTexture(std::vector<uint8_t> &out) {
    out.resize((size_t)TEX_SIZE * TEX_SIZE * 4);
    for (int y = 0; y < TEX_SIZE; y++) {
        for (int x = 0; x < TEX_SIZE; x++) {
            bool left = x < TEX_SIZE / 2;
            bool top = y < TEX_SIZE / 2;
            uint8_t r = 0, g = 0, b = 0;
            if (left && top) {
                r = 255;
            } else if (!left && top) {
                g = 255;
            } else if (left && !top) {
                b = 255;
            } else {
                r = 255;
                g = 255;
                b = 255;
            }
            if (std::abs(x - y) < 3 || std::abs(x + y - TEX_SIZE) < 3) {
                r = 0;
                g = 0;
                b = 0;
            }
            size_t o = ((size_t)y * TEX_SIZE + (size_t)x) * 4;
            out[o] = r;
            out[o + 1] = g;
            out[o + 2] = b;
            out[o + 3] = 255;
        }
    }
}

} // namespace

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    std::printf("drag to spin the carousel; close the window to quit\n");
    std::fflush(stdout);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("CubicSDR bgfx scope spike", START_W, START_H, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    bgfx::Init init;
    init.type = bgfx::RendererType::OpenGL;
    init.reset = BGFX_RESET_VSYNC;
    init.swapChain.width = (uint32_t)START_W;
    init.swapChain.height = (uint32_t)START_H;
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
#if defined(_WIN32)
    init.swapChain.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(__linux__)
    const char *driver = SDL_GetCurrentVideoDriver();
    if (driver && std::strcmp(driver, "wayland") == 0) {
        init.swapChain.ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        init.swapChain.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    } else {
        init.swapChain.ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        init.swapChain.nwh =
            (void *)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    }
#elif defined(__APPLE__)
    init.swapChain.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#endif
    bgfx::SwapChain swapChain = init.swapChain;
    if (!bgfx::init(init)) {
        std::printf("bgfx::init failed\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    std::printf("bgfx init ok: %s\n", bgfx::getRendererName(bgfx::getRendererType()));
    std::fflush(stdout);

    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    PosTexCoord0Vertex verts[4] = {
        {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, -1.0f, 0.0f, 1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f, 1.0f, 1.0f},
    };
    uint16_t indices[6] = {0, 1, 2, 1, 3, 2};
    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), layout);
    bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));

    std::vector<uint8_t> texData;
    buildTexture(texData);
    bgfx::TextureHandle tex = bgfx::createTexture2D((uint16_t)TEX_SIZE, (uint16_t)TEX_SIZE, false, 1,
        bgfx::TextureFormat::RGBA8, 0, bgfx::makeRef(texData.data(), (uint32_t)texData.size()));
    bgfx::UniformHandle s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

    bgfx::ShaderHandle vsh = loadShader("vs");
    bgfx::ShaderHandle fsh = loadShader("fs");
    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        std::printf("shader load failed\n");
        bgfx::shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);

    glm::mat4 proj = glm::frustum(-FRUSTUM_HALF, FRUSTUM_HALF, -FRUSTUM_HALF, FRUSTUM_HALF, 1.0f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, EYE_Z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));

    ScopeCarousel::CarouselState carousel;
    bool running = true, dragging = false;
    int frame = 0;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                swapChain.width = (uint32_t)e.window.data1;
                swapChain.height = (uint32_t)e.window.data2;
                bgfx::reset(BGFX_RESET_VSYNC, &swapChain);
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                dragging = true;
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
                dragging = false;
            } else if (e.type == SDL_EVENT_MOUSE_MOTION && dragging) {
                int ww, hh;
                SDL_GetWindowSize(window, &ww, &hh);
                ScopeCarousel::dragStep(carousel, e.motion.xrel / (float)ww);
            }
        }
        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        float interval = ScopeCarousel::carouselInterval(PANEL_SPACING);
        ScopeCarousel::restStep(carousel, PANEL_SPACING, dragging);
        float scopeX = carousel.ctr;
        bool showScope = ScopeCarousel::scopeVisibleStep(carousel, PANEL_SPACING);
        float specX = interval + carousel.ctr;
        bool showSpec = ScopeCarousel::spectrumVisibleStep(carousel, PANEL_SPACING);

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x101010ff);
        bgfx::setViewRect(0, 0, 0, (uint16_t)w, (uint16_t)h);
        bgfx::setViewTransform(0, glm::value_ptr(view), glm::value_ptr(proj));
        uint64_t state =
            BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA;

        if (showScope) {
            float tilt = ScopeCarousel::panelTiltDeg(scopeX);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(scopeX, 0.0f, 0.0f))
                * glm::rotate(glm::mat4(1.0f), glm::radians(-tilt), glm::vec3(0.0f, 1.0f, 0.0f));
            bgfx::setTransform(glm::value_ptr(model));
            bgfx::setVertexBuffer(0, vbh);
            bgfx::setIndexBuffer(ibh);
            bgfx::setTexture(0, s_texColor, tex);
            bgfx::setState(state);
            bgfx::submit(0, program);
        }
        if (showSpec) {
            float tilt = ScopeCarousel::panelTiltDeg(specX);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(specX, 0.0f, 0.0f))
                * glm::rotate(glm::mat4(1.0f), glm::radians(-tilt), glm::vec3(0.0f, 1.0f, 0.0f));
            bgfx::setTransform(glm::value_ptr(model));
            bgfx::setVertexBuffer(0, vbh);
            bgfx::setIndexBuffer(ibh);
            bgfx::setTexture(0, s_texColor, tex);
            bgfx::setState(state);
            bgfx::submit(0, program);
        }
        bgfx::frame();

        if (frame == 1) {
            std::printf("frame 1\n");
            std::fflush(stdout);
        }
        if ((frame++ % 60) == 0) {
            char title[128];
            std::snprintf(title, sizeof(title), "bgfx scope spike ctr=%.2f tgt=%.2f", carousel.ctr,
                carousel.ctrTarget);
            SDL_SetWindowTitle(window, title);
        }
    }

    std::printf("shutting down\n");
    std::fflush(stdout);
    bgfx::destroy(program);
    bgfx::destroy(tex);
    bgfx::destroy(vbh);
    bgfx::destroy(ibh);
    bgfx::destroy(s_texColor);
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

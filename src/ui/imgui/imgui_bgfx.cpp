// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "imgui_bgfx.h"

#include <imgui.h>

#include <bgfx/bgfx.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <robotomono_regular.ttf.h>

#include <glsl/vs_imgui.sc.bin.h>
#include <glsl/fs_imgui.sc.bin.h>
#include <essl/vs_imgui.sc.bin.h>
#include <essl/fs_imgui.sc.bin.h>

#include <cstring>

namespace {

struct EmbeddedShader {
    bgfx::RendererType::Enum type;
    const char *name;
    const uint8_t *data;
    uint32_t size;
};

const EmbeddedShader kShaders[] = {
    {bgfx::RendererType::OpenGL, "vs_imgui", vs_imgui_glsl, (uint32_t)sizeof(vs_imgui_glsl)},
    {bgfx::RendererType::OpenGLES, "vs_imgui", vs_imgui_essl, (uint32_t)sizeof(vs_imgui_essl)},
    {bgfx::RendererType::OpenGL, "fs_imgui", fs_imgui_glsl, (uint32_t)sizeof(fs_imgui_glsl)},
    {bgfx::RendererType::OpenGLES, "fs_imgui", fs_imgui_essl, (uint32_t)sizeof(fs_imgui_essl)},
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

struct BackendState {
    bgfx::ViewId view = 0;
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle s_texFont = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout layout;
    ImFont *monoFont = nullptr;
};

BackendState g_state;

bgfx::TextureHandle toBgfxTexture(ImTextureID id) {
    bgfx::TextureHandle handle;
    handle.idx = (uint16_t)(uintptr_t)id;
    return handle;
}

ImTextureID toImTextureId(bgfx::TextureHandle handle) {
    return (ImTextureID)(uintptr_t)handle.idx;
}

void updateTextures(ImDrawData *drawData) {
    if (drawData->Textures == nullptr) {
        return;
    }
    for (ImTextureData *tex : *drawData->Textures) {
        if (tex->Status == ImTextureStatus_WantCreate) {
            IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);
            bgfx::TextureHandle handle = bgfx::createTexture2D(
                (uint16_t)tex->Width, (uint16_t)tex->Height, false, 1, bgfx::TextureFormat::RGBA8, 0, NULL);
            const bgfx::Memory *mem = bgfx::copy(tex->GetPixels(), (uint32_t)tex->GetSizeInBytes());
            bgfx::updateTexture2D(
                handle, 0, 0, 0, 0, (uint16_t)tex->Width, (uint16_t)tex->Height, mem);
            tex->SetTexID(toImTextureId(handle));
            tex->SetStatus(ImTextureStatus_OK);
        } else if (tex->Status == ImTextureStatus_WantUpdates) {
            bgfx::TextureHandle handle = toBgfxTexture(tex->GetTexID());
            const ImTextureRect &r = tex->UpdateRect;
            const size_t bytesPerPixel = (size_t)tex->BytesPerPixel;
            const size_t rowBytes = (size_t)r.w * bytesPerPixel;
            const bgfx::Memory *mem = bgfx::alloc((uint32_t)(rowBytes * (size_t)r.h));
            const uint8_t *src = (const uint8_t *)tex->GetPixelsAt(r.x, r.y);
            for (int row = 0; row < r.h; row++) {
                std::memcpy(mem->data + (size_t)row * rowBytes, src + (size_t)row * tex->GetPitch(), rowBytes);
            }
            bgfx::updateTexture2D(
                handle, 0, 0, (uint16_t)r.x, (uint16_t)r.y, (uint16_t)r.w, (uint16_t)r.h, mem);
            tex->SetStatus(ImTextureStatus_OK);
        } else if (tex->Status == ImTextureStatus_WantDestroy) {
            bgfx::TextureHandle handle = toBgfxTexture(tex->GetTexID());
            if (bgfx::isValid(handle)) {
                bgfx::destroy(handle);
            }
            tex->SetTexID(ImTextureID_Invalid);
            tex->SetStatus(ImTextureStatus_Destroyed);
        }
    }
}

} // namespace

bool imguiInit(bgfx::ViewId view) {
    g_state.view = view;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    io.BackendRendererName = "cubic_bgfx";
    io.IniFilename = nullptr;

    io.Fonts->AddFontDefault();
    ImFontConfig monoConfig;
    monoConfig.FontDataOwnedByAtlas = false;
    g_state.monoFont = io.Fonts->AddFontFromMemoryTTF(
        (void *)s_robotoMonoRegularTtf, (int)sizeof(s_robotoMonoRegularTtf), 32.0f, &monoConfig);

    g_state.layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    g_state.s_texFont = bgfx::createUniform("s_texFont", bgfx::UniformType::Sampler);

    bgfx::ShaderHandle vsh = loadShader("vs_imgui");
    bgfx::ShaderHandle fsh = loadShader("fs_imgui");
    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        return false;
    }
    g_state.program = bgfx::createProgram(vsh, fsh, true);
    return bgfx::isValid(g_state.program);
}

void imguiShutdown() {
    ImGuiPlatformIO &pio = ImGui::GetPlatformIO();
    for (ImTextureData *tex : pio.Textures) {
        if (tex->Status != ImTextureStatus_Destroyed) {
            bgfx::TextureHandle handle = toBgfxTexture(tex->GetTexID());
            if (bgfx::isValid(handle)) {
                bgfx::destroy(handle);
            }
            tex->SetTexID(ImTextureID_Invalid);
            tex->SetStatus(ImTextureStatus_Destroyed);
        }
    }
    bgfx::destroy(g_state.program);
    bgfx::destroy(g_state.s_texFont);
    ImGui::DestroyContext();
}

void imguiBeginFrame(const ImguiInput &in, int width, int height, float dt) {
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.DeltaTime = dt > 0.0f ? dt : 1.0f / 60.0f;
    io.AddMousePosEvent(in.mx, in.my);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, in.left);
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, in.right);
    io.AddMouseButtonEvent(ImGuiMouseButton_Middle, in.middle);
    io.AddMouseWheelEvent(in.wheelX, in.wheelY);
    ImGui::NewFrame();
}

ImFont *imguiMonoFont() {
    return g_state.monoFont;
}

namespace {

uint16_t clampU16(float v, uint16_t lo, uint16_t hi) {
    if (v < (float)lo) {
        v = (float)lo;
    }
    if (v > (float)hi) {
        v = (float)hi;
    }
    return (uint16_t)v;
}

} // namespace

void imguiEndFrame() {
    ImGui::Render();
    ImDrawData *drawData = ImGui::GetDrawData();
    updateTextures(drawData);
    float fbW = drawData->DisplaySize.x * drawData->FramebufferScale.x;
    float fbH = drawData->DisplaySize.y * drawData->FramebufferScale.y;
    if (fbW <= 0.0f || fbH <= 0.0f || drawData->CmdListsCount == 0) {
        return;
    }
    uint16_t vw = (uint16_t)fbW;
    uint16_t vh = (uint16_t)fbH;
    bgfx::setViewRect(g_state.view, 0, 0, vw, vh);
    glm::mat4 view(1.0f);
    glm::mat4 proj = glm::ortho(drawData->DisplayPos.x, drawData->DisplayPos.x + drawData->DisplaySize.x,
        drawData->DisplayPos.y + drawData->DisplaySize.y, drawData->DisplayPos.y, -1.0f, 1.0f);
    bgfx::setViewTransform(g_state.view, glm::value_ptr(view), glm::value_ptr(proj));
    glm::mat4 model(1.0f);
    float *modelPtr = glm::value_ptr(model);

    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList *list = drawData->CmdLists[n];
        uint32_t vtxCount = (uint32_t)list->VtxBuffer.Size;
        uint32_t idxCount = (uint32_t)list->IdxBuffer.Size;
        if (vtxCount == 0 || idxCount == 0) {
            continue;
        }
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;
        bgfx::allocTransientVertexBuffer(&tvb, vtxCount, g_state.layout);
        bgfx::allocTransientIndexBuffer(&tib, idxCount, sizeof(ImDrawIdx) == 4);
        for (uint32_t i = 0; i < vtxCount; i++) {
            const ImDrawVert &v = list->VtxBuffer.Data[i];
            uint8_t *o = tvb.data + i * (uint32_t)tvb.stride;
            float *pos = (float *)o;
            pos[0] = v.pos.x;
            pos[1] = v.pos.y;
            pos[2] = 0.0f;
            float *uv = (float *)(o + 12);
            uv[0] = v.uv.x;
            uv[1] = v.uv.y;
            std::memcpy(o + 20, &v.col, sizeof(uint32_t));
        }
        std::memcpy(tib.data, list->IdxBuffer.Data, (size_t)idxCount * sizeof(ImDrawIdx));

        for (int cmd_i = 0; cmd_i < list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd &cmd = list->CmdBuffer.Data[cmd_i];
            if (cmd.UserCallback != nullptr) {
                cmd.UserCallback(list, &cmd);
                continue;
            }
            float cx0 = (cmd.ClipRect.x - drawData->DisplayPos.x) * drawData->FramebufferScale.x;
            float cy0 = (cmd.ClipRect.y - drawData->DisplayPos.y) * drawData->FramebufferScale.y;
            float cx1 = (cmd.ClipRect.z - drawData->DisplayPos.x) * drawData->FramebufferScale.x;
            float cy1 = (cmd.ClipRect.w - drawData->DisplayPos.y) * drawData->FramebufferScale.y;
            if (cx1 <= cx0 || cy1 <= cy0) {
                continue;
            }
            uint16_t sx = clampU16(cx0, 0, vw);
            uint16_t sy = clampU16(cy0, 0, vh);
            uint16_t ex = clampU16(cx1, 0, vw);
            uint16_t ey = clampU16(cy1, 0, vh);
            if (ex <= sx || ey <= sy) {
                continue;
            }
            bgfx::setScissor(sx, sy, ex - sx, ey - sy);
            bgfx::setVertexBuffer(0, &tvb, cmd.VtxOffset, vtxCount - cmd.VtxOffset);
            bgfx::setIndexBuffer(&tib, cmd.IdxOffset, cmd.ElemCount);
            bgfx::setTexture(0, g_state.s_texFont, toBgfxTexture(cmd.GetTexID()),
                BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT
                    | BGFX_SAMPLER_MIP_POINT);
            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA);
            bgfx::setTransform(modelPtr);
            bgfx::submit(g_state.view, g_state.program);
        }
    }
}

cmake_minimum_required(VERSION 3.21...3.28)

project(CubicSpikes)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Drop a <name>_spike.cpp in this directory and build it with
# `python build.py --spike <name>`; no other edits needed.
file(GLOB SPIKE_MAINS "*_spike.cpp")

include(FetchContent)

option(BGFXSPIKE_PREBUILT_SHADERS "Use committed shader headers instead of building shaderc" ON)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_AMALGAMATED ON CACHE BOOL "" FORCE)
if(BGFXSPIKE_PREBUILT_SHADERS)
    set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
else()
    set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
endif()
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    bgfx.cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG v1.168.9637-616
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(bgfx.cmake)
FetchContent_GetProperties(bgfx.cmake SOURCE_DIR BGFX_CMAKE_DIR)
FetchContent_GetProperties(bgfx SOURCE_DIR BGFX_SRC_DIR)
if(BGFXSPIKE_PREBUILT_SHADERS)
    set(SHADER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/imgui/shaders/generated)
    set(SHADER_SOURCES "")
else()
    set(BGFX_SHADER_INCLUDE_PATH ${BGFX_SRC_DIR}/src)
    if(NOT COMMAND bgfx_compile_shaders)
        include(${BGFX_CMAKE_DIR}/cmake/bgfxToolUtils.cmake)
    endif()
    bgfx_compile_shaders(
        TYPE VERTEX
        SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/imgui/shaders/vs_imgui.sc
        VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/imgui/shaders/varying.def.sc
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/generated/shaders
        PROFILES 430 300_es
        AS_HEADERS
    )
    bgfx_compile_shaders(
        TYPE FRAGMENT
        SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/imgui/shaders/fs_imgui.sc
        VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/imgui/shaders/varying.def.sc
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/generated/shaders
        PROFILES 430 300_es
        AS_HEADERS
    )
    set(SHADER_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/generated/shaders)
    set(SHADER_SOURCES
        ../src/ui/imgui/shaders/vs_imgui.sc
        ../src/ui/imgui/shaders/fs_imgui.sc)
endif()

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.20
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.93.0
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

set(IMGUI_SOURCES
    ../src/ui/imgui/imgui_bgfx.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp)

foreach(SPIKE_MAIN ${SPIKE_MAINS})
    get_filename_component(SPIKE_NAME ${SPIKE_MAIN} NAME_WE)
    add_executable(${SPIKE_NAME} ${SPIKE_MAIN} ${IMGUI_SOURCES} ${SHADER_SOURCES})
    target_include_directories(${SPIKE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/viewmodel
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/sdr
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/demod
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/util
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${imgui_SOURCE_DIR}
        ${SHADER_INCLUDE_DIR})
    target_link_libraries(${SPIKE_NAME} PRIVATE bgfx bx glm::glm)
    if(TARGET SDL3::SDL3)
        target_link_libraries(${SPIKE_NAME} PRIVATE SDL3::SDL3)
    else()
        target_link_libraries(${SPIKE_NAME} PRIVATE SDL3::SDL3-static)
    endif()
endforeach()

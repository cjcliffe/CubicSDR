cmake_minimum_required(VERSION 3.21...3.28)

project(CubicSpikes)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
# (Raylib spikes removed with the bgfx pivot; history keeps them.)

# bgfx stack (bgfx + SDL3 + glm) for the GPU-port spikes.
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
if(BGFXSPIKE_PREBUILT_SHADERS)
    set(SHADER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/generated)
else()
    FetchContent_GetProperties(bgfx.cmake SOURCE_DIR BGFX_CMAKE_DIR)
    FetchContent_GetProperties(bgfx SOURCE_DIR BGFX_SRC_DIR)
    set(BGFX_SHADER_INCLUDE_PATH ${BGFX_SRC_DIR}/src)
    if(NOT COMMAND bgfx_compile_shaders)
        include(${BGFX_CMAKE_DIR}/cmake/bgfxToolUtils.cmake)
    endif()
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

if(BGFXSPIKE_PREBUILT_SHADERS)
    add_executable(bgfxscope_spike bgfxscope_spike.cpp)
else()
    bgfx_compile_shaders(
        TYPE VERTEX
        SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs.sc
        VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying.def.sc
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/generated/shaders
        PROFILES 430 300_es
        AS_HEADERS
    )
    bgfx_compile_shaders(
        TYPE FRAGMENT
        SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs.sc
        VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying.def.sc
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/generated/shaders
        PROFILES 430 300_es
        AS_HEADERS
    )
    add_executable(bgfxscope_spike bgfxscope_spike.cpp shaders/vs.sc shaders/fs.sc)
    set(SHADER_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/generated/shaders)
endif()
target_include_directories(bgfxscope_spike PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/viewmodel
    ${SHADER_INCLUDE_DIR})
target_link_libraries(bgfxscope_spike PRIVATE bgfx bx glm::glm)
if(TARGET SDL3::SDL3)
    target_link_libraries(bgfxscope_spike PRIVATE SDL3::SDL3)
else()
    target_link_libraries(bgfxscope_spike PRIVATE SDL3::SDL3-static)
endif()

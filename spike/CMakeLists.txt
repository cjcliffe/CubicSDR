cmake_minimum_required(VERSION 3.15...3.28)

project(CubicSpikes)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(tuner_spike tuner_spike.cpp)
target_include_directories(tuner_spike PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/viewmodel ${CMAKE_CURRENT_SOURCE_DIR}/../src/sdr)
target_link_libraries(tuner_spike PRIVATE raylib)

add_executable(modesel_spike modesel_spike.cpp)
target_include_directories(modesel_spike PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/viewmodel)
target_link_libraries(modesel_spike PRIVATE raylib)

add_executable(spectrum_spike spectrum_spike.cpp)
target_include_directories(spectrum_spike PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/viewmodel ${CMAKE_CURRENT_SOURCE_DIR}/../src/sdr)
target_link_libraries(spectrum_spike PRIVATE raylib)

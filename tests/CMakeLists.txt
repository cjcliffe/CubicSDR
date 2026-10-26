# CubicSDR C++ unit tests (doctest, vendored under external/doctest).
#
# Configured only when BUILD_TESTING=ON. This target compiles just the
# sources under test plus the doctest entry point; it never links
# wxWidgets, OpenGL, SoapySDR, or liquid-dsp.

enable_testing()

add_executable(cubicsdr_tests
    main.cpp
    test_SpinMutex.cpp
    test_Gradient.cpp
    test_ThreadBlockingQueue.cpp
    test_DataTree.cpp
    test_Timer.cpp
    test_IOThread.cpp
    ${PROJECT_SOURCE_DIR}/src/util/Gradient.cpp
    ${PROJECT_SOURCE_DIR}/src/util/DataTree.cpp
    ${PROJECT_SOURCE_DIR}/src/util/Timer.cpp
    ${PROJECT_SOURCE_DIR}/src/IOThread.cpp
    ${PROJECT_SOURCE_DIR}/external/tinyxml/tinyxml.cpp
    ${PROJECT_SOURCE_DIR}/external/tinyxml/tinystr.cpp
    ${PROJECT_SOURCE_DIR}/external/tinyxml/tinyxmlparser.cpp
    ${PROJECT_SOURCE_DIR}/external/tinyxml/tinyxmlerror.cpp
)

target_include_directories(cubicsdr_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external/doctest
    ${PROJECT_SOURCE_DIR}/external/tinyxml
    ${PROJECT_SOURCE_DIR}/src/util
)

target_compile_features(cubicsdr_tests PRIVATE cxx_std_14)
target_link_libraries(cubicsdr_tests PRIVATE Threads::Threads)

IF(WIN32)
    # windows.h min/max macros break std::max; the app inherits this define from wxWidgets.
    target_compile_definitions(cubicsdr_tests PRIVATE NOMINMAX)
ENDIF()

add_test(NAME cubicsdr_tests COMMAND cubicsdr_tests)

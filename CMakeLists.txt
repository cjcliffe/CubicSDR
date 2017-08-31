cmake_minimum_required (VERSION 2.8)

project (CubicSDR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

SET(CUBICSDR_VERSION_MAJOR "0")
SET(CUBICSDR_VERSION_MINOR "2")
SET(CUBICSDR_VERSION_PATCH "2")
SET(CUBICSDR_VERSION_SUFFIX "")
SET(CUBICSDR_VERSION "${CUBICSDR_VERSION_MAJOR}.${CUBICSDR_VERSION_MINOR}.${CUBICSDR_VERSION_PATCH}${CUBICSDR_VERSION_SUFFIX}")

SET(CPACK_PACKAGE_VERSION "${CUBICSDR_VERSION_MAJOR}.${CUBICSDR_VERSION_MINOR}.${CUBICSDR_VERSION_PATCH}")
SET(CPACK_PACKAGE_VERSION_MAJOR ${CUBICSDR_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${CUBICSDR_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${CUBICSDR_VERSION_PATCH})

SET (CUSTOM_BUILD OFF CACHE BOOL "Enable custom build options")
# Build options for custom deploys, optimization and debugging
IF(CUSTOM_BUILD)
    SET (CUBICSDR_BUILD_TITLE CACHE STRING "Custom Title")
    # bundle flags
    SET (CUBICSDR_INSTALL_NAME CACHE "CubicSDR" "Installation Name")
    SET (CUBICSDR_INSTALL_TITLE CACHE "CubicSDR" "Installation Title")
    SET (CUBICSDR_HEADER_IMAGE CACHE "" "Image file to display in header")
    SET (CUBICSDR_HEADER_BG CACHE "000000" "Background Color (HEX) for header")
    # feature flags
    SET (CUBICSDR_ENABLE_VIEW_DEMOD ON CACHE BOOL "Enable Second Demodulator Spectrum/Waterfall view.")
    SET (CUBICSDR_ENABLE_VIEW_SCOPE ON CACHE BOOL "Enable Demodulator Scope/Spectrum view.")
    SET (CUBICSDR_ENABLE_ABOUT_DIALOG ON CACHE BOOL "Enable About Dialog.")
    SET (CUBICSDR_DEFAULT_HIDE_BOOKMARKS OFF CACHE BOOL "Hide Bookmarks by Default.")
    SET (CUBICSDR_MODEM_EXCLUDE CACHE "" "Comma-separated list of modems to exclude.")    

    IF (NOT CUBICSDR_HEADER_IMAGE STREQUAL "")
        SET(CUBICSDR_HAS_HEADER_IMAGE TRUE)
        GET_FILENAME_COMPONENT(CUBICSDR_HEADER_IMAGE_FILE "${CUBICSDR_HEADER_IMAGE}" NAME)
        GET_FILENAME_COMPONENT(CUBICSDR_HEADER_IMAGE_DIR "${CUBICSDR_HEADER_IMAGE}" PATH)
        ADD_DEFINITIONS( 
            -DCUBICSDR_HEADER_IMAGE="${CUBICSDR_HEADER_IMAGE_FILE}"
            -DCUBICSDR_HEADER_BG="${CUBICSDR_HEADER_BG}"
        )
    ENDIF()

    IF (NOT CUBICSDR_MODEM_EXCLUDE STREQUAL "")
        ADD_DEFINITIONS( 
            -DCUBICSDR_MODEM_EXCLUDE="${CUBICSDR_MODEM_EXCLUDE}"
        )
    ENDIF()
ELSE()
    SET (CUBICSDR_BUILD_TITLE "CubicSDR v${CUBICSDR_VERSION} :: www.cubicsdr.com")
    # bundle flags
    SET (CUBICSDR_INSTALL_NAME "CubicSDR")
    SET (CUBICSDR_INSTALL_TITLE "CubicSDR ${CUBICSDR_VERSION} Installer")
    SET (CUBICSDR_HEADER_IMAGE "")
    SET (CUBICSDR_HEADER_BG "")
    # feature flags
    SET (CUBICSDR_ENABLE_VIEW_DEMOD ON)
    SET (CUBICSDR_ENABLE_VIEW_SCOPE ON)
    SET (CUBICSDR_DEFAULT_HIDE_BOOKMARKS OFF)
    SET (CUBICSDR_ENABLE_ABOUT_DIALOG ON)
    SET (CUBICSDR_EXCLUDE_MODEM "")    
ENDIF()

IF(CUBICSDR_ENABLE_VIEW_DEMOD) 
    ADD_DEFINITIONS( -DCUBICSDR_ENABLE_VIEW_DEMOD=1 )
ENDIF()

IF(CUBICSDR_ENABLE_VIEW_SCOPE) 
    ADD_DEFINITIONS( -DCUBICSDR_ENABLE_VIEW_SCOPE=1 )
ENDIF()

IF(CUBICSDR_DEFAULT_HIDE_BOOKMARKS) 
    ADD_DEFINITIONS( -DCUBICSDR_DEFAULT_HIDE_BOOKMARKS=1 )
ENDIF()

IF(CUBICSDR_ENABLE_ABOUT_DIALOG) 
    ADD_DEFINITIONS( -DCUBICSDR_ENABLE_ABOUT_DIALOG=1 )
ENDIF()

ADD_DEFINITIONS(
    -DCUBICSDR_INSTALL_NAME="${CUBICSDR_INSTALL_NAME}"
    -DCUBICSDR_VERSION="${CUBICSDR_VERSION}"
    -DCUBICSDR_BUILD_TITLE="${CUBICSDR_BUILD_TITLE}"
)

SET (ENABLE_DIGITAL_LAB OFF CACHE BOOL "Enable 'Digital Lab' testing features.")
IF(ENABLE_DIGITAL_LAB)
ADD_DEFINITIONS(
    -DENABLE_DIGITAL_LAB=1
)
ENDIF()

set(USE_HAMLIB OFF CACHE BOOL "Support hamlib for radio control functions.")

if (USE_HAMLIB)
    find_package(hamlib REQUIRED)
    
    if (NOT HAMLIB_FOUND)
        message(FATAL_ERROR "hamlib development files not found...")
    endif ()
    
    include_directories(${HAMLIB_INCLUDE_DIR})
    link_libraries(${HAMLIB_LIBRARY})

    ADD_DEFINITIONS(-DUSE_HAMLIB)        
endif ()

macro(configure_files srcDir destDir globStr)
    message(STATUS "Copying ${srcDir}/${globStr} to directory ${destDir}")
    make_directory(${destDir})

    file(GLOB templateFiles RELATIVE ${srcDir} ${srcDir}/${globStr})
    foreach(templateFile ${templateFiles})
        set(srcTemplatePath ${srcDir}/${templateFile})
        message(STATUS "Configuring file ${templateFile}")
        if(NOT IS_DIRECTORY ${srcTemplatePath})
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${templateFile}
                    COPYONLY)
        endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(templateFile)
endmacro(configure_files)

macro(configure_files_recurse srcDir destDir)
    message(STATUS "Configuring directory ${destDir}")
    make_directory(${destDir})

    file(GLOB_RECURSE templateFiles RELATIVE ${srcDir} ${srcDir}/*)
    foreach(templateFile ${templateFiles})
        set(srcTemplatePath ${srcDir}/${templateFile})
        message(STATUS "Configuring file ${templateFile}")
        if(NOT IS_DIRECTORY ${srcTemplatePath})
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${templateFile}
                    COPYONLY)
        endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(templateFile)
endmacro(configure_files_recurse)

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    MESSAGE( "64 bit compiler detected" )
    SET( EX_PLATFORM 64 )
    SET( EX_PLATFORM_NAME "x64" )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 ) 
    MESSAGE( "32 bit compiler detected" )
    SET( EX_PLATFORM 32 )
    SET( EX_PLATFORM_NAME "x86" )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )


SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${EX_PLATFORM_NAME})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/${EX_PLATFORM_NAME})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/${EX_PLATFORM_NAME})

IF (MSVC) 
    include_directories ("${PROJECT_SOURCE_DIR}/external/wglext")
    SET(LIQUID_INCLUDES "${PROJECT_SOURCE_DIR}/external/liquid-dsp/include/" CACHE STRING "Liquid-DSP include directory")
    SET(LIQUID_LIBRARIES "${PROJECT_SOURCE_DIR}/external/liquid-dsp/msvc/${EX_PLATFORM}/libliquid.lib" CACHE STRING "Liquid-DSP Library")
    SET(LIQUID_DLL "${PROJECT_SOURCE_DIR}/external/liquid-dsp/msvc/${EX_PLATFORM}/libliquid.dll" CACHE STRING "Liquid-DSP DLL")
    SET(HAMLIB_DLLS "${PROJECT_SOURCE_DIR}/external/hamlib/${EX_PLATFORM}/libhamlib-2.dll" CACHE STRING "HAMLIB DLLS")
ELSE (MSVC)
    ADD_DEFINITIONS(
        -std=c++0x 
        -pthread
    )
ENDIF(MSVC)

find_package(OpenGL REQUIRED)
find_package(Liquid REQUIRED)
include_directories(${LIQUID_INCLUDES})
SET(OTHER_LIBRARIES ${OTHER_LIBRARIES} ${LIQUID_LIBRARIES})

find_package(wxWidgets COMPONENTS gl core propgrid adv base REQUIRED)
set(wxWidgets_CONFIGURATION mswu)
include(${wxWidgets_USE_FILE})

find_package(SoapySDR "0.4.0" NO_MODULE REQUIRED)
include_directories(${SOAPY_SDR_INCLUDE_DIR})
SET(OTHER_LIBRARIES ${SOAPY_SDR_LIBRARY} ${OTHER_LIBRARIES})
ADD_DEFINITIONS(
    -DUSE_SOAPY_SDR=1
)       

IF (WIN32)
    set(wxWidgets_USE_STATIC ON)

    set(BUILD_INSTALLER OFF CACHE BOOL "Build Installer")
    
    # Audio device selection is not mandatory, dummy audio device is used if none are compiled in.
    # Can also compile support for more than one simultaneously.
    set(USE_AUDIO_DS OFF CACHE BOOL "Include support for DirectSound")
    set(USE_AUDIO_WASAPI ON CACHE BOOL "Include support for WASAPI Audio")
    # TODO:
    # set(USE_AUDIO_ASIO OFF CACHE BOOL "Include support for ASIO Audio")

    # WASAPI
    IF(USE_AUDIO_WASAPI)
        ADD_DEFINITIONS(-D__WINDOWS_WASAPI__)    
        IF (NOT MSVC)    
            SET(OTHER_LIBRARIES ${OTHER_LIBRARIES} -luuid -lksuser)
        ENDIF(NOT MSVC)
    ENDIF(USE_AUDIO_WASAPI)

    # DirectSound
    IF (USE_AUDIO_DS)    
        ADD_DEFINITIONS(-D__WINDOWS_DS__)    
        IF (MSVC)    
            SET(OTHER_LIBRARIES ${OTHER_LIBRARIES} dsound.lib)
        ELSE (MSVC)
            SET(OTHER_LIBRARIES ${OTHER_LIBRARIES} -ldsound)
        ENDIF (MSVC)
    ENDIF(USE_AUDIO_DS)    
  
    SET(USE_MINGW_PATCH OFF CACHE BOOL "Add some missing functions when compiling against mingw liquid-dsp.")
    IF (USE_MINGW_PATCH) 
        SET(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} legacy_stdio_definitions.lib libgcc.a")
        ADD_DEFINITIONS(
            -DMINGW_PATCH=1
        )
        SET (GCC_LINKDIR "" CACHE STRING "")
        IF (GCC_LINKDIR)
            link_directories("${GCC_LINKDIR}")
        ENDIF()
    ENDIF()
ENDIF (WIN32)

MACRO(use_included_rtaudio)
    SET (cubicsdr_sources
        ${cubicsdr_sources}
        external/rtaudio/RtAudio.cpp
    )
    SET (cubicsdr_headers
        ${cubicsdr_headers}
        external/rtaudio/RtAudio.h
    )
    SOURCE_GROUP("_ext-RTAudio" REGULAR_EXPRESSION "external/rtaudio/.*${REG_EXT}")
    include_directories(${PROJECT_SOURCE_DIR}/external/rtaudio)
ENDMACRO(use_included_rtaudio)

IF (UNIX AND NOT APPLE)
    set(BUILD_DEB OFF CACHE BOOL "Build DEB")

    SET(USE_SYSTEM_RTAUDIO OFF CACHE BOOL "Use the system RtAudio which in turn provides OSS, ALSA, JACK, PulseAudio support depending on how it was compiled")

    SET(LIQUID_LIB liquid)
    SET(OTHER_LIBRARIES ${OTHER_LIBRARIES} dl)

    IF(USE_SYSTEM_RTAUDIO)
        find_package(RtAudio)
        SET(OTHER_LIBRARIES ${OTHER_LIBRARIES} ${RTAUDIO_LIBRARIES})
        ADD_DEFINITIONS(${RTAUDIO_DEFINITIONS})
        include_directories(${RTAUDIO_INCLUDE_DIR})
    ELSE(USE_SYSTEM_RTAUDIO)
        use_included_rtaudio()

        SET(USE_AUDIO_PULSE ON CACHE BOOL "Use Pulse Audio")
        SET(USE_AUDIO_JACK OFF CACHE BOOL "Use Jack Audio")
        SET(USE_AUDIO_ALSA OFF CACHE BOOL "Use ALSA Audio")
        SET(USE_AUDIO_OSS OFF CACHE BOOL "Use OSS Audio")
  
        IF(USE_AUDIO_PULSE)
           SET (OTHER_LIBRARIES ${OTHER_LIBRARIES} pulse-simple pulse)
           ADD_DEFINITIONS(
                -D__LINUX_PULSE__
           )
        ENDIF(USE_AUDIO_PULSE)

        IF(USE_AUDIO_JACK)
           find_package(Jack)
           SET (OTHER_LIBRARIES ${OTHER_LIBRARIES} ${JACK_LIBRARIES})
           ADD_DEFINITIONS(
                -D__UNIX_JACK__
           )
           include_directories(${JACK_INCLUDE_DIRS})
        ENDIF(USE_AUDIO_JACK)

        IF(USE_AUDIO_ALSA)
           SET (OTHER_LIBRARIES ${OTHER_LIBRARIES} asound)
           set(ALSA_INCLUDE_DIR "/usr/include" CACHE FILEPATH "ALSA include path")
           include_directories(${ALSA_INCLUDE_DIR})
           set(ALSA_LIB_DIR "/usr/lib" CACHE FILEPATH "ALSA lib path")
           link_directories(${ALSA_LIB_DIR})
           ADD_DEFINITIONS(
               -D__LINUX_ALSA__
           )
        ENDIF(USE_AUDIO_ALSA)

        IF(USE_AUDIO_OSS)
           SET (OTHER_LIBRARIES ${OTHER_LIBRARIES} oss)
           ADD_DEFINITIONS(
                -D__LINUX_OSS__
           )
        ENDIF(USE_AUDIO_OSS)
    ENDIF(USE_SYSTEM_RTAUDIO)
ELSE(UNIX AND NOT APPLE)
    use_included_rtaudio()
ENDIF(UNIX AND NOT APPLE)

IF (APPLE)
    SET(CMAKE_OSX_DEPLOYMENT_TARGET, "10.9")
  
    SET(LIQUID_LIB liquid)
    link_directories(/usr/local/lib)
    link_directories(/opt/local/lib)

    ADD_DEFINITIONS(
      -D__MACOSX_CORE__
    )    

   FIND_LIBRARY(COREAUDIO_LIBRARY CoreAudio)
   SET (OTHER_LIBRARIES ${COREAUDIO_LIBRARY} ${OTHER_LIBRARIES})
   set(BUNDLE_APP OFF CACHE BOOL "Bundle Application")

ENDIF (APPLE)


SET (cubicsdr_sources
    ${cubicsdr_sources}
    src/CubicSDR.cpp
    src/AppFrame.cpp
    src/AppConfig.cpp
    src/FrequencyDialog.cpp
    src/DemodLabelDialog.cpp
    src/IOThread.cpp
    src/ModemProperties.cpp
    src/BookmarkMgr.cpp
    src/sdr/SDRDeviceInfo.cpp
    src/sdr/SDRPostThread.cpp
    src/sdr/SDREnumerator.cpp
    src/sdr/SoapySDRThread.h
    src/demod/DemodulatorPreThread.cpp
    src/demod/DemodulatorThread.cpp
    src/demod/DemodulatorWorkerThread.cpp
    src/demod/DemodulatorInstance.cpp
    src/demod/DemodulatorMgr.cpp
    src/modules/modem/Modem.cpp
    src/modules/modem/ModemAnalog.cpp
    src/modules/modem/ModemDigital.cpp
    src/modules/modem/analog/ModemAM.cpp
    src/modules/modem/analog/ModemDSB.cpp
    src/modules/modem/analog/ModemFM.cpp
    src/modules/modem/analog/ModemNBFM.cpp
    src/modules/modem/analog/ModemFMStereo.cpp
    src/modules/modem/analog/ModemIQ.cpp
    src/modules/modem/analog/ModemLSB.cpp
    src/modules/modem/analog/ModemUSB.cpp
    src/audio/AudioThread.cpp
    src/util/Gradient.cpp
    src/util/Timer.cpp
    src/util/MouseTracker.cpp
    src/util/GLExt.cpp
    src/util/GLFont.cpp
    src/util/DataTree.cpp
    src/panel/ScopePanel.cpp
    src/panel/SpectrumPanel.cpp
    src/panel/WaterfallPanel.cpp
    src/panel/MeterPanel.cpp
    src/panel/MeterPanel.h
    src/visual/ColorTheme.cpp
    src/visual/PrimaryGLContext.cpp
    src/visual/InteractiveCanvas.cpp
    src/visual/MeterCanvas.cpp
    src/visual/MeterContext.cpp
    src/visual/TuningCanvas.cpp
    src/visual/TuningContext.cpp
    src/visual/ModeSelectorCanvas.cpp
    src/visual/ModeSelectorContext.cpp
    src/visual/ScopeCanvas.cpp
    src/visual/ScopeContext.cpp
    src/visual/SpectrumCanvas.cpp
    src/visual/WaterfallCanvas.cpp
    src/visual/GainCanvas.cpp
    src/visual/ImagePanel.cpp
    src/process/VisualProcessor.cpp
    src/process/ScopeVisualProcessor.cpp
    src/process/SpectrumVisualProcessor.cpp
    src/process/FFTVisualDataThread.cpp
    src/process/FFTDataDistributor.cpp
    src/process/SpectrumVisualDataThread.cpp
    src/ui/GLPanel.cpp
    src/forms/SDRDevices/SDRDevices.cpp
    src/forms/SDRDevices/SDRDevicesForm.cpp
    src/forms/SDRDevices/SDRDeviceAdd.cpp
    src/forms/SDRDevices/SDRDeviceAddForm.cpp
    src/forms/Bookmark/BookmarkPanel.cpp
    src/forms/Bookmark/BookmarkView.cpp
    src/forms/Dialog/ActionDialogBase.cpp
    src/forms/Dialog/ActionDialog.cpp
    src/forms/Dialog/AboutDialogBase.cpp
    src/forms/Dialog/AboutDialog.cpp
    external/lodepng/lodepng.cpp
    external/tinyxml/tinyxml.cpp
    external/tinyxml/tinystr.cpp
    external/tinyxml/tinyxmlparser.cpp
    external/tinyxml/tinyxmlerror.cpp
    external/cubicvr2/math/cubic_math.cpp
)

IF(ENABLE_DIGITAL_LAB)
    SET (cubicsdr_sources
        ${cubicsdr_sources}
        src/forms/DigitalConsole/DigitalConsole.cpp
        src/forms/DigitalConsole/DigitalConsoleFrame.cpp
        src/modules/modem/digital/ModemASK.cpp
        src/modules/modem/digital/ModemAPSK.cpp
        src/modules/modem/digital/ModemBPSK.cpp
        src/modules/modem/digital/ModemDPSK.cpp
        src/modules/modem/digital/ModemGMSK.cpp
        src/modules/modem/digital/ModemPSK.cpp
        src/modules/modem/digital/ModemOOK.cpp
        src/modules/modem/digital/ModemST.cpp
        src/modules/modem/digital/ModemSQAM.cpp
        src/modules/modem/digital/ModemQAM.cpp
        src/modules/modem/digital/ModemQPSK.cpp
        src/modules/modem/digital/ModemFSK.cpp
    )
ENDIF()

SET (cubicsdr_headers
    ${cubicsdr_headers}
    src/CubicSDRDefs.h
    src/CubicSDR.h
    src/AppFrame.h
    src/AppConfig.h
    src/FrequencyDialog.h
    src/DemodLabelDialog.h
    src/IOThread.h
    src/ModemProperties.h
    src/BookmarkMgr.h
    src/sdr/SDRDeviceInfo.h
    src/sdr/SDRPostThread.h
    src/sdr/SDREnumerator.h
    src/sdr/SoapySDRThread.cpp
    src/demod/DemodulatorPreThread.h
    src/demod/DemodulatorThread.h
    src/demod/DemodulatorWorkerThread.h
    src/demod/DemodulatorInstance.h
    src/demod/DemodulatorMgr.h
    src/demod/DemodDefs.h
    src/modules/modem/Modem.h
    src/modules/modem/ModemAnalog.h
    src/modules/modem/ModemDigital.h
    src/modules/modem/analog/ModemAM.h
    src/modules/modem/analog/ModemDSB.h
    src/modules/modem/analog/ModemFM.h
    src/modules/modem/analog/ModemNBFM.h
    src/modules/modem/analog/ModemFMStereo.h
    src/modules/modem/analog/ModemIQ.h
    src/modules/modem/analog/ModemLSB.h
    src/modules/modem/analog/ModemUSB.h
    src/audio/AudioThread.h
    src/util/Gradient.h
    src/util/Timer.h
	src/util/ThreadBlockingQueue.h
    src/util/MouseTracker.h
    src/util/GLExt.h
    src/util/GLFont.h
    src/util/DataTree.h
    src/panel/ScopePanel.h
    src/panel/SpectrumPanel.h
    src/panel/WaterfallPanel.h
    src/visual/ColorTheme.h
    src/visual/PrimaryGLContext.h
    src/visual/InteractiveCanvas.h
    src/visual/MeterCanvas.h
    src/visual/MeterContext.h
    src/visual/TuningCanvas.h
    src/visual/TuningContext.h
    src/visual/ModeSelectorCanvas.h
    src/visual/ModeSelectorContext.h
    src/visual/ScopeCanvas.h
    src/visual/ScopeContext.h
    src/visual/SpectrumCanvas.h
    src/visual/WaterfallCanvas.h
    src/visual/GainCanvas.h
    src/visual/ImagePanel.h
    src/process/VisualProcessor.h
    src/process/ScopeVisualProcessor.h
    src/process/SpectrumVisualProcessor.h
    src/process/FFTVisualDataThread.h
    src/process/FFTDataDistributor.h
    src/process/SpectrumVisualDataThread.h
    src/ui/GLPanel.h
    src/ui/UITestCanvas.cpp
    src/ui/UITestCanvas.h
    src/ui/UITestContext.cpp
    src/ui/UITestContext.h    
    src/forms/SDRDevices/SDRDevices.h
    src/forms/SDRDevices/SDRDevicesForm.h
    src/forms/SDRDevices/SDRDeviceAdd.h
    src/forms/SDRDevices/SDRDeviceAddForm.h
    src/forms/Bookmark/BookmarkPanel.h
    src/forms/Bookmark/BookmarkView.h
    src/forms/Dialog/ActionDialogBase.h
    src/forms/Dialog/ActionDialog.h
    src/forms/Dialog/AboutDialogBase.h
    src/forms/Dialog/AboutDialog.h
    external/lodepng/lodepng.h
    external/tinyxml/tinyxml.h
    external/tinyxml/tinystr.h
    external/cubicvr2/math/aabb.h
    external/cubicvr2/math/cubic_math.h
    external/cubicvr2/math/cubic_types.h
    external/cubicvr2/math/frustum.h
    external/cubicvr2/math/mat3.h
    external/cubicvr2/math/mat4.h
    external/cubicvr2/math/plane.h
    external/cubicvr2/math/quaternion.h
    external/cubicvr2/math/sphere.h
    external/cubicvr2/math/transform.h
    external/cubicvr2/math/triangle.h
    external/cubicvr2/math/vec2.h
    external/cubicvr2/math/vec3.h
    external/cubicvr2/math/vec4.h
)

IF(ENABLE_DIGITAL_LAB)
SET (cubicsdr_headers
    ${cubicsdr_headers}
    src/forms/DigitalConsole/DigitalConsole.h
    src/forms/DigitalConsole/DigitalConsoleFrame.h
    src/modules/modem/digital/ModemASK.h
    src/modules/modem/digital/ModemAPSK.h
    src/modules/modem/digital/ModemBPSK.h
    src/modules/modem/digital/ModemDPSK.h
    src/modules/modem/digital/ModemGMSK.h
    src/modules/modem/digital/ModemPSK.h
    src/modules/modem/digital/ModemOOK.h
    src/modules/modem/digital/ModemST.h
    src/modules/modem/digital/ModemSQAM.h
    src/modules/modem/digital/ModemQAM.h
    src/modules/modem/digital/ModemQPSK.h
    src/modules/modem/digital/ModemFSK.h
)    
ENDIF()


IF (USE_HAMLIB)
    SET (cubicsdr_headers
        ${cubicsdr_headers}
        src/forms/Dialog/PortSelectorDialogBase.h
        src/forms/Dialog/PortSelectorDialog.h
        src/rig/RigThread.h
        external/rs232/rs232.h
    )    
    SET (cubicsdr_sources
        ${cubicsdr_sources}
	src/forms/Dialog/PortSelectorDialogBase.cpp
        src/forms/Dialog/PortSelectorDialog.cpp
        src/rig/RigThread.cpp
    )
    
    IF(WIN32)
        SET (cubicsdr_sources
            ${cubicsdr_sources}
            external/rs232/rs232-win.cpp
        )
    ELSE()
        SET (cubicsdr_sources
            ${cubicsdr_sources}
            external/rs232/rs232-linux.cpp
        )
    ENDIF()    
    
    include_directories ( 
        external/rs232/
    )
ENDIF()


SET (CUBICSDR_FONTS 
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono12.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono16.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono18.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono24.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono27.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono32.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono36.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono48.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono64.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono72.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono96.fnt
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono12_0.png
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono16_0.png
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono18_0.png
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono24_0.png
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono27_0.png
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono32_0.png
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono36_0.png
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono48_0.png
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono64_0.png
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono72_0.png
    ${PROJECT_SOURCE_DIR}/font/vera_sans_mono96_0.png
)

set(REG_EXT "[^/]*([.]cpp|[.]c|[.]h|[.]hpp)$")

SOURCE_GROUP("Base" REGULAR_EXPRESSION "src/${REG_EXT}")
SOURCE_GROUP("Forms\\SDRDevices" REGULAR_EXPRESSION "src/forms/SDRDevices/${REG_EXT}")
SOURCE_GROUP("Forms\\Bookmark" REGULAR_EXPRESSION "src/forms/Bookmark/${REG_EXT}")
SOURCE_GROUP("Forms\\Dialog" REGULAR_EXPRESSION "src/forms/Dialog/${REG_EXT}")
SOURCE_GROUP("SDR" REGULAR_EXPRESSION "src/sdr/${REG_EXT}")
IF(USE_HAMLIB)
    SOURCE_GROUP("Rig" REGULAR_EXPRESSION "src/rig/${REG_EXT}")    
    SOURCE_GROUP("_ext-RS-232" REGULAR_EXPRESSION "external/rs232/${REG_EXT}")    
ENDIF()
SOURCE_GROUP("Demodulator" REGULAR_EXPRESSION "src/demod/${REG_EXT}")
SOURCE_GROUP("Modem" REGULAR_EXPRESSION "src/modules/modem/${REG_EXT}")
SOURCE_GROUP("Modem\\Analog" REGULAR_EXPRESSION "src/modules/modem/analog/${REG_EXT}")
IF(ENABLE_DIGITAL_LAB)
SOURCE_GROUP("Modem\\Digital" REGULAR_EXPRESSION "src/modules/modem/digital/${REG_EXT}")
SOURCE_GROUP("Forms\\DigitalConsole" REGULAR_EXPRESSION "src/forms/DigitalConsole/${REG_EXT}")
ENDIF()
SOURCE_GROUP("Audio" REGULAR_EXPRESSION "src/audio/${REG_EXT}")
SOURCE_GROUP("Utility" REGULAR_EXPRESSION "src/util/${REG_EXT}")
SOURCE_GROUP("Visual" REGULAR_EXPRESSION "src/visual/${REG_EXT}")
SOURCE_GROUP("Panel" REGULAR_EXPRESSION "src/panel/${REG_EXT}")
SOURCE_GROUP("Process" REGULAR_EXPRESSION "src/process/${REG_EXT}")
SOURCE_GROUP("UI" REGULAR_EXPRESSION "src/ui/${REG_EXT}")
SOURCE_GROUP("_ext-LodePNG" REGULAR_EXPRESSION "external/lodepng/.*${REG_EXT}")
SOURCE_GROUP("_ext-TinyXML" REGULAR_EXPRESSION "external/tinyxml/.*${REG_EXT}")
SOURCE_GROUP("_ext-CubicVR2" REGULAR_EXPRESSION "external/cubicvr2/.*${REG_EXT}")

include_directories ( 
    ${PROJECT_SOURCE_DIR}/src/forms/SDRDevices
    ${PROJECT_SOURCE_DIR}/src/forms/DigitalConsole
    ${PROJECT_SOURCE_DIR}/src/forms/Bookmark
    ${PROJECT_SOURCE_DIR}/src/forms/Dialog
    ${PROJECT_SOURCE_DIR}/src/sdr 
    ${PROJECT_SOURCE_DIR}/src/demod
    ${PROJECT_SOURCE_DIR}/src/modules
    ${PROJECT_SOURCE_DIR}/src/modules/modem
    ${PROJECT_SOURCE_DIR}/src/modules/modem/digital
    ${PROJECT_SOURCE_DIR}/src/modules/modem/analog
    ${PROJECT_SOURCE_DIR}/src/audio
    ${PROJECT_SOURCE_DIR}/src/util
    ${PROJECT_SOURCE_DIR}/src/panel
    ${PROJECT_SOURCE_DIR}/src/visual
    ${PROJECT_SOURCE_DIR}/src/process
    ${PROJECT_SOURCE_DIR}/src/ui
    ${PROJECT_SOURCE_DIR}/src/rig
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external/lodepng 
    ${PROJECT_SOURCE_DIR}/external/tinyxml
    ${PROJECT_SOURCE_DIR}/external/cubicvr2/math
)

set(RES_FILES "")
if(MINGW OR MSVC)
 set(RES_FILES ${PROJECT_SOURCE_DIR}/cubicsdr.rc)
 set(CMAKE_RC_COMPILER_INIT windres)
 ENABLE_LANGUAGE(RC)
 IF(EX_PLATFORM EQUAL 64)
    SET(RC_TARGET "pe-x86-64")
 ELSE(EX_PLATFORM EQUAL 64)
    SET(RC_TARGET "pe-i386")
 ENDIF(EX_PLATFORM EQUAL 64)

 SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif(MINGW OR MSVC)

IF (NOT BUNDLE_APP)
    configure_files(${PROJECT_SOURCE_DIR}/font ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME}/fonts "*.fnt")
    configure_files(${PROJECT_SOURCE_DIR}/font ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME}/fonts "*.png")
    configure_files(${PROJECT_SOURCE_DIR}/icon ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} CubicSDR.ico)
    IF(MSVC)
        configure_files(${PROJECT_SOURCE_DIR}/external/liquid-dsp/msvc/${EX_PLATFORM}/ ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")
    ENDIF()
    IF (CUBICSDR_HAS_HEADER_IMAGE)
        configure_files(${CUBICSDR_HEADER_IMAGE_DIR} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} ${CUBICSDR_HEADER_IMAGE_FILE})
    ENDIF()
    add_executable(CubicSDR ${cubicsdr_sources} ${cubicsdr_headers} ${RES_FILES})
    target_link_libraries(CubicSDR ${LIQUID_LIB} ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES} ${OTHER_LIBRARIES})
ENDIF (NOT BUNDLE_APP)

IF (MSVC)
  set_target_properties(CubicSDR PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
  set_target_properties(CubicSDR PROPERTIES COMPILE_DEFINITIONS_DEBUG "_WINDOWS")
  set_target_properties(CubicSDR PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
  set_target_properties(CubicSDR PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_WINDOWS")
  set_target_properties(CubicSDR PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
  set_target_properties(CubicSDR PROPERTIES COMPILE_DEFINITIONS_RELEASE "_WINDOWS")
  set_target_properties(CubicSDR PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
  set_target_properties(CubicSDR PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "_WINDOWS")
  set(CMAKE_CREATE_WIN32_EXE "/SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
  set_target_properties (CubicSDR PROPERTIES OUTPUT_NAME "${CUBICSDR_INSTALL_NAME}")
  
  ADD_DEFINITIONS(
        /wd\"4996\"
  )   
ENDIF(MSVC)

IF (APPLE)
    ADD_DEFINITIONS(
        -DHAVE_TYPE_TRAITS=1
        -mmacosx-version-min=10.9
    )
ENDIF(APPLE)

IF (APPLE AND BUNDLE_APP)
    PROJECT(CubicSDR)
    set(CPACK_BINARY_DRAGNDROP ON)
 
    SET(MACOSX_BUNDLE_BUNDLE_NAME CubicSDR)
    SET(CPACK_APPLE_BUNDLE_ID "com.cubicproductions.cubicsdr")    
    set(CUBICSDR_CODE_SIGN OFF CACHE BOOL "Code Signing")
    
    set(BUNDLE_SOAPY_MODS OFF CACHE BOOL "Bundle local SoapySDR modules")
    set(BUNDLE_MIR_SDR OFF CACHE BOOL "Bundle mir_sdr for personal use only -- do not distribute.")
    
    IF (BUNDLE_SOAPY_MODS)
        ADD_DEFINITIONS(
            -DBUNDLE_SOAPY_MODS=1
        )
        set(BUNDLED_MODS_ONLY OFF CACHE BOOL "Use bundled mods only")
        IF (BUNDLED_MODS_ONLY)
        ADD_DEFINITIONS(
            -DBUNDLED_MODS_ONLY=1
        )
        ENDIF()
    ENDIF()
    
    ADD_DEFINITIONS(
        -std=c++0x 
        -pthread
        -D_OSX_APP_
    )   
      
    ADD_EXECUTABLE(CubicSDR 
        MACOSX_BUNDLE 
        ${cubicsdr_sources}
        ${cubicsdr_headers} 
        ${CUBICSDR_FONTS}
        ${PROJECT_SOURCE_DIR}/icon/CubicSDR.icns
    ) 
    
    SET_SOURCE_FILES_PROPERTIES(
        ${PROJECT_SOURCE_DIR}/icon/CubicSDR.icns
        PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )

    SET_SOURCE_FILES_PROPERTIES(
        ${CUBICSDR_FONTS}
        PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources/fonts
    )

    target_link_libraries(CubicSDR ${LIQUID_LIB} ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES} ${OTHER_LIBRARIES})
    SET_TARGET_PROPERTIES(CubicSDR PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_STRING "CubicSDR Open-Source Software-Defined Radio Application"
        MACOSX_BUNDLE_BUNDLE_NAME "CubicSDR"
        MACOSX_BUNDLE_BUNDLE_VERSION "${CUBICSDR_VERSION}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${CUBICSDR_VERSION_MAJOR}.${CUBICSDR_VERSION_MINOR}.${CUBICSDR_VERSION_PATCH}${CUBICSDR_VERSION_SUFFIX}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${CUBICSDR_VERSION_MAJOR}.${CUBICSDR_VERSION_MINOR}.${CUBICSDR_VERSION_PATCH}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.cubicproductions.cubicsdr"
        MACOSX_BUNDLE_ICON_FILE "CubicSDR.icns"
        MACOSX_BUNDLE_COPYRIGHT "Copyright 2015 Charles J. Cliffe. All Rights Reserved."
    )
    
    SET(APPS "${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME}/CubicSDR.app")
    # SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
    # SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
        
    IF (BUNDLE_SOAPY_MODS)
    message(STATUS "SOAPY_ROOT: ${SOAPY_SDR_ROOT}")
    SET(SOAPY_SDR_MOD_PATH "${SOAPY_SDR_ROOT}/lib/SoapySDR/modules${SOAPY_SDR_ABI_VERSION}")
    message(STATUS "SOAPY_SDR_MOD_PATH: ${SOAPY_SDR_MOD_PATH}")
    
    file(GLOB SOAPY_MODS ${SOAPY_SDR_MOD_PATH}/*.so)
    
    FOREACH(SOAPY_MOD_FILE ${SOAPY_MODS})
            INSTALL( FILES "${SOAPY_MOD_FILE}"
                DESTINATION "${APPS}/Contents/MacOS/modules"
                COMPONENT Runtime
            )
    ENDFOREACH()
        
    ENDIF(BUNDLE_SOAPY_MODS)

    SET_TARGET_PROPERTIES(CubicSDR PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/cmake/CubicSDRInfo.plist.in")

    INSTALL(CODE "
       SET(BU_COPY_FULL_FRAMEWORK_CONTENTS ON)
       include(BundleUtilities)
       fixup_bundle(\"${APPS}\"   \"\"   \"/usr/local/lib\")
       " COMPONENT Runtime)

    IF (BUNDLE_SOAPY_MODS)
       FOREACH(SOAPY_MOD_FILE ${SOAPY_MODS})
           GET_FILENAME_COMPONENT(SOAPY_MOD_NAME ${SOAPY_MOD_FILE} NAME)
           IF(NOT BUNDLE_MIR_SDR) # prevent inclusion of libmirsdrapi-rsp.so
           IF(${SOAPY_MOD_NAME} STREQUAL "libsdrPlaySupport.so")
               message(STATUS "Excluding libsdrPlaySupport.so")
               CONTINUE()
           ELSE()
               message(STATUS "Bundling ${SOAPY_MOD_NAME} from ${SOAPY_MOD_FILE}")
           ENDIF()
           ENDIF()
           INSTALL(CODE "
               fixup_bundle(\"${APPS}\"   \"${APPS}/Contents/MacOS/modules/${SOAPY_MOD_NAME}\"   \"/usr/local/lib\")
              " COMPONENT Runtime)
       ENDFOREACH()
   ENDIF(BUNDLE_SOAPY_MODS)
   
   IF (CUBICSDR_CODE_SIGN)
       SET (CUBICSDR_CERT "3rd Party Mac Developer Application: [Name]" CACHE STRING "Code signing certificate name.")
       MESSAGE(STATUS "Code Signing Enabled: ${CUBICSDR_CERT}")        
  
       CONFIGURE_FILE(
         "${PROJECT_SOURCE_DIR}/cmake/code_sign.sh.in"
         "${CMAKE_CURRENT_BINARY_DIR}/code_sign.sh"
        )
        CONFIGURE_FILE(
          "${PROJECT_SOURCE_DIR}/cmake/dmg_sign.sh.in"
          "${CMAKE_CURRENT_BINARY_DIR}/dmg_sign.sh"
         )
  
        ADD_CUSTOM_COMMAND(
            TARGET CubicSDR
            COMMAND chmod ARGS +x ${CMAKE_CURRENT_BINARY_DIR}/code_sign.sh ${CMAKE_CURRENT_BINARY_DIR}/dmg_sign.sh
        )

        INSTALL(CODE "
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E echo \"Signing code..\"
                COMMAND \"${CMAKE_CURRENT_BINARY_DIR}/code_sign.sh\"
            )"
        COMPONENT Runtime)
  
        ADD_CUSTOM_COMMAND(
            TARGET CubicSDR
            POST_BUILD
            COMMAND ${CMAKE_CPACK_COMMAND}
            COMMAND ${CMAKE_COMMAND} -E echo "Signing package.."
            COMMAND "${CMAKE_CURRENT_BINARY_DIR}/dmg_sign.sh" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
   ENDIF(CUBICSDR_CODE_SIGN)
   
   
   INSTALL(CODE "
       VERIFY_APP(\"${APPS}\")
       " COMPONENT Runtime)

   INSTALL(TARGETS CubicSDR
       BUNDLE DESTINATION . COMPONENT Runtime
   )       

   include(CPack)
ENDIF()

IF(APPLE AND NOT BUNDLE_APP)
    IF (NOT CMAKE_INSTALL_PREFIX)
        SET(CMAKE_INSTALL_PREFIX "/usr/")
    ENDIF()
    ADD_DEFINITIONS(
        -DRES_FOLDER="${CMAKE_INSTALL_PREFIX}/share/cubicsdr/"
    )    

    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,relro")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro")

    INSTALL(TARGETS CubicSDR DESTINATION bin)
        install(FILES 
            ${PROJECT_SOURCE_DIR}/src/CubicSDR.png
        DESTINATION share/cubicsdr)

        install(FILES 
            ${CUBICSDR_FONTS}
        DESTINATION share/cubicsdr/fonts)

    CONFIGURE_FILE(
      "${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
       @ONLY IMMEDIATE)
    ADD_CUSTOM_TARGET(uninstall
      "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake") 
          
ENDIF ()


IF (WIN32 AND NOT BUILD_INSTALLER)
    ADD_DEFINITIONS(
        -DRES_FOLDER="../share/cubicsdr/"
    )
    
    INSTALL(TARGETS CubicSDR DESTINATION bin)
    INSTALL(FILES         
        ${LIQUID_DLL}
    DESTINATION bin)

    IF(USE_HAMLIB)
        FOREACH(HAMLIB_DLL ${HAMLIB_DLLS})
            message(STATUS "Copying Hamlib DLL: ${HAMLIB_DLL}")
            INSTALL( FILES "${HAMLIB_DLL}"
                DESTINATION bin
            )
        ENDFOREACH()
    ENDIF()

    INSTALL(FILES 
        ${PROJECT_SOURCE_DIR}/src/CubicSDR.png
     DESTINATION share/cubicsdr)

    INSTALL(FILES 
        ${CUBICSDR_FONTS}
     DESTINATION share/cubicsdr/fonts)

    IF (CUBICSDR_HAS_HEADER_IMAGE)
        INSTALL(FILES 
        ${CUBICSDR_HEADER_IMAGE}
     DESTINATION share/cubicsdr/)
    ENDIF()

ENDIF()

IF (WIN32 AND BUILD_INSTALLER)
    set(BUNDLE_SOAPY_MODS OFF CACHE BOOL "Bundle local SoapySDR modules")

    set(CPACK_GENERATOR NSIS)
    set(CPACK_PACKAGE_NAME "${CUBICSDR_INSTALL_NAME}")
    set(CPACK_NSIS_DISPLAY_NAME "${CUBICSDR_INSTALL_TITLE}")
    set(CPACK_PACKAGE_VENDOR "cubicsdr.com")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CUBICSDR_INSTALL_NAME}")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "CubicSDR.ico")
    SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE") 
    set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/icon\\\\NSIS_Header.bmp")
    IF(EX_PLATFORM EQUAL 64)
        SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
        SET(CPACK_NSIS_PACKAGE_NAME "${CUBICSDR_INSTALL_NAME}")
        SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CUBICSDR_INSTALL_NAME} ${CPACK_PACKAGE_VERSION}")
        set(CMAKE_CL_64 TRUE)    # This gets around a bug in the CPack installer name generation for MinGW 64-bit since 2.8
    ELSE(EX_PLATFORM EQUAL 64)
        SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
        SET(CPACK_NSIS_PACKAGE_NAME "${CUBICSDR_INSTALL_NAME} (x86)")
        SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CUBICSDR_INSTALL_NAME} ${CPACK_PACKAGE_VERSION} (x86)")
        set(CMAKE_CL_64 FALSE)
    ENDIF(EX_PLATFORM EQUAL 64)
    
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".") 
    install(TARGETS CubicSDR RUNTIME DESTINATION .)

    install(FILES 
        ${PROJECT_SOURCE_DIR}/icon/CubicSDR.ico
        ${LIQUID_DLL}
        DESTINATION .)

    install(FILES 
        ${CUBICSDR_FONTS}
        DESTINATION fonts)

    IF (CUBICSDR_HAS_HEADER_IMAGE)
        INSTALL(FILES 
        ${CUBICSDR_HEADER_IMAGE}
        DESTINATION .)
    ENDIF()
        
    IF(USE_HAMLIB)
        FOREACH(HAMLIB_DLL ${HAMLIB_DLLS})
            message(STATUS "Copying Hamlib DLL: ${HAMLIB_DLL}")
            INSTALL( FILES 
                ${HAMLIB_DLL}
            DESTINATION .)
        ENDFOREACH()
    ENDIF()

    IF (BUNDLE_SOAPY_MODS)
        ADD_DEFINITIONS(
            -DBUNDLE_SOAPY_MODS=1
        )
        set(BUNDLED_MODS_ONLY OFF CACHE BOOL "Use bundled mods only")
        IF (BUNDLED_MODS_ONLY)
        ADD_DEFINITIONS(
            -DBUNDLED_MODS_ONLY=1
        )
        ENDIF()
        
        file(GLOB SOAPY_BINS ${SOAPY_SDR_ROOT}/bin/*.dll)
        file(GLOB SOAPY_MODS ${SOAPY_SDR_ROOT}/lib/SoapySDR/modules${SOAPY_SDR_ABI_VERSION}/*.dll)
        message(STATUS "SOAPY_BINS: ${SOAPY_BINS}")
        message(STATUS "SOAPY_MODS: ${SOAPY_MODS}")
        install(FILES ${SOAPY_BINS} DESTINATION .)
        install(FILES ${SOAPY_MODS} DESTINATION modules)
    ENDIF(BUNDLE_SOAPY_MODS)
    
    IF(MSVC AND EX_PLATFORM EQUAL 32)
        file(GLOB MSVC32_DEPS ${PROJECT_SOURCE_DIR}/external/msvc/${EX_PLATFORM_NAME}/*.dll)
        install(FILES ${MSVC32_DEPS} DESTINATION .)
    ENDIF(MSVC AND EX_PLATFORM EQUAL 32)

        set(CPACK_PACKAGE_EXECUTABLES CubicSDR "CubicSDR")

    IF (MSVC)
        install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/external/msvc/${EX_PLATFORM_NAME}/vc_redist.${EX_PLATFORM_NAME}.exe DESTINATION vc_redist)
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\vc_redist\\\\vc_redist.${EX_PLATFORM_NAME}.exe\\\" /q:a'")
    ENDIF (MSVC)
          
          
    INCLUDE(CPack)    
ENDIF (WIN32 AND BUILD_INSTALLER)


IF (UNIX AND NOT APPLE AND BUILD_DEB) 
    set(CPACK_GENERATOR DEB)
    set(CPACK_PACKAGE_NAME "CubicSDR")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS " libwxgtk3.0-0, libpulse0")
     SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Charles J. Cliffe <cj@cubicproductions.com>")
    SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "CubicSDR Software Defined Radio application v${CUBICSDR_VERSION}")
    SET(CPACK_DEBIAN_PACKAGE_SECTION "comm")
        SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${EX_PLATFORM_NAME}")

    IF (NOT CMAKE_INSTALL_PREFIX)
        SET(CMAKE_INSTALL_PREFIX "/usr/")
    ENDIF()
    ADD_DEFINITIONS(
        -DRES_FOLDER="${CMAKE_INSTALL_PREFIX}/share/cubicsdr/"
        -D_FORTIFY_SOURCE=2
    )    
        
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,relro")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro")

    CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/external/deb/deb_post.sh.in" 
            "${CMAKE_CURRENT_BINARY_DIR}/deb_post.sh" @ONLY IMMEDIATE)

    CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CubicSDR.desktop.in"
            "${CMAKE_CURRENT_BINARY_DIR}/CubicSDR.desktop" @ONLY IMMEDIATE)

    INSTALL(TARGETS CubicSDR DESTINATION bin)
        install(FILES 
            ${PROJECT_SOURCE_DIR}/src/CubicSDR.png
        DESTINATION share/cubicsdr)

        install(FILES 
            ${CUBICSDR_FONTS}
        DESTINATION share/cubicsdr/fonts)
        
    INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/CubicSDR.desktop" 
        DESTINATION share/applications)
        
    INCLUDE(CPack)
ENDIF()
IF(UNIX AND NOT APPLE AND NOT BUILD_DEB)
    IF (NOT CMAKE_INSTALL_PREFIX)
        SET(CMAKE_INSTALL_PREFIX "/usr/")
    ENDIF()
    ADD_DEFINITIONS(
        -DRES_FOLDER="${CMAKE_INSTALL_PREFIX}/share/cubicsdr/"
    )    

    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,relro")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro")
 
    CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CubicSDR.desktop.in"
            "${CMAKE_CURRENT_BINARY_DIR}/CubicSDR.desktop" @ONLY IMMEDIATE)

    INSTALL(TARGETS CubicSDR DESTINATION bin)
 
    INSTALL(FILES 
            ${PROJECT_SOURCE_DIR}/src/CubicSDR.png
            DESTINATION share/cubicsdr)

    INSTALL(FILES 
            ${CUBICSDR_FONTS}
            DESTINATION share/cubicsdr/fonts)


    IF (CUBICSDR_HAS_HEADER_IMAGE)
        INSTALL(FILES 
            ${CUBICSDR_HEADER_IMAGE}
            DESTINATION share/cubicsdr)
    ENDIF()

    INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/CubicSDR.desktop"
            DESTINATION share/applications)
        
    CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/external/deb/deb_post.sh.in" 
            "${CMAKE_CURRENT_BINARY_DIR}/deb_post.sh" @ONLY IMMEDIATE)
    CONFIGURE_FILE(
      "${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
       @ONLY IMMEDIATE)
    ADD_CUSTOM_TARGET(uninstall
      "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake") 

ENDIF()

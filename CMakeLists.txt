cmake_minimum_required (VERSION 2.8)

SET(CUBICSDR_VERSION_MAJOR "0")
SET(CUBICSDR_VERSION_MINOR "1")
SET(CUBICSDR_VERSION_PATCH "8")
SET(CUBICSDR_VERSION_REL "beta-issue64")
SET(CUBICSDR_VERSION "${CUBICSDR_VERSION_MAJOR}.${CUBICSDR_VERSION_MINOR}.${CUBICSDR_VERSION_PATCH}-${CUBICSDR_VERSION_REL}")

SET(CPACK_PACKAGE_VERSION "${CUBICSDR_VERSION_MAJOR}.${CUBICSDR_VERSION_MINOR}.${CUBICSDR_VERSION_PATCH}")
SET(CPACK_PACKAGE_VERSION_MAJOR ${CUBICSDR_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${CUBICSDR_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${CUBICSDR_VERSION_PATCH})

ADD_DEFINITIONS(
    -DCUBICSDR_VERSION="${CUBICSDR_VERSION}"
)

# IF (NOT APPLE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
# ENDIF(NOT APPLE)


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

project (CubicSDR)

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    MESSAGE( "64 bit compiler detected" )
    SET( EX_PLATFORM 64 )
    SET( EX_PLATFORM_NAME "x64" )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 ) 
    MESSAGE( "32 bit compiler detected" )
    SET( EX_PLATFORM 32 )
    SET( EX_PLATFORM_NAME "x86" )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

if ( WIN32 AND EX_PLATFORM EQUAL 64)
	set(BUILD_X86 OFF CACHE BOOL "Copy & Build 32-bit files even if 64-bit compiler is detected.")
	if (BUILD_X86)
	    SET( EX_PLATFORM 32 )
		SET( EX_PLATFORM_NAME "x86" )
	endif (BUILD_X86)
endif ( WIN32 AND EX_PLATFORM EQUAL 64)

SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${EX_PLATFORM_NAME})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/${EX_PLATFORM_NAME})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/${EX_PLATFORM_NAME})

find_package(OpenGL REQUIRED)
IF (NOT WIN32)
find_package(FFTW REQUIRED)
find_package(Liquid REQUIRED)
include_directories(${LIQUID_INCLUDES} ${FFTW_INCLUDES})
SET(OTHER_LIBRARIES ${OTHER_LIBRARIES} ${LIQUID_LIBRARIES} ${FFTW_LIBRARIES})
ENDIF()

find_package(wxWidgets COMPONENTS gl core base REQUIRED)
set(wxWidgets_CONFIGURATION mswu)
include(${wxWidgets_USE_FILE})

SET(USE_SOAPY_SDR ON CACHE BOOL "Build with SoapySDR support instead of RTL-SDR.")
IF (USE_SOAPY_SDR) 
    find_package(SoapySDR NO_MODULE REQUIRED)
    include_directories(${SOAPY_SDR_INCLUDE_DIR})
    SET(OTHER_LIBRARIES ${SOAPY_SDR_LIBRARY} ${OTHER_LIBRARIES})
    ADD_DEFINITIONS(
		-DUSE_SOAPY_SDR=1
    )       
ELSE()
    SET(USE_RTL_SDR ON CACHE BOOL "Build with RTL-SDR support only.")
    ADD_DEFINITIONS(
		-DUSE_RTL_SDR=1
    )       
ENDIF()

IF (WIN32)
	set(wxWidgets_USE_STATIC ON)

	set(BUILD_INSTALLER OFF CACHE BOOL "Build Installer")
	
	# Audio device selection is not mandatory, dummy audio device is used if none are compiled in.
	# Can also compile support for more than one simultaneously.
	set(USE_AUDIO_DS ON CACHE BOOL "Include support for DirectSound")
	set(USE_AUDIO_WASAPI OFF CACHE BOOL "Include support for WASAPI Audio")
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
  
  # ASIO?
  #IF(USE_AUDIO_ASIO)
  #ENDIF(USE_AUDIO_ASIO)
  
	# FFTW
	include_directories ( ${PROJECT_SOURCE_DIR}/external/fftw-3.3.4 )
	set(FFTW_LIB libfftw3f-3)

	link_directories ( ${PROJECT_SOURCE_DIR}/external/fftw-3.3.4/${EX_PLATFORM} )
	configure_files(${PROJECT_SOURCE_DIR}/external/fftw-3.3.4/${EX_PLATFORM} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")

    IF(USE_RTL_SDR)
        include_directories( ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release )
    ENDIF()

    include_directories ( ${PROJECT_SOURCE_DIR}/external/liquid-dsp/include )
	set(LIQUID_LIB libliquid)
	
	# Haven't looked into why these are different, just explicitly including everything for now until it can be sorted neatly.
	IF (MSVC)
	  # LIQUID
	  link_directories ( ${PROJECT_SOURCE_DIR}/external/liquid-dsp/msvc/${EX_PLATFORM} )
	  configure_files(${PROJECT_SOURCE_DIR}/external/liquid-dsp/msvc/${EX_PLATFORM} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME}/ "*.dll")
	  # RTL-SDR
      IF(USE_RTL_SDR)
          link_directories ( ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/msvc/${EX_PLATFORM} )
          configure_files(${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/msvc/${EX_PLATFORM} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")
      ENDIF()
	ELSE (MSVC) # GCC
	  # LIQUID
	  link_directories ( ${PROJECT_SOURCE_DIR}/external/liquid-dsp/gcc/${EX_PLATFORM} )
	  configure_files(${PROJECT_SOURCE_DIR}/external/liquid-dsp/gcc/${EX_PLATFORM} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")
	  # RTL-SDR
      IF(USE_RTL_SDR)
          link_directories ( ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/gcc/${EX_PLATFORM} )
          configure_files(${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/gcc/${EX_PLATFORM} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")
      ENDIF()
	ENDIF(MSVC)  
  
  # Copy DLL files to build dir
  configure_files(${PROJECT_SOURCE_DIR}/external/fftw-3.3.4/${EX_PLATFORM_NAME} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")  
  
ENDIF (WIN32)

IF (UNIX AND NOT APPLE)
    set(BUILD_DEB OFF CACHE BOOL "Build DEB")

    IF(USE_RTL_SDR)
        SET(RTLSDR_INCLUDE "/usr/local/include" CACHE FILEPATH "RTL-SDR Include Path")
        SET(RTLSDR_LIB "/usr/local/lib" CACHE FILEPATH "RTL-SDR Lib Path")
        include_directories(${RTLSDR_INCLUDE})
        link_directories(${RTLSDR_LIB})
    ENDIF()
    SET(USE_AUDIO_PULSE ON CACHE BOOL "Use Pulse Audio")
    SET(USE_AUDIO_JACK OFF CACHE BOOL "Use Jack Audio")
    SET(USE_AUDIO_ALSA OFF CACHE BOOL "Use ALSA Audio")
    SET(USE_AUDIO_OSS OFF CACHE BOOL "Use OSS Audio")
  
    SET(FFTW_LIB fftw3f)
    SET(LIQUID_LIB liquid)
    SET(OTHER_LIBRARIES ${OTHER_LIBRARIES} dl)

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
ENDIF(UNIX AND NOT APPLE)

IF (APPLE)
    SET(CMAKE_OSX_DEPLOYMENT_TARGET, "10.9")
    IF (USE_RTL_SDR)
        SET(RTLSDR_INCLUDE "/opt/local/include" CACHE FILEPATH "RTL-SDR Include Path")
        SET(RTLSDR_LIB "/opt/local/lib" CACHE FILEPATH "RTL-SDR Lib Path")
        include_directories(${RTLSDR_INCLUDE})
        link_directories(${RTLSDR_LIB})
        SET(LIB_DIRS ${LIB_DIRS} ${RTLSDR_LIB})
    ENDIF()
  
    SET(FFTW_LIB fftw3f)
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
	src/CubicSDR.cpp
	src/AppFrame.cpp
	src/AppConfig.cpp
	src/FrequencyDialog.cpp
    src/IOThread.cpp
	src/sdr/SDRDeviceInfo.cpp
	src/sdr/SDRPostThread.cpp
	src/demod/DemodulatorPreThread.cpp
	src/demod/DemodulatorThread.cpp
	src/demod/DemodulatorWorkerThread.cpp
	src/demod/DemodulatorInstance.cpp
	src/demod/DemodulatorMgr.cpp
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
	src/process/VisualProcessor.cpp
	src/process/ScopeVisualProcessor.cpp
	src/process/SpectrumVisualProcessor.cpp
	src/process/FFTVisualDataThread.cpp
	src/process/FFTDataDistributor.cpp
    src/process/SpectrumVisualDataThread.cpp
	src/ui/GLPanel.cpp
    src/forms/SDRDevices/SDRDevices.cpp
    src/forms/SDRDevices/SDRDevicesForm.cpp
	external/rtaudio/RtAudio.cpp
	external/lodepng/lodepng.cpp
	external/tinyxml/tinyxml.cpp
	external/tinyxml/tinystr.cpp
	external/tinyxml/tinyxmlparser.cpp
	external/tinyxml/tinyxmlerror.cpp
	external/cubicvr2/math/cubic_math.cpp
)

SET (cubicsdr_headers
	src/CubicSDRDefs.h
	src/CubicSDR.h
	src/AppFrame.h
	src/AppConfig.h
	src/FrequencyDialog.h
    src/IOThread.h
	src/sdr/SDRDeviceInfo.h
	src/sdr/SDRPostThread.h
	src/demod/DemodulatorPreThread.h
	src/demod/DemodulatorThread.h
	src/demod/DemodulatorWorkerThread.h
	src/demod/DemodulatorInstance.h
	src/demod/DemodulatorMgr.h
	src/demod/DemodDefs.h
	src/audio/AudioThread.h
	src/util/Gradient.h
	src/util/Timer.h
	src/util/ThreadQueue.h
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
	external/rtaudio/RtAudio.h
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


IF (USE_SOAPY_SDR) 
    SET(cubicsdr_sources ${cubicsdr_sources} src/sdr/SoapySDRThread.cpp)
    SET(cubicsdr_headers ${cubicsdr_headers} src/sdr/SoapySDRThread.h)
ELSE()
    SET(cubicsdr_sources ${cubicsdr_sources} src/sdr/SDRThread.cpp)
    SET(cubicsdr_headers ${cubicsdr_headers} src/sdr/SDRThread.h)
ENDIF()


set(REG_EXT "[^/]*([.]cpp|[.]c|[.]h|[.]hpp)$")

SOURCE_GROUP("Base" REGULAR_EXPRESSION "src/${REG_EXT}")
SOURCE_GROUP("Forms\\SDRDevices" REGULAR_EXPRESSION "src/forms/SDRDevices/${REG_EXT}")
SOURCE_GROUP("SDR" REGULAR_EXPRESSION "src/sdr/${REG_EXT}")
SOURCE_GROUP("Demodulator" REGULAR_EXPRESSION "src/demod/${REG_EXT}")
SOURCE_GROUP("Audio" REGULAR_EXPRESSION "src/audio/${REG_EXT}")
SOURCE_GROUP("Utility" REGULAR_EXPRESSION "src/util/${REG_EXT}")
SOURCE_GROUP("Visual" REGULAR_EXPRESSION "src/visual/${REG_EXT}")
SOURCE_GROUP("Panel" REGULAR_EXPRESSION "src/panel/${REG_EXT}")
SOURCE_GROUP("Process" REGULAR_EXPRESSION "src/process/${REG_EXT}")
SOURCE_GROUP("UI" REGULAR_EXPRESSION "src/ui/${REG_EXT}")
SOURCE_GROUP("_ext-RTAudio" REGULAR_EXPRESSION "external/rtaudio/.*${REG_EXT}")
SOURCE_GROUP("_ext-LodePNG" REGULAR_EXPRESSION "external/lodepng/.*${REG_EXT}")
SOURCE_GROUP("_ext-TinyXML" REGULAR_EXPRESSION "external/tinyxml/.*${REG_EXT}")
SOURCE_GROUP("_ext-CubicVR2" REGULAR_EXPRESSION "external/cubicvr2/.*${REG_EXT}")

include_directories ( 
	${PROJECT_SOURCE_DIR}/src/forms/SDRDevices
	${PROJECT_SOURCE_DIR}/src/sdr 
	${PROJECT_SOURCE_DIR}/src/demod
	${PROJECT_SOURCE_DIR}/src/audio
	${PROJECT_SOURCE_DIR}/src/util
	${PROJECT_SOURCE_DIR}/src/panel
	${PROJECT_SOURCE_DIR}/src/visual
	${PROJECT_SOURCE_DIR}/src/process
	${PROJECT_SOURCE_DIR}/src/ui
	${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/external/rtaudio
	${PROJECT_SOURCE_DIR}/external/lodepng 
	${PROJECT_SOURCE_DIR}/external/tinyxml
	${PROJECT_SOURCE_DIR}/external/cubicvr2/math
)

IF (MSVC) 
include_directories (
	${PROJECT_SOURCE_DIR}/external/wglext
)

ELSE (MSVC)
	ADD_DEFINITIONS(
		-std=c++0x 
		-pthread
	)
ENDIF(MSVC)

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
    configure_files(${PROJECT_SOURCE_DIR}/font ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.fnt")
    configure_files(${PROJECT_SOURCE_DIR}/font ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.png")
    configure_files(${PROJECT_SOURCE_DIR}/icon ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} CubicSDR.ico)
    add_executable(CubicSDR ${cubicsdr_sources} ${cubicsdr_headers} ${RES_FILES})
    IF (USE_RTL_SDR)
        SET(OTHER_LIBRARIES rtlsdr ${OTHER_LIBRARIES})
    ENDIF()
    target_link_libraries(CubicSDR ${LIQUID_LIB} ${FFTW_LIB} ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES} ${OTHER_LIBRARIES})
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
ENDIF(MSVC)

IF (APPLE)
    ADD_DEFINITIONS(
        -DHAVE_TYPE_TRAITS=1
        -mmacosx-version-min=10.9
    )
ENDIF(APPLE)

IF (APPLE AND BUNDLE_APP)
    PROJECT(CubicSDR)
    SET(MACOSX_BUNDLE_BUNDLE_NAME CubicSDR)
    
    ADD_DEFINITIONS(
		-std=c++0x 
		-pthread
        -D_OSX_APP_
    )   
      
    ADD_EXECUTABLE(CubicSDR 
        MACOSX_BUNDLE 
        ${cubicsdr_sources}
        ${cubicsdr_headers} 
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono12.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono16.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono18.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono24.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono32.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono48.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono12_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono16_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono18_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono24_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono32_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono48_0.png
        ${PROJECT_SOURCE_DIR}/icon/CubicSDR.icns
    ) 
    
    SET_SOURCE_FILES_PROPERTIES(
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono12.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono16.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono18.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono24.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono32.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono48.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono12_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono16_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono18_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono24_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono32_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono48_0.png
        ${PROJECT_SOURCE_DIR}/icon/CubicSDR.icns
        PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
      
    IF (USE_RTL_SDR)
      SET(OTHER_LIBRARIES rtlsdr ${OTHER_LIBRARIES})
    ENDIF()

    target_link_libraries(CubicSDR ${LIQUID_LIB} ${FFTW_LIB} ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES} ${OTHER_LIBRARIES})
    SET_TARGET_PROPERTIES(CubicSDR PROPERTIES MACOSX_BUNDLE TRUE)

    SET_TARGET_PROPERTIES(CubicSDR PROPERTIES
         MACOSX_BUNDLE_BUNDLE_NAME "CubicSDR"
         # MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
         # MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
         # MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
         MACOSX_BUNDLE_GUI_IDENTIFIER "com.cubicproductions.cubicsdr"
         MACOSX_BUNDLE_ICON_FILE CubicSDR.icns
      )
    
    SET(APPS "${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME}/CubicSDR.app")
    # SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
    # SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

    INSTALL(CODE "
           SET(BU_COPY_FULL_FRAMEWORK_CONTENTS ON)
       include(BundleUtilities)
       fixup_bundle(\"${APPS}\"   \"\"   \"/usr/local/lib\")
       VERIFY_APP(\"${APPS}\")
       " COMPONENT Runtime)

   INSTALL(TARGETS CubicSDR
       BUNDLE DESTINATION . COMPONENT Runtime
   )       

   set(CPACK_BINARY_DRAGNDROP ON)
   include(CPack)
ENDIF (APPLE AND BUNDLE_APP)

IF (WIN32 AND BUILD_INSTALLER)
	set(CPACK_GENERATOR NSIS)
	set(CPACK_PACKAGE_NAME "CubicSDR")
	set(CPACK_PACKAGE_VENDOR "cubicsdr.com")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CubicSDR ${CUBICSDR_VERSION} Installer")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "CubicSDR")
	SET(CPACK_NSIS_INSTALLED_ICON_NAME "CubicSDR.ico")
	SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE") 
	set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/icon\\\\NSIS_Header.bmp")
	IF(EX_PLATFORM EQUAL 64)
		SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
		SET(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
		SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
		set(CMAKE_CL_64 TRUE)	# This gets around a bug in the CPack installer name generation for MinGW 64-bit since 2.8
	ELSE(EX_PLATFORM EQUAL 64)
		SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
		SET(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} (x86)")
		SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION} (x86)")
		set(CMAKE_CL_64 FALSE)
	ENDIF(EX_PLATFORM EQUAL 64)
	
	set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".") 
	install(TARGETS CubicSDR RUNTIME DESTINATION .)
	install(FILES 
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono12.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono16.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono18.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono24.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono32.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono48.fnt
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono12_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono16_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono18_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono24_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono32_0.png
        ${PROJECT_SOURCE_DIR}/font/vera_sans_mono48_0.png
        ${PROJECT_SOURCE_DIR}/icon/CubicSDR.ico
		${PROJECT_SOURCE_DIR}/external/fftw-3.3.4/${EX_PLATFORM}/libfftw3f-3.dll
		DESTINATION .)
	IF (MSVC)
        IF (USE_RTL_SDR)
            SET (OTHER_INSTALL_FILES ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/msvc/${EX_PLATFORM}/rtlsdr.dll)
        ENDIF()
    	install(FILES 
    		${PROJECT_SOURCE_DIR}/external/liquid-dsp/msvc/${EX_PLATFORM}/libliquid.dll
    		${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/msvc/${EX_PLATFORM}/libusb-1.0.dll
            ${OTHER_INSTALL_FILES}
    		DESTINATION .)
	ELSE (MSVC)
        IF (USE_RTL_SDR)
            SET (OTHER_INSTALL_FILES ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/gcc/${EX_PLATFORM}/librtlsdr.dll)
        ENDIF()
    	install(FILES 
    		${PROJECT_SOURCE_DIR}/external/liquid-dsp/gcc/${EX_PLATFORM}/libliquid.dll    		
    		${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/gcc/${EX_PLATFORM}/libusb-1.0.dll
            ${OTHER_INSTALL_FILES}
    		DESTINATION .)
	ENDIF(MSVC)
	
	IF(MSVC AND EX_PLATFORM EQUAL 32)
	install(FILES 
		${PROJECT_SOURCE_DIR}/external/msvc/${EX_PLATFORM_NAME}/libgcc_s_dw2-1.dll
		DESTINATION .)
	ENDIF(MSVC AND EX_PLATFORM EQUAL 32)

		set(CPACK_PACKAGE_EXECUTABLES CubicSDR "CubicSDR")

	IF (MSVC)
		install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/external/msvc/${EX_PLATFORM_NAME}/vcredist_${EX_PLATFORM_NAME}.exe DESTINATION vc_redist)
		set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\vc_redist\\\\vcredist_${EX_PLATFORM_NAME}.exe\\\" /q:a'")
	ENDIF (MSVC)
		  
		  
	INCLUDE(CPack)	
ENDIF (WIN32 AND BUILD_INSTALLER)


IF (UNIX AND BUILD_DEB) 

        set(CPACK_GENERATOR DEB)
	set(CPACK_PACKAGE_NAME "CubicSDR")
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS " libfftw3-single3, libwxgtk3.0-0, libpulse0")
    IF (USE_RTL_SDR)
    	SET(CPACK_DEBIAN_PACKAGE_DEPENDS " librtlsdr0, ${CPACK_DEBIAN_PACKAGE_DEPENDS}")
    ENDIF()
	SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Charles J. Cliffe <cj@cubicproductions.com>")
	SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "CubicSDR Software Defined Radio application v${CUBICSDR_VERSION}")
	SET(CPACK_DEBIAN_PACKAGE_SECTION "comm")
        SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
	SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${EX_PLATFORM_NAME}")
    	
	ADD_DEFINITIONS(
	    -DRES_FOLDER="/usr/share/cubicsdr/"
	    -D_FORTIFY_SOURCE=2
	)	

        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,relro")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro")

	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/external/deb/deb_post.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/deb_post.sh" @ONLY IMMEDIATE)

	INSTALL(TARGETS CubicSDR DESTINATION bin)
        install(FILES 
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono12.fnt
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono16.fnt
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono18.fnt
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono24.fnt
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono32.fnt
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono48.fnt
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono12_0.png
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono16_0.png
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono18_0.png
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono24_0.png
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono32_0.png
            ${PROJECT_SOURCE_DIR}/font/vera_sans_mono48_0.png
            ${PROJECT_SOURCE_DIR}/src/CubicSDR.png
	    DESTINATION share/cubicsdr)
            INSTALL(FILES CubicSDR.desktop 
            DESTINATION share/applications)
	INCLUDE(CPack)
ENDIF (UNIX AND BUILD_DEB)

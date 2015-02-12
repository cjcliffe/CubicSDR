cmake_minimum_required (VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

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

SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${EX_PLATFORM_NAME})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/${EX_PLATFORM_NAME})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/${EX_PLATFORM_NAME})

find_package(OpenGL REQUIRED)

find_package(wxWidgets REQUIRED gl core base)
set(wxWidgets_CONFIGURATION mswu)
include(${wxWidgets_USE_FILE})


IF (WIN32)
	set(wxWidgets_USE_STATIC ON)
	
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
	include_directories ( ${PROJECT_SOURCE_DIR}/external/fftw-3.3.4 ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release )
	set(FFTW_LIB libfftw3f-3)
	link_directories ( ${PROJECT_SOURCE_DIR}/external/fftw-3.3.4/${EX_PLATFORM} )
	configure_files(${PROJECT_SOURCE_DIR}/external/fftw-3.3.4/${EX_PLATFORM} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")

    include_directories ( ${PROJECT_SOURCE_DIR}/external/liquid-dsp/include )
	set(LIQUID_LIB libliquid)
	
	# Haven't looked into why these are different, just explicitly including everything for now until it can be sorted neatly.
	IF (MSVC)
	  # LIQUID
	  link_directories ( ${PROJECT_SOURCE_DIR}/external/liquid-dsp/msvc/${EX_PLATFORM} )
	  configure_files(${PROJECT_SOURCE_DIR}/external/liquid-dsp/msvc/${EX_PLATFORM} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME}/ "*.dll")
	  # RTL-SDR
	  link_directories ( ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/msvc/${EX_PLATFORM} )
	  configure_files(${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/msvc/${EX_PLATFORM} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")
	ELSE (MSVC) # GCC
	  # LIQUID
	  link_directories ( ${PROJECT_SOURCE_DIR}/external/liquid-dsp/gcc/${EX_PLATFORM} )
	  configure_files(${PROJECT_SOURCE_DIR}/external/liquid-dsp/gcc/${EX_PLATFORM} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")
	  # RTL-SDR
	  link_directories ( ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/gcc/${EX_PLATFORM} )
	  configure_files(${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/gcc/${EX_PLATFORM} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")
	ENDIF(MSVC)  
  
  # Copy DLL files to build dir
  configure_files(${PROJECT_SOURCE_DIR}/external/fftw-3.3.4/${EX_PLATFORM_NAME} ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.dll")  
  
ENDIF (WIN32)

IF (UNIX AND NOT APPLE)
    set(RTLSDR_INCLUDE "/usr/local/include" CACHE FILEPATH "RTL-SDR Include Path")
    set(RTLSDR_LIB "/usr/local/lib" CACHE FILEPATH "RTL-SDR Lib Path")
    set(USE_AUDIO_PULSE ON CACHE BOOL "Use Pulse Audio")
    set(USE_AUDIO_JACK OFF CACHE BOOL "Use Jack Audio")
    set(USE_AUDIO_ALSA OFF CACHE BOOL "Use ALSA Audio")
    set(USE_AUDIO_OSS OFF CACHE BOOL "Use OSS Audio")
    include_directories(${RTLSDR_INCLUDE})
    link_directories(${RTLSDR_LIB})
  
    set(FFTW_LIB fftw3f)
    set(LIQUID_LIB liquid)
    set(OTHER_LIBRARIES dl)

IF(USE_AUDIO_PULSE)
   SET (OTHER_LIBRARIES ${OTHER_LIBRARIES} pulse-simple pulse)
   ADD_DEFINITIONS(
        -D__LINUX_PULSE__
   )
ENDIF(USE_AUDIO_PULSE)

IF(USE_AUDIO_JACK)
   SET (OTHER_LIBRARIES ${OTHER_LIBRARIES} jack)
   ADD_DEFINITIONS(
        -D__UNIX_JACK__
   )
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
    set(RTLSDR_INCLUDE "/opt/local/include" CACHE FILEPATH "RTL-SDR Include Path")
    set(RTLSDR_LIB "/opt/local/lib" CACHE FILEPATH "RTL-SDR Lib Path")
    include_directories(${RTLSDR_INCLUDE})
    link_directories(${RTLSDR_LIB})
  
    set(FFTW_LIB fftw3f)
    set(LIQUID_LIB liquid)

    ADD_DEFINITIONS(
      -D__MACOSX_CORE__
    )    

   FIND_LIBRARY(COREAUDIO_LIBRARY CoreAudio)
   SET (OTHER_LIBRARIES ${COREAUDIO_LIBRARY})
ENDIF (APPLE)


SET (cubicsdr_sources
    src/CubicSDR.cpp
	src/AppFrame.cpp
	src/sdr/SDRThread.cpp
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
	src/visual/SpectrumContext.cpp
	src/visual/WaterfallCanvas.cpp
	src/visual/WaterfallContext.cpp
	external/rtaudio/RtAudio.cpp
	external/lodepng/lodepng.cpp
    external/tinyxml/tinyxml.cpp
    external/tinyxml/tinystr.cpp
    external/tinyxml/tinyxmlparser.cpp
    external/tinyxml/tinyxmlerror.cpp
)

SET (cubicsdr_headers
	src/CubicSDRDefs.h
    src/CubicSDR.h
	src/AppFrame.h
	src/sdr/SDRThread.h
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
	src/visual/SpectrumContext.h
	src/visual/WaterfallCanvas.h
	src/visual/WaterfallContext.h
	external/rtaudio/RtAudio.h
	external/lodepng/lodepng.h
    external/tinyxml/tinyxml.h
    external/tinyxml/tinystr.h
)

include_directories ( 
	${PROJECT_SOURCE_DIR}/src/sdr 
	${PROJECT_SOURCE_DIR}/src/demod
	${PROJECT_SOURCE_DIR}/src/audio
	${PROJECT_SOURCE_DIR}/src/util
	${PROJECT_SOURCE_DIR}/src/visual 
	${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/external/rtaudio
	${PROJECT_SOURCE_DIR}/external/lodepng 
	${PROJECT_SOURCE_DIR}/external/tinyxml
)

IF (MSVC) 
include_directories (
	${PROJECT_SOURCE_DIR}/external/wglext
)

ELSE (MSVC)
	ADD_DEFINITIONS(
		-std=c++0x # or -std=c++11
		-pthread
	)
ENDIF(MSVC)

configure_files(${PROJECT_SOURCE_DIR}/font ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.fnt")
configure_files(${PROJECT_SOURCE_DIR}/font ${CMAKE_BINARY_DIR}/${EX_PLATFORM_NAME} "*.png")

add_executable(CubicSDR ${cubicsdr_sources} ${cubicsdr_headers})

target_link_libraries(CubicSDR rtlsdr ${LIQUID_LIB} ${FFTW_LIB} ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES} ${OTHER_LIBRARIES})

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


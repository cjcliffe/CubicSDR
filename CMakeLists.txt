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

SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})

#add_subdirectory(${PROJECT_SOURCE_DIR}/glfw-3.0.3 ${PROJECT_BINARY_DIR}/glfw/)
#include_directories(${PROJECT_SOURCE_DIR}/glfw-3.0.3/include)
#include_directories ( ${PROJECT_SOURCE_DIR}/../CubicVR-2/build/include )
#link_directories ( ${PROJECT_SOURCE_DIR}/../CubicVR-2/build/lib )

find_package(OpenGL REQUIRED)

find_package(wxWidgets REQUIRED gl core base)
set(wxWidgets_CONFIGURATION mswu)
include(${wxWidgets_USE_FILE})
 
# include_directories ( ${PROJECT_SOURCE_DIR}/../CubicVR-2/build/include )
# link_directories ( ${PROJECT_SOURCE_DIR}/../CubicVR-2/build/lib ${OPENGL_INCLUDE_DIR})

IF (WIN32)
  set(wxWidgets_USE_STATIC ON)
  include_directories ( ${PROJECT_SOURCE_DIR}/external/fftw-3.3.4-dll64  ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release )
  link_directories (  ${PROJECT_SOURCE_DIR}/external/fftw-3.3.4-dll64  ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/x64 )
  set(FFTW_LIB fftw3-3)
    
  link_directories ( ${PROJECT_SOURCE_DIR}/external/liquid-dsp/lib/64 )
  include_directories ( ${PROJECT_SOURCE_DIR}/external/liquid-dsp/include )
  
  ADD_DEFINITIONS(
    # To use wasapi
    -D__WINDOWS_WASAPI__
	
    # To use directsound
    #-D__WINDOWS_DS__
  )
 
  # To use wasapi
  SET(OTHER_LIBRARIES -luuid -lksuser )  
  
  # To use DirectSound (which uses WASAPI anyways?)
  # SET(OTHER_LIBRARIES -ldsound)
  
  # Copy DLL files to build dir
  configure_files(${PROJECT_SOURCE_DIR}/external/liquid-dsp-lib/64 ${CMAKE_BINARY_DIR} "*.dll")
  configure_files(${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/x64 ${CMAKE_BINARY_DIR} "*.dll")
  configure_files(${PROJECT_SOURCE_DIR}/external/fftw-3.3.4-dll64 ${CMAKE_BINARY_DIR} "*.dll")
  
ENDIF (WIN32)

IF (UNIX)
    set(RTLSDR_INCLUDE "/opt/local/include" CACHE FILEPATH "RTL-SDR Include Path")
    set(RTLSDR_LIB "/opt/local/lib" CACHE FILEPATH "RTL-SDR Lib Path")
    include_directories(${RTLSDR_INCLUDE})
    link_directories(${RTLSDR_LIB})
  
    set(FFTW_LIB fftw3)

    ADD_DEFINITIONS(
        # -D__UNIX_JACK__
        # -D__LINUX_ALSA__
        # -D__LINUX_PULSE__
        # -D__LINUX_OSS__
    )
ENDIF(UNIX)

IF (APPLE)
    set(RTLSDR_INCLUDE "/opt/local/include" CACHE FILEPATH "RTL-SDR Include Path")
    set(RTLSDR_LIB "/opt/local/lib" CACHE FILEPATH "RTL-SDR Lib Path")
    include_directories(${RTLSDR_INCLUDE})
    link_directories(${RTLSDR_LIB})
  
    set(FFTW_LIB fftw3)

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
	src/demod/DemodulatorThread.cpp
	src/demod/DemodulatorWorkerThread.cpp
	src/demod/DemodulatorInstance.cpp
	src/demod/DemodulatorMgr.cpp
	src/audio/AudioThread.cpp
	src/util/Gradient.cpp
	src/util/Timer.cpp
	src/util/MouseTracker.cpp
	src/util/GLFont.cpp
	src/visual/PrimaryGLContext.cpp
	src/visual/ScopeCanvas.cpp
	src/visual/ScopeContext.cpp
	src/visual/SpectrumCanvas.cpp
	src/visual/SpectrumContext.cpp
	src/visual/WaterfallCanvas.cpp
	src/visual/WaterfallContext.cpp
	external/rtaudio/RtAudio.cpp
	external/lodepng/lodepng.cpp
)

SET (cubicsdr_headers
	src/CubicSDRDefs.h
    src/CubicSDR.h
	src/AppFrame.h
	src/sdr/SDRThread.h
	src/sdr/SDRPostThread.h
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
	src/util/GLFont.h
	src/visual/PrimaryGLContext.h
	src/visual/ScopeCanvas.h
	src/visual/ScopeContext.h
	src/visual/SpectrumCanvas.h
	src/visual/SpectrumContext.h
	src/visual/WaterfallCanvas.h
	src/visual/WaterfallContext.h
	external/rtaudio/RtAudio.h
	external/lodepng/lodepng.h
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
)

ADD_DEFINITIONS(
  -std=c++0x # or -std=c++11
  -pthread
)
 
configure_files(${PROJECT_SOURCE_DIR}/font ${CMAKE_BINARY_DIR} "*.fnt")
configure_files(${PROJECT_SOURCE_DIR}/font ${CMAKE_BINARY_DIR} "*.png")

add_executable(CubicSDR ${cubicsdr_sources} ${cubicsdr_headers})

target_link_libraries(CubicSDR rtlsdr liquid ${FFTW_LIB} ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES} ${OTHER_LIBRARIES})



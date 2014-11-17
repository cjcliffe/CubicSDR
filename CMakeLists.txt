cmake_minimum_required (VERSION 2.8)

macro(configure_files srcDir destDir)
    message(STATUS "Configuring directory ${destDir}")
    make_directory(${destDir})

    file(GLOB templateFiles RELATIVE ${srcDir} ${srcDir}/*)
    foreach(templateFile ${templateFiles})
        set(srcTemplatePath ${srcDir}/${templateFile})
        if(NOT IS_DIRECTORY ${srcTemplatePath})
#            message(STATUS "Configuring file ${templateFile}")
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
        if(NOT IS_DIRECTORY ${srcTemplatePath})
#            message(STATUS "Configuring file ${templateFile}")
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

set(wxWidgets_CONFIGURATION mswu)
find_package(wxWidgets COMPONENTS gl core base REQUIRED)
include(${wxWidgets_USE_FILE})
 
# include_directories ( ${PROJECT_SOURCE_DIR}/../CubicVR-2/build/include )
# link_directories ( ${PROJECT_SOURCE_DIR}/../CubicVR-2/build/lib ${OPENGL_INCLUDE_DIR})


if (DEFINED WIN32)
  include_directories ( ${PROJECT_SOURCE_DIR}/external/fftw-3.3.4-dll32  ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release )
  link_directories (  ${PROJECT_SOURCE_DIR}/external/fftw-3.3.4-dll32  ${PROJECT_SOURCE_DIR}/external/rtl-sdr-release/x32 )
  set(FFTW_LIB fftw3-3)
  
  include_directories ( ${PROJECT_SOURCE_DIR}/external/portaudio/include )
  link_directories ( ${PROJECT_SOURCE_DIR}/external/portaudio/libs )
  SET (PORTAUDIO_LIBRARY portaudio.dll winmm)
    
  link_directories ( ${PROJECT_SOURCE_DIR}/external/liquid-dsp/lib )
  include_directories ( ${PROJECT_SOURCE_DIR}/external/liquid-dsp/include )
else (DEFINED WIN32)
  set(RTLSDR_INCLUDE "/opt/local/include" CACHE FILEPATH "RTL-SDR Include Path")
  set(RTLSDR_LIB "/opt/local/lib" CACHE FILEPATH "RTL-SDR Lib Path")
  include_directories(${RTLSDR_INCLUDE})
  link_directories(${RTLSDR_LIB})
  
  set(FFTW_LIB fftw3)

  SET (PORTAUDIO_LIBRARY portaudio)  
endif (DEFINED WIN32)



SET (cubicsdr_sources
    src/CubicSDR.cpp
	src/SDRThread.cpp
	src/IQBufferThread.cpp
	src/PrimaryGLContext.cpp
	src/AppFrame.cpp
	src/SDRThreadQueue.cpp
	src/SDRThreadTask.cpp
	src/DemodulatorThread.cpp
	src/DemodulatorThreadQueue.cpp
	src/DemodulatorThreadTask.cpp
	src/AudioThread.cpp
	src/AudioThreadQueue.cpp
	src/AudioThreadTask.cpp
	src/Gradient.cpp
	src/Timer.cpp
	src/visual/ScopeCanvas.cpp
	src/visual/ScopeContext.cpp
	src/visual/SpectrumCanvas.cpp
	src/visual/SpectrumContext.cpp
	src/visual/WaterfallCanvas.cpp
	src/visual/WaterfallContext.cpp
)

SET (cubicsdr_headers
    src/CubicSDR.h
	src/SDRThread.h
	src/IQBufferThread.h
	src/PrimaryGLContext.h
	src/AppFrame.h
	src/CubicSDRDefs.h
	src/SDRThreadQueue.h
	src/SDRThreadTask.h
	src/DemodulatorThread.h
	src/DemodulatorThreadQueue.h
	src/DemodulatorThreadTask.h
	src/AudioThread.h
	src/AudioThreadQueue.h
	src/AudioThreadTask.h
	src/Gradient.h
	src/Timer.h
	src/visual/ScopeCanvas.h
	src/visual/ScopeContext.h
	src/visual/SpectrumCanvas.h
	src/visual/SpectrumContext.h
	src/visual/WaterfallCanvas.h
	src/visual/WaterfallContext.h
)

include_directories ( ${PROJECT_SOURCE_DIR}/src/visual 
					  ${PROJECT_SOURCE_DIR}/src )

#configure_files(${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/shaders COPYONLY)
#configure_files(${PROJECT_SOURCE_DIR}/png ${PROJECT_BINARY_DIR}/png COPYONLY)

add_executable(CubicSDR ${cubicsdr_sources} ${cubicsdr_headers})

target_link_libraries(CubicSDR rtlsdr liquid ${FFTW_LIB} ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES} ${PORTAUDIO_LIBRARY})
# cubicvr2 glfw ${GLFW_LIBRARIES} 






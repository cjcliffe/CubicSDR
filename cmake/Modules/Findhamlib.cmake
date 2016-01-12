# - Try to find Hamlib
# Author: George L. Emigh - AB4BD
# 
# Change Log: Charles J. Cliffe <cj@cubicproductions.com>
#  Updates: 
#       Jan 2015 - Add /opt/ paths for OSX MacPorts
#                - Fix HAMLIB_INCLUDE_DIR absolute search
#                - Add static lib support
#  TODO: 
#       Windows support
#
# HAMLIB_FOUND - system has Hamlib
# HAMLIB_LIBRARY - location of the library for hamlib
# HAMLIB_INCLUDE_DIR - location of the include files for hamlib

set(HAMLIB_FOUND FALSE)

find_path(HAMLIB_INCLUDE_DIR
	NAMES hamlib/rig.h
	PATHS
		/usr/include
		/usr/local/include
		/opt/local/include
)

find_library(HAMLIB_LIBRARY
	NAMES hamlib
	PATHS
		/usr/lib64/hamlib
		/usr/lib/hamlib
		/usr/lib64
		/usr/lib
		/usr/local/lib64/hamlib
		/usr/local/lib/hamlib
		/usr/local/lib64
		/usr/local/lib
		/opt/local/lib
		/opt/local/lib/hamlib
)

find_library(HAMLIB_STATIC_LIBRARY
	NAMES libhamlib.a
	PATHS
		/usr/lib64/hamlib
		/usr/lib/hamlib
		/usr/lib64
		/usr/lib
		/usr/local/lib64/hamlib
		/usr/local/lib/hamlib
		/usr/local/lib64
		/usr/local/lib
		/opt/local/lib
		/opt/local/lib/hamlib
)

GET_FILENAME_COMPONENT(HAMLIB_LIB_FOLDER ${HAMLIB_STATIC_LIBRARY} DIRECTORY)

file(GLOB HAMLIB_STATIC_MODS ${HAMLIB_LIB_FOLDER}/hamlib-*.a)

if(HAMLIB_INCLUDE_DIR AND HAMLIB_LIBRARY)
	set(HAMLIB_FOUND TRUE)
    # message(STATUS "Hamlib version: ${VERSION}")
	message(STATUS "Found hamlib library at: ${HAMLIB_LIBRARY}")
	message(STATUS "Found hamlib static library at: ${HAMLIB_STATIC_LIBRARY}")
	message(STATUS "Found hamlib static modules: ${HAMLIB_STATIC_MODS}")
	message(STATUS "Found hamlib include directory at: ${HAMLIB_INCLUDE_DIR}")
endif(HAMLIB_INCLUDE_DIR AND HAMLIB_LIBRARY)

IF(NOT HAMLIB_FOUND)
  IF(NOT HAMLIB_FIND_QUIETLY)
    MESSAGE(STATUS "HAMLIB was not found.")
  ELSE(NOT HAMLIB_FIND_QUIETLY)
    IF(HAMLIB_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "HAMLIB was not found.")
    ENDIF(HAMLIB_FIND_REQUIRED)
  ENDIF(NOT HAMLIB_FIND_QUIETLY)
ENDIF(NOT HAMLIB_FOUND)
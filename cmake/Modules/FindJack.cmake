# Try to find JACK
# This will define the following variables:
#
#  JACK_FOUND - Whether Jack was found.
#  JACK_INCLUDE_DIRS - Jack include directories.
#  JACK_LIBRARIES - Jack libraries.

include(FindPackageHandleStandardArgs)

if(JACK_LIBRARIES AND JACK_INCLUDE_DIRS)

  # in cache already
  set(JACK_FOUND TRUE)

else()

  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(_JACK jack)
  endif(PKG_CONFIG_FOUND)

  find_path(JACK_INCLUDE_DIR
    NAMES
      jack/jack.h
    PATHS
      ${_JACK_INCLUDEDIR}
  )
  
  find_library(JACK_LIBRARY
    NAMES
      jack
    PATHS
      ${_JACK_LIBDIR}
  )

  set(JACK_INCLUDE_DIRS
    ${JACK_INCLUDE_DIR}
  )

  set(JACK_LIBRARIES
    ${JACK_LIBRARY}
  )

  find_package_handle_standard_args(Jack DEFAULT_MSG JACK_LIBRARIES JACK_INCLUDE_DIRS)

  # show the JACK_INCLUDE_DIRS and JACK_LIBRARIES variables only in the advanced view
  mark_as_advanced(JACK_INCLUDE_DIR JACK_LIBRARY JACK_INCLUDE_DIRS JACK_LIBRARIES)

endif()


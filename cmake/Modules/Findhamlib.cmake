# - Try to find hamlib
# Once done, this will define:
#
#  hamlib_FOUND - system has Hamlib-2
#  hamlib_INCLUDE_DIRS - the Hamlib-2 include directories
#  hamlib_LIBRARIES - link these to use Hamlib-2
#  hamlib_STATIC_FOUND - system has Hamlib-2 static archive
#  hamlib_STATIC_LIBRARIES - link these to use Hamlib-2 static archive

include (LibFindMacros)

# pkg-config?
find_path (__hamlib_pc_path NAMES hamlib.pc
  PATH_SUFFIXES lib/pkgconfig
)
if (__hamlib_pc_path)
  set (ENV{PKG_CONFIG_PATH} "${__hamlib_pc_path}" "$ENV{PKG_CONFIG_PATH}")
  unset (__hamlib_pc_path CACHE)
endif ()

# Use pkg-config to get hints about paths, libs and, flags
unset (__pkg_config_checked_hamlib CACHE)
libfind_pkg_check_modules (PC_HAMLIB hamlib)

if (NOT PC_HAMLIB_STATIC_LIBRARIES)
  if (WIN32)
    set (PC_HAMLIB_STATIC_LIBRARIES hamlib ws2_32)
  else ()
    set (PC_HAMLIB_STATIC_LIBRARIES hamlib m dl usb)
  endif ()
endif ()

# The libraries
libfind_library (hamlib hamlib)
libfind_library (hamlib_STATIC libhamlib.a)

find_path (hamlib_INCLUDE_DIR hamlib/rig.h)

# Set the include dir variables and the libraries and let libfind_process do the rest
set (hamlib_PROCESS_INCLUDES hamlib_INCLUDE_DIR)
set (hamlib_PROCESS_LIBS hamlib_LIBRARY)
libfind_process (hamlib)

set (hamlib_STATIC_PROCESS_INCLUDES hamlib_STATIC_INCLUDE_DIR)
set (hamlib_STATIC_PROCESS_LIBS hamlib_STATIC_LIBRARY PC_HAMLIB_STATIC_LIBRARIES)
libfind_process (hamlib_STATIC)

# make sure we return a full path for the library we return
if (hamlib_FOUND)
  list (REMOVE_ITEM hamlib_LIBRARIES hamlib)
  if (hamlib_STATIC_LIBRARIES)
    list (REMOVE_ITEM hamlib_STATIC_LIBRARIES hamlib)
  endif ()
endif ()

# Handle the  QUIETLY and REQUIRED  arguments and set  HAMLIB_FOUND to
# TRUE if all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (hamlib DEFAULT_MSG hamlib_INCLUDE_DIRS hamlib_LIBRARY hamlib_LIBRARIES)

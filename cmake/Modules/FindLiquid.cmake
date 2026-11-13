# - Find LIQUID
# Find the native LIQUID includes and library
#
#  LIQUID_INCLUDES    - where to find liquid/liquid.h
#  LIQUID_LIBRARIES   - List of libraries when using liquid-dsp.
#  LIQUID_FOUND       - True if LIQUID found.

find_path (LIQUID_INCLUDES liquid/liquid.h)

find_library (LIQUID_LIBRARIES NAMES liquid)

# handle the QUIETLY and REQUIRED arguments and set LIQUID_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (Liquid DEFAULT_MSG LIQUID_LIBRARIES LIQUID_INCLUDES)

#mark_as_advanced (LIQUID_LIBRARIES LIQUID_INCLUDES)

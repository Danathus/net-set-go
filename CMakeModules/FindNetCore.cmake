# Locate NetCore library
#
# Will define:
# ${NETCORE_LIBRARIES} for use with TARGET_LINK_LIBRARIES()
# ${NETCORE_INCLUDE_DIRECTORIES} for use with INCLUDE_DIRECTORIES()

INCLUDE(netsetgo_common)
 
# variable names of the individual NetSetGo libraries.  Can be used in application cmakelist.txt files.
FIND_NETSETGO_LIBRARY(NETCORE_LIBRARY       NetCore)
FIND_NETSETGO_LIBRARY(NETCORE_DEBUG_LIBRARY NetCoreD)

IF (NOT NETCORE_DEBUG_LIBRARY)
  SET(NETCORE_DEBUG_LIBRARY NETCORE_LIBRARY)
  MESSAGE(STATUS "No debug library was found for NETCORE_DEBUG_LIBRARY")
ENDIF()

# convienent list of libraries to link with when using NetCore
SET(NETCORE_LIBRARIES
    optimized ${NETCORE_LIBRARY} debug ${NETCORE_DEBUG_LIBRARY}
)

SET(NETCORE_INCLUDE_DIRECTORIES ${NETSETGO_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set NETCORE_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NetCore DEFAULT_MSG NETSETGO_INCLUDE_DIR NETCORE_LIBRARY)

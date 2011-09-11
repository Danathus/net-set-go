# Locate NetSetGo libraries
#
# Assumes that you want to locate all NetSetGo libraries.
#
# Will define:
# NETSETGO_FOUND               - system has NetSetGo
# NETSETGO_LIBRARIES           - for use with TARGET_LINK_LIBRARIES()
# NETSETGO_INCLUDE_DIRECTORIES - for use with INCLUDE_DIRECTORIES()

INCLUDE(FindNetCore)

# convienent list of libraries to link with when using NetSetGo
SET(NETSETGO_LIBRARIES
    ${NETCORE_LIBRARIES}
)

SET(NETSETGO_INCLUDE_DIRECTORIES ${NETSETGO_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set NETSETGO_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NetSetGo DEFAULT_MSG NETSETGO_INCLUDE_DIRECTORIES NETSETGO_LIBRARIES)

#IF(NETSETGO_INCLUDE_DIRECTORIES AND NETSETGO_LIBRARIES)
#  SET(NETSETGO_FOUND TRUE CACHE STRING "Whether NetSetGo was found or not" FORCE)
#  MESSAGE(STATUS "Found NetSetGo: ${NETSETGO_LIBRARIES} (Headers in ${NETSETGO_INCLUDE_DIRECTORIES})")
#ELSE()
#  SET(NETSETGO_FOUND FALSE CACHE STRING "Whether NetSetGo was found or not" FORCE)
#  IF(NETSETGO_FIND_REQUIRED)
#    MESSAGE(FATAL_ERROR "Could not find NetSetGo library. Please install NetSetGo")
#  ELSE()
#    MESSAGE(STATUS "Did not find NetSetGo")
#  ENDIF()
#ENDIF()

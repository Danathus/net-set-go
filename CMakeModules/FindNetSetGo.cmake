# Locate NetSetGo libraries
#
# Assumes that you want to locate all NetSetGo libraries.
#
# Will define:
# ${NETSETGO_LIBRARIES} for use with TARGET_LINK_LIBRARIES()
# ${NETSETGO_INCLUDE_DIRECTORIES} for use with INCLUDE_DIRECTORIES()

INCLUDE(FindNetCore)

# convienent list of libraries to link with when using NetSetGo
SET(NETSETGO_LIBRARIES
    ${NETCORE_LIBRARIES}
)

SET(NETSETGO_INCLUDE_DIRECTORIES ${NETSETGO_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set NETSETGO_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NetSetGo DEFAULT_MSG NETSETGO_INCLUDE_DIR NETSETGO_LIBRARIES)

# This defines the follow variables
# NETSETGO_ROOT        : The root of the NetSetGo directory
# NETSETGO_INCLUDE_DIR : The NetSetGo include directory

#
# This folder contains some functions which helps find the indidivual parts of NetSetGo
# and is typically included by other .cmake files.

# where to find the root NetSetGo folder
FIND_PATH(NETSETGO_ROOT source
          PATHS
          $ENV{NETSETGO_ROOT}
          DOC "The root folder of NetSetGo"
)

# where to find the NetSetGo include dir
FIND_PATH(NETSETGO_INCLUDE_DIR NetSetGo/NetCore/NetCoreExport.h
          PATHS
          ${NETSETGO_ROOT}/inclucde
          $ENV{NETSETGO_ROOT}/include
          /usr/local/include
          /usr/freeware/include     
          DOC "The NetSetGo include folder. Should contain 'NetCore'"
)


# where to find the NetSetGo lib dir
SET(NETSETGO_LIB_SEARCH_PATH 
    ${NETSETGO_ROOT}/lib
    ${NETSETGO_ROOT}/build/lib
    $ENV{NETSETGO_ROOT}/build/lib
    /usr/local/lib
    /usr/lib
)

MACRO(FIND_NETSETGO_LIBRARY LIB_VAR LIB_NAME)
  FIND_LIBRARY(${LIB_VAR} NAMES ${LIB_NAME}
               PATHS
               ${NETSETGO_LIB_SEARCH_PATH}
  )
ENDMACRO(FIND_NETSETGO_LIBRARY LIB_VAR LIB_NAME)            

# iOS library finding routines
MACRO(FIND_NETSETGO_IOS_LIBRARY LIB_VAR PLATFORM CONFIG LIB_NAME)
  FIND_LIBRARY(${LIB_VAR} NAMES ${LIB_NAME}
               PATHS
               "$ENV{NETSETGO_ROOT}/build_iOS/lib/${CONFIG}-${PLATFORM}"
               "$ENV{NETSETGO_ROOT}/build_iOS/NetSetGo.build/${CONFIG}-${PLATFORM}"
               "$ENV{NETSETGO_ROOT}/build/lib/${CONFIG}-${PLATFORM}"
               "$ENV{NETSETGO_ROOT}/build/NetSetGo.build/${CONFIG}-${PLATFORM}"
              )
ENDMACRO(FIND_NETSETGO_IOS_LIBRARY LIB_VAR LIB_NAME)

MACRO(FIND_NETSETGO_IOS_LIBRARIES LIB_VAR_BASE LIB_NAME)
  FIND_NETSETGO_IOS_LIBRARY(${LIB_VAR_BASE}_IPHONESIM       iphonesimulator Release ${LIB_NAME})
  FIND_NETSETGO_IOS_LIBRARY(${LIB_VAR_BASE}_IPHONESIM_DEBUG iphonesimulator Debug   ${LIB_NAME})
  FIND_NETSETGO_IOS_LIBRARY(${LIB_VAR_BASE}_IPHONEOS        iphoneos        Release ${LIB_NAME})
  FIND_NETSETGO_IOS_LIBRARY(${LIB_VAR_BASE}_IPHONEOS_DEBUG  iphoneos        Debug   ${LIB_NAME})
ENDMACRO(FIND_NETSETGO_IOS_LIBRARIES LIB_VAR_BASE LIB_NAME)

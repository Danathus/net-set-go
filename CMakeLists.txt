cmake_minimum_required(VERSION 2.6)

# the project's name
PROJECT(NetSetGo)

# Set where to look for the Find*.cmake files
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules  # here
                      ${CMAKE_MODULE_PATH})             # everywhere

INCLUDE(AddFiles)

################################################################################
# Visual studio specific
IF (WIN32)
   SET(CMAKE_DEBUG_POSTFIX  "d")
   
   OPTION(BUILD_WITH_MP "Enables the /MP multi-processor compiler option for Visual Studio 2005 and above" ON)
   MARK_AS_ADVANCED(BUILD_WITH_MP)
   IF(BUILD_WITH_MP)
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
   ENDIF(BUILD_WITH_MP)
   
   # Platform specific definitions
   ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
   ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)  
   ADD_DEFINITIONS(-DNOMINMAX)
ENDIF (WIN32)
################################################################################

SET(SOURCE_PATH ${CMAKE_SOURCE_DIR}/source)
SET(HEADER_PATH ${CMAKE_SOURCE_DIR}/include)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

MAKE_DIRECTORY(${EXECUTABLE_OUTPUT_PATH})
MAKE_DIRECTORY(${LIBRARY_OUTPUT_PATH})

FIND_PACKAGE(OpenThreads REQUIRED) # is there a better way to do threading?

# OPTIONS
OPTION(BUILD_TESTS "Enables the building of the NetSetGo unit tests" OFF)

################################################################################
# handle the installation of the ext deps
install(DIRECTORY ${EXT_BIN_DIR} DESTINATION ${PROJECT_BINARY_DIR}
        FILES_MATCHING PATTERN "*.dll"
        PATTERN ".svn" EXCLUDE)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
               IMMEDIATE @ONLY)
  
ADD_CUSTOM_TARGET(UNINSTALL
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

################################################################################

ADD_SUBDIRECTORY(${SOURCE_PATH}/NetCore)

# tests
IF (BUILD_TESTS)
  ADD_SUBDIRECTORY(tests)
ENDIF (BUILD_TESTS)

################################################################################

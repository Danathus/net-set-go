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

FIND_PACKAGE(OpenThreads) # is there a better way to do threading?

# OPTIONS
IF (OPENTHREADS_FOUND)
  OPTION(BUILD_TESTS "Enables the building of the NetSetGo unit tests" OFF)
ENDIF (OPENTHREADS_FOUND)
IF(APPLE)
  INCLUDE("$ENV{TWILIGHT_ROOT}/CMakeModules/OSX.cmake")
  OPTION(BUILD_IOS "Build for iOS (requires the iPhone SDK)" ON)
  OPTION(BUILD_OSX "Build for OSX (Mac PC)" OFF)
ENDIF(APPLE)

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

# iOS stuff

IF (BUILD_IOS)
  #SET(CMAKE_OSX_SYSROOT iphoneos4.2)
  SET(CMAKE_OSX_ARCHITECTURES $(ARCHS_STANDARD_32_BIT))
  SET(CMAKE_CXX_FLAGS "-x objective-c++")
  SET(CMAKE_EXE_LINKER_FLAGS
    "-framework Foundation -framework CoreGraphics -framework OpenGLES -framework AudioToolbox -framework QuartzCore -framework UIKit -framework OpenAL -framework MediaPlayer -framework CFNetwork -framework SystemConfiguration"
  )
  LINK_DIRECTORIES(\${HOME}/\${SDKROOT}/lib
                   ${LIBRARY_OUTPUT_PATH}/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)
  )

  SET(APP_TYPE MACOSX_BUNDLE)

  #####

  SET(TARGETSDK iPhoneOS4.2.sdk)
  SET(CMAKE_OSX_SYSROOT /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/${TARGETSDK})
  macro(ADD_FRAMEWORK fwname appname)
    find_library(FRAMEWORK_${fwname}
      NAMES ${fwname}
      PATHS ${CMAKE_OSX_SYSROOT}/System/Library
      PATH_SUFFIXES Frameworks
      NO_DEFAULT_PATH)
    if( ${FRAMEWORK_${fwname}} STREQUAL FRAMEWORK_${fwname}-NOTFOUND)
      MESSAGE(ERROR ": Framework ${fwname} not found")
    else()
      TARGET_LINK_LIBRARIES(${appname} ${FRAMEWORK_${fwname}})
      MESSAGE(STATUS "Framework ${fwname} found at ${FRAMEWORK_${fwname}}")
    endif()
  endmacro(ADD_FRAMEWORK)

ENDIF(BUILD_IOS)

IF (BUILD_IOS)
  INCLUDE_DIRECTORIES(/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/${TARGETSDK}/System/Library/Frameworks/OpenAL.framework/Headers
  )
ENDIF (BUILD_IOS)

MESSAGE( STATUS "CMAKE_OSX_SYSROOT:         " ${CMAKE_OSX_SYSROOT} )

################################################################################

ADD_SUBDIRECTORY(${SOURCE_PATH}/NetSetGo)

# tests
IF (BUILD_TESTS)
  ADD_SUBDIRECTORY(tests)
ENDIF (BUILD_TESTS)

################################################################################

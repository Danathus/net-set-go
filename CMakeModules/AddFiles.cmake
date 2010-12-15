# Arguments:
#    OUT       - a list of source files to append to
#    ROOT_PATH - the base path to find source files in
#    FILE_PATH - the local path (appended to root) to
#                search for source files in
#
# Notes:
#    - OUT is built up and given to ADD_EXECUTABLE etc
#    - will find ${ROOT_PATH}/${FILE_PATH}/*.c* (.c, .cpp, etc)
#
# Example usage:
#    ADD_SOURCE_FILES(EXE_SOURCE_FILES ${SOURCE_PATH} demo)
#
FUNCTION(ADD_SOURCE_FILES OUT ROOT_PATH FILE_PATH)
   # generate a list of source files in this directory
   if(FILE_PATH)
      file(GLOB TEMP_SOURCES ${ROOT_PATH}/${FILE_PATH}/*.c*)
   else(FILE_PATH)
      file(GLOB TEMP_SOURCES ${ROOT_PATH}/*.c*)
   endif(FILE_PATH)
   # add to the list of files we're building up
   list(APPEND ${OUT} ${TEMP_SOURCES})
   # this is required to write not just the local OUT var
   #    but the one in the scope of whatever called us
   SET("${OUT}" ${${OUT}} PARENT_SCOPE)

   # create a source group for these files
   if(FILE_PATH)
      string(REGEX REPLACE "/" "\\\\" SOURCE_GROUP_NAME ${FILE_PATH})
      # replace all of SOURCE_GROUP_NAME with itself prepended by "Source Files\"
      string(REPLACE "${SOURCE_GROUP_NAME}" "Source Files\\${SOURCE_GROUP_NAME}" SOURCE_GROUP_NAME ${SOURCE_GROUP_NAME})
      source_group("${SOURCE_GROUP_NAME}" FILES ${TEMP_SOURCES})
   endif(FILE_PATH)
ENDFUNCTION()

# Arguments:
#    OUT       - a list of header files to append to
#    ROOT_PATH - the base path to find header files in
#    FILE_PATH - the local path (appended to root) to
#                search for header files in
#
# Notes:
#    - OUT is built up and given to ADD_EXECUTABLE etc
#    - will find ${ROOT_PATH}/${FILE_PATH}/*.h* (.h, .hpp, etc)
#
# Example usage:
#    ADD_SOURCE_FILES(EXE_HEADER_FILES ${HEADER_PATH} demo)
#
FUNCTION(ADD_HEADER_FILES OUT ROOT_PATH FILE_PATH)
   # generate a list of header files in this directory
   if(FILE_PATH)
      file(GLOB TEMP_HEADERS ${ROOT_PATH}/${FILE_PATH}/*.h*)
   else(FILE_PATH)
      file(GLOB TEMP_HEADERS ${ROOT_PATH}/*.h*)
   endif(FILE_PATH)
   # add to the list of files we're building up
   list(APPEND ${OUT} ${TEMP_HEADERS})
   # this is required to write not just the local OUT var
   #    but the one in the scope of whatever called us
   SET("${OUT}" ${${OUT}} PARENT_SCOPE)

   # create a source group for these files
   if(FILE_PATH)
      string(REGEX REPLACE "/" "\\\\" HEADER_GROUP_NAME ${FILE_PATH})
      # replace all of HEADER_GROUP_NAME with itself prepended by "Header Files\"
      string(REPLACE "${HEADER_GROUP_NAME}" "Header Files\\${HEADER_GROUP_NAME}" HEADER_GROUP_NAME ${HEADER_GROUP_NAME})
      source_group("${HEADER_GROUP_NAME}" FILES ${TEMP_HEADERS})
   endif(FILE_PATH)
ENDFUNCTION()

SET(EXE_NAME testNetCore)

SET(SOURCE_DIR ${CMAKE_SOURCE_DIR}/tests/NetCore)
SET(HEADER_DIR ${CMAKE_SOURCE_DIR}/tests/NetCore)

ADD_SOURCE_FILES(EXE_SOURCE_FILES ${SOURCE_DIR} "")
ADD_HEADER_FILES(EXE_HEADER_FILES ${HEADER_DIR} "")

ADD_EXECUTABLE(${EXE_NAME}
               ${EXE_SOURCE_FILES}
               ${EXE_HEADER_FILES}
)

INCLUDE_DIRECTORIES(${HEADER_PATH}
                    # todo: add more
)

# little hack to get the .dll into /bin and .lib into /lib
IF (MSVC)
  SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES PREFIX "../../bin/")
  SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES IMPORT_PREFIX "../")
ENDIF (MSVC)

# libraries to link into the executable, both Release and Debug
TARGET_LINK_LIBRARIES(${EXE_NAME}
                      NetCore
                      optimized ${OPENTHREADS_LIBRARY} debug  ${OPENTHREADS_LIBRARY_DEBUG}
)

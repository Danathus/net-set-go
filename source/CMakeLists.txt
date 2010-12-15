SET(LIB_NAME NetCore)

SET(SOURCE_DIR ${SOURCE_PATH}/${LIB_NAME})
SET(HEADER_DIR ${HEADER_PATH}/${LIB_NAME})

ADD_SOURCE_FILES(LIB_SOURCE_FILES ${SOURCE_DIR} "")

ADD_HEADER_FILES(LIB_HEADER_FILES ${HEADER_DIR} "")

ADD_LIBRARY(${LIB_NAME} SHARED
            ${LIB_SOURCE_FILES}
            ${LIB_HEADER_FILES}
)

INCLUDE_DIRECTORIES(${HEADER_PATH}
                    # todo: add more
)

# little hack to get the .dll into /bin and .lib into /lib
IF (MSVC)
  SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES PREFIX "../../bin/")
  SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES IMPORT_PREFIX "../")
ENDIF (MSVC)

#libraries to link into the executable, both Release and Debug
TARGET_LINK_LIBRARIES(${LIB_NAME}
                      ws2_32
                      ${DTUTIL_LIBRARIES}
)

SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES DEFINE_SYMBOL NETCORE)

SET_TARGET_PROPERTIES(${LIB_NAME} 
                       PROPERTIES FRAMEWORK TRUE 
                       PUBLIC_HEADER "${LIB_HEADER_FILES}"
                       )

INSTALL(TARGETS ${LIB_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${LIB_NAME}
)

file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
file(GLOB_RECURSE HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")

add_library(statme STATIC ${SOURCES} ${HEADERS}) 

target_include_directories(statme PUBLIC
  ${ALLSTAT_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${LOGME_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
  ${SYNCME_INCLUDE_DIR}
)

target_compile_definitions(statme PRIVATE SYNCME_BUILD)

if(USE_ALLSTAT)
  target_compile_definitions(statme PUBLIC USE_ALLSTAT)
endif()

if(WIN32)
  target_compile_definitions(statme PUBLIC 
    _WINSOCK_DEPRECATED_NO_WARNINGS
    _CRT_SECURE_NO_WARNINGS
  )
endif()


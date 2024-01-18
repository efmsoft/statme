file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
file(GLOB_RECURSE HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")

if(NOT DEFINED USE_STATMED_ONLY)
  set(USE_STATMED_ONLY OFF)
endif()

if(NOT DEFINED USE_STATME_ONLY)
  set(USE_STATME_ONLY OFF)
endif()

set(STATME_INCLUDE_DIRS 
  ${ALLSTAT_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${LOGME_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
  ${SYNCME_INCLUDE_DIR}
)

set(STATME_WIN32_DEFS
  _WINSOCK_DEPRECATED_NO_WARNINGS
  _CRT_SECURE_NO_WARNINGS
)

if(NOT ${USE_STATMED_ONLY})
  add_library(statme STATIC ${SOURCES} ${HEADERS})
  target_include_directories(statme PUBLIC ${STATME_INCLUDE_DIRS})
  target_compile_definitions(statme PRIVATE _STATME_STATIC_BUILD_)
  if(USE_ALLSTAT)
    target_compile_definitions(statme PUBLIC USE_ALLSTAT)
  endif()
  if(WIN32)
    target_compile_definitions(statme PUBLIC ${STATME_WIN32_DEFS})
  endif()
endif()

if(NOT ${USE_STATME_ONLY})
  add_library(statmed SHARED ${SOURCES} ${HEADERS}) 
  target_include_directories(statmed PUBLIC ${STATME_INCLUDE_DIRS})
  target_compile_definitions(statmed PRIVATE _STATME_DLL_BUILD_)

  if(USE_ALLSTAT)
    target_compile_definitions(statmed PUBLIC USE_ALLSTAT)
  endif()

  if(WIN32)
    target_compile_definitions(statmed PUBLIC ${STATME_WIN32_DEFS})
  endif()
  target_link_libraries(statmed LINK_PRIVATE syncme ${LOGME_LIBRARIES})

  if(NOT WIN32)
    target_link_libraries(statmed LINK_PUBLIC pthread) 
  endif()
endif()
 
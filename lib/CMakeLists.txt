file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
file(GLOB_RECURSE HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include(AsUCharArray)

AsUCharArray(favicon.ico Favicon 16 FAVICON_SOURCE FAVICON_HEADER)
list(APPEND SOURCES ${FAVICON_SOURCE})
list(APPEND HEADERS ${FAVICON_HEADER})

AsUCharArray(statme.css Style 16 STYLE_SOURCE STYLE_HEADER)
list(APPEND SOURCES ${STYLE_SOURCE})
list(APPEND HEADERS ${STYLE_HEADER})
 
if(NOT DEFINED USE_STATMED_ONLY)
  set(USE_STATMED_ONLY OFF)
endif()

if(NOT DEFINED USE_STATME_ONLY)
  set(USE_STATME_ONLY OFF)
endif()

set(STATME_INCLUDE_DIRS 
  ${ALLSTAT_INCLUDE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${LOGME_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
  ${SYNCME_INCLUDE_DIR}
  ${JSONCPP_INCLUDE_DIR}
)

set(STATME_WIN32_DEFS
  _WINSOCK_DEPRECATED_NO_WARNINGS
  _CRT_SECURE_NO_WARNINGS
  USE_LOGME
)

if(NOT ${USE_STATMED_ONLY})
  add_library(statme STATIC ${SOURCES} ${HEADERS})
  target_include_directories(statme PUBLIC ${STATME_INCLUDE_DIRS})
  target_compile_definitions(statme PRIVATE _STATME_STATIC_BUILD_)
  target_compile_definitions(statme PUBLIC USE_ALLSTAT USE_LOGME)
  if(WIN32)
    target_compile_definitions(statme PUBLIC ${STATME_WIN32_DEFS})
  endif()
endif()

if(NOT ${USE_STATME_ONLY})
  add_library(statmed SHARED ${SOURCES} ${HEADERS}) 
  target_include_directories(statmed PUBLIC ${STATME_INCLUDE_DIRS})
  target_compile_definitions(statmed PRIVATE _STATME_DLL_BUILD_)

  if(NOT WIN32)
    target_compile_options(statmed PRIVATE -fPIC -fvisibility=default)
  endif()

  target_compile_definitions(statmed PUBLIC USE_ALLSTAT USE_LOGME)

  if(WIN32)
    target_compile_definitions(statmed PUBLIC ${STATME_WIN32_DEFS})
  endif()
  target_link_libraries(statmed LINK_PRIVATE syncme ${LOGME_LIBRARIES} ${OPENSSL_LIBRARIES} ${JSONCPP_LIBRARIES})

  if(WIN32)
    target_link_libraries(statmed LINK_PUBLIC WS2_32.Lib) 
  else()
    target_link_libraries(statmed LINK_PUBLIC pthread) 
  endif()
endif()
 
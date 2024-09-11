macro(Findjsoncpp)
  set(folder ${CMAKE_CURRENT_SOURCE_DIR})

  while(TRUE)
    message("Processing ${folder}, project root ${CMAKE_SOURCE_DIR}")

    # Communicator and jsoncpp are submodules in NFS
    if(EXISTS ${folder}/jsoncpp/jsoncppConfig.cmake.in)
      message("Found jsoncpp library: ${folder}/jsoncpp")
      set(JSONCPP_ROOT ${folder}/jsoncpp)
      break()
    endif()

    # jsoncpp is checked out from git to bin dir is part of Communicator
    if(EXISTS ${folder}/bin/jsoncpp/jsoncppConfig.cmake.in)
      message("Found jsoncpp library: ${folder}/bin/jsoncpp")
      set(JSONCPP_ROOT ${folder}/bin/jsoncpp)
      break()
    endif()

    # jsoncpp is submodules in NFS and we are called from top level CMakeLists.txt
    if(EXISTS ${folder}/Modules/jsoncpp/jsoncppConfig.cmake.in)
      message("Found jsoncpp library: ${folder}/Modules/jsoncpp")
      set(JSONCPP_ROOT ${folder}/Modules/jsoncpp)
      break()
    endif()

    # do not leave project
    if(folder STREQUAL ${CMAKE_SOURCE_DIR})
      break()
    endif()

    cmake_path(HAS_PARENT_PATH folder has_parent)
    if(NOT has_parent)
      break()
    endif()

    set(previous ${folder})
    cmake_path(GET folder PARENT_PATH folder)

    if(folder STREQUAL previous)
      break()
    endif()
  endwhile()
endmacro()

set(JSONCPP_ROOT "")
Findjsoncpp()

if(JSONCPP_ROOT STREQUAL "")
  if(NOT FORCE_FETCH)
    message(FATAL_ERROR "Internal error:FORCE_FETCH disabled but jsoncpp is not found.")
  endif()

  set(JSONCPP_ROOT "${CMAKE_SOURCE_DIR}/bin/jsoncpp")
  if(NOT EXISTS JSONCPP_ROOT)
    include(FetchContent)
    FetchContent_Declare(jsoncpp
      GIT_REPOSITORY "https://github.com/open-source-parsers/jsoncpp.git"
      GIT_TAG master
      SOURCE_SUBDIR jsoncpp
      SOURCE_DIR "${JSONCPP_ROOT}"
    )
    FetchContent_MakeAvailable(jsoncpp)
  endif()
endif()

# Add jsoncpp library only. Without examples and test

# Hack jsoncpp defines name in below way
#    set(STATIC_LIB ${PROJECT_NAME}_static)
#    add_library(${STATIC_LIB} STATIC ${PUBLIC_HEADERS} ${JSONCPP_SOURCES})
set(_PROJECT_NAME_SAVED ${PROJECT_NAME})
set(PROJECT_NAME jsoncpp)
set(BUILD_STATIC_LIBS ON)
add_subdirectory(${JSONCPP_ROOT}/src/lib_json)
set(PROJECT_NAME ${_PROJECT_NAME_SAVED})
set(JSONCPP_INCLUDE_DIR ${JSONCPP_ROOT}/include)

set(JSONCPP_LIBRARIES jsoncpp_static)

include_directories(jsoncpp_static PUBLIC
  ${JSONCPP_INCLUDE_DIR}
)

if(TARGET jsoncpp_static)
  set_target_properties(jsoncpp_static PROPERTIES FOLDER "Dependencies")
endif()

message("Findjsoncpp.cmake: JSONCPP_ROOT=${JSONCPP_ROOT}")
message("Findjsoncpp.cmake: JSONCPP_INCLUDE_DIR=${JSONCPP_INCLUDE_DIR}")
message("Findjsoncpp.cmake: JSONCPP_LIBRARIES=${JSONCPP_LIBRARIES}")


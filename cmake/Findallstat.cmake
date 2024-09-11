macro(Findallstat)
  set(folder ${CMAKE_CURRENT_SOURCE_DIR})

  while(TRUE)
    message("Processing ${folder}, project root ${CMAKE_SOURCE_DIR}")

    if(EXISTS ${folder}/allstat/AllStat/include/AllStat/AllStat.h)
      message("Found allstat library: ${folder}/allstat")
      set(ALLSTAT_ROOT ${folder}/allstat)
      break()
    endif()

    if(EXISTS ${folder}/bin/allstat/AllStat/include/AllStat/AllStat.h)
      message("Found allstat library: ${folder}/bin/allstat")
      set(ALLSTAT_ROOT ${folder}/bin/allstat)
      break()
    endif()

    if(EXISTS ${folder}/Modules/allstat/AllStat/include/AllStat/AllStat.h)
      message("Found allstat library: ${folder}/Modules/allstat")
      set(ALLSTAT_ROOT ${folder}/Modules/allstat)
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

set(ALLSTAT_ROOT "")
Findallstat()

if(ALLSTAT_ROOT STREQUAL "")
  if(NOT FORCE_FETCH)
    message(FATAL_ERROR "Internal error:FORCE_FETCH disabled but allstat is not found.")
  endif()
  set(ALLSTAT_ROOT "${CMAKE_SOURCE_DIR}/bin/allstat")
  include(FetchContent)
  FetchContent_Declare(allstat
    GIT_REPOSITORY "https://github.com/efmsoft/allstat.git"
    GIT_TAG "master"
    SOURCE_DIR "${ALLSTAT_ROOT}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
  )
  FetchContent_GetProperties(allstat)
  if(NOT allstat_POPULATED)
    FetchContent_Populate(allstat)
  endif()
endif()

if(ALLSTAT_ROOT STREQUAL "")
  message(FATAL_ERROR "Cannot find allstat root.")
endif()

include(${ALLSTAT_ROOT}/cmake/Findallstat.cmake)

set(ALLSTAT_INCLUDE_DIR "${ALLSTAT_ROOT}/AllStat/include")

add_compile_definitions(USE_ALLSTAT)
include_directories(${ALLSTAT_INCLUDE_DIR})

if(TARGET allstat)
  set_target_properties(allstat PROPERTIES FOLDER "Dependencies")
endif()

message("Enable allstat usage")
message("ALLSTAT_INCLUDE_DIR=${ALLSTAT_INCLUDE_DIR}")

set(ALLSTAT_LIBRARIES allstat)

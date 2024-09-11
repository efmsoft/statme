macro(FindLogme)
  set(folder ${CMAKE_CURRENT_SOURCE_DIR})

  while(TRUE)
    message("Processing ${folder}, project root ${CMAKE_SOURCE_DIR}")

    if(EXISTS ${folder}/logme/logme)
      message("Found logme library: ${folder}/logme")
      set(LOGME_ROOT ${folder}/logme)
      break()
    endif()

    # logme is checked out from git to bin dir and it is part of policy endgine (for example)
    if(EXISTS ${folder}/bin/logme/logme)
      message("Found logme library: ${folder}/bin/logme")
      set(LOGME_ROOT ${folder}/bin/logme)
      break()
    endif()

    if(EXISTS ${folder}/Modules/logme/logme)
      message("Found logme library: ${folder}/Modules/logme")
      set(LOGME_ROOT ${folder}/Modules/logme)
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

set(USE_LOGME OFF)
set(LOGME_ROOT "")
if(USE_LOGME_SHARED)
  set(LOGME_TARGET logmed)
  set(LOGME_CMAKE_SUBDIR dynamic)
else()
  set(LOGME_TARGET logme)
  set(LOGME_CMAKE_SUBDIR logme)
endif()

if(ENABLE_LOGME)
  if(NOT FORCE_FETCH)
    FindLogme()
  endif()

  if(LOGME_ROOT STREQUAL "")
    set(LOGME_ROOT "${CMAKE_SOURCE_DIR}/bin/logme")
  endif()
  set(LOGME_INCLUDE_DIR "${LOGME_ROOT}/logme/include")
  add_compile_definitions(USE_LOGME)
  include_directories(${LOGME_INCLUDE_DIR})

  message("Enable logme usage")
  message("LOGME_INCLUDE_DIR=${LOGME_INCLUDE_DIR}")

  if(NOT EXISTS ${LOGME_ROOT})
    include(FetchContent)
    FetchContent_Declare(logme
      GIT_REPOSITORY "https://github.com/efmsoft/logme.git"
      GIT_TAG "main"
      SOURCE_DIR "${LOGME_ROOT}"
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
    )
    FetchContent_GetProperties(logme)
    if(NOT logme_POPULATED)
      FetchContent_Populate(logme)
    endif()
  endif()
endif()

if(LOGME_ROOT STREQUAL "")
  message(FATAL_ERROR "Cannot find logme root.")
endif()

set(USE_LOGME ON)

include(${LOGME_ROOT}/cmake/Findlogme.cmake)

if(TARGET ${LOGME_TARGET})
  set_target_properties(${LOGME_TARGET} PROPERTIES FOLDER "Dependencies")
endif()

set(LOGME_LIBRARIES ${LOGME_TARGET})

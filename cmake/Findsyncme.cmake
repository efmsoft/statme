macro(FindSyncme)
  set(folder ${CMAKE_CURRENT_SOURCE_DIR})

  while(TRUE)
    message("Processing ${folder}, project root ${CMAKE_SOURCE_DIR}")

    if(EXISTS ${folder}/syncme/lib/include/Syncme/Sync.h)
      message("Found syncme library: ${folder}/syncme")
      set(SYNCME_ROOT ${folder}/syncme)
      break()
    endif()

    # syncme is checked out from git to bin dir and it is part of policy endgine (for example)
    if(EXISTS ${folder}/bin/syncme/lib/include/Syncme/Sync.h)
      message("Found syncme library: ${folder}/bin/syncme")
      set(SYNCME_ROOT ${folder}/bin/syncme)
      break()
    endif()

    if(EXISTS ${folder}/Modules/syncme/lib/include/Syncme/Sync.h)
      message("Found syncme library: ${folder}/Modules/syncme")
      set(SYNCME_ROOT ${folder}/Modules/syncme)
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

set(SYNCME_ROOT "")
FindSyncme()

if(SYNCME_ROOT STREQUAL "")
  if(NOT FORCE_FETCH)
    message(FATAL_ERROR "Internal error:FORCE_FETCH disabled but syncme is not found.")
  endif()

  set(SYNCME_ROOT "${CMAKE_SOURCE_DIR}/bin/syncme")
  if(NOT EXISTS SYNCME_ROOT)
    include(FetchContent)
    FetchContent_Declare(syncme
      GIT_REPOSITORY "https://github.com/efmsoft/syncme.git"
      GIT_TAG "main"
      SOURCE_SUBDIR syncme
      SOURCE_DIR "${SYNCME_ROOT}"
    )
    FetchContent_MakeAvailable(syncme)
  endif()
endif()

set(SYNCME_INCLUDE_DIR "${SYNCME_ROOT}/lib/include")

add_compile_definitions(USE_SYNCME)
include(${SYNCME_ROOT}/cmake/Findsyncme.cmake)

include_directories(${SYNCME_INCLUDE_DIR})

if(TARGET syncme)
  set_target_properties(syncme PROPERTIES FOLDER "Dependencies")
endif()

message("Enable syncme usage")
message("SYNCME_INCLUDE_DIR=${SYNCME_INCLUDE_DIR}")

set(SYNCME_LIBRARIES syncme) 
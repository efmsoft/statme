cmake_minimum_required(VERSION 3.0.0)
project(statme VERSION 1.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

cmake_policy(SET CMP0077 NEW)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE) 

if (CMAKE_BUILD_TYPE STREQUAL "")
  message(STATUS "  Diag: Build type was unspecified, set to Release")
  set(CMAKE_BUILD_TYPE Release)
else ()
  message(STATUS "  Diag: Build type specified as '${CMAKE_BUILD_TYPE}'")
endif ()

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
  set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "_DEBUG")
endif ()

# Set to ON if we should clone dependencies in a temporary folder
#
# In case of OFF we are trying to find dependencies in the parent folders. It can work much
# faster if all submodules of a big project placed to the same directory (for example, Modules
# subdirectory of the project). In this case we will have directory structure like this
# / 
#   Modules
#          allstat
#          logme
#          syncme
#              CMakeLists.txt   - this file
# 
# FindXXX.cmake will locate dependency. But it can cause problems in case of version conflict
set(FORCE_FETCH ON)

set(ENABLE_ALLSTAT ON)
set(ENABLE_LOGME ON)

add_compile_definitions(LOGME_INRELEASE)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(USE_SYNCME_SHARED ON)
set(USE_LOGME_SHARED ON)

find_package(allstat MODULE)
find_package(logme MODULE)
find_package(OpenSSL REQUIRED) 
find_package(syncme MODULE)
find_package(jsoncpp MODULE)

enable_testing()
add_subdirectory(lib ${CMAKE_SOURCE_DIR}/bin/Static)
add_subdirectory(tests)

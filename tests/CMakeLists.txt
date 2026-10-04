project(statme_tests)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

cmake_policy(SET CMP0077 NEW)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(SOURCES
  header_tests.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
  gtest_main
  statmed
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} DISCOVERY_MODE PRE_TEST)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")

set_target_properties(gmock PROPERTIES FOLDER "Tests/gtest")
set_target_properties(gmock_main PROPERTIES FOLDER "Tests/gtest")
set_target_properties(gtest PROPERTIES FOLDER "Tests/gtest")
set_target_properties(gtest_main PROPERTIES FOLDER "Tests/gtest")


# The allocation tests count the heap allocations of the code under test by
# replacing the global operator new, which a shared library does not show (it
# has its own operator new on Windows). So the sources are compiled into the
# test executable itself instead of linking statmed.
add_executable(statme_alloc_tests
  header_alloc_tests.cpp
  ../lib/source/http/Headers.cpp
  ../lib/source/http/ReqHeaders.cpp
  ../lib/source/http/ResHeaders.cpp
  ../lib/source/http/Find2CRLF.cpp
  ../lib/source/http/Version.cpp
  ../lib/source/http/StreamData.cpp
)

target_compile_definitions(statme_alloc_tests PRIVATE _STATME_STATIC_BUILD_)
target_include_directories(statme_alloc_tests PRIVATE ../lib/include)
target_link_libraries(statme_alloc_tests gtest_main)

gtest_discover_tests(statme_alloc_tests DISCOVERY_MODE PRE_TEST)

set_target_properties(statme_alloc_tests PROPERTIES FOLDER "Tests")

# Minimum required CMake version.
cmake_minimum_required (VERSION 3.8)
# Project name and settings.
project(egolib-tests CXX)
message("building Egolib Tests Executable")
set_project_default_properties()

# Add include directory of Google Test.
include_directories("${IDLIB_PATH}/external/googletest/googletest/include")

# Include directory locations.
include_directories(${PROJECT_SOURCE_DIR}/../library/src)
include_directories(${PROJECT_SOURCE_DIR})

# Build a list of all tests.
file(GLOB_RECURSE test_files ${PROJECT_SOURCE_DIR}/egolib/tests/*.cpp)

add_executable(egolib-tests-executable ${test_files})
target_link_libraries(egolib-tests-executable egolib-library)
target_link_libraries(egolib-tests-executable gtest gtest_main)

include(CTest)
enable_testing()
add_test(egolib-tests ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/egolib-tests-executable)
if (NOT MSVC)
  add_custom_target(egolib-check COMMAND ${CMAKE_CTEST_COMMAND} --verbose DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/egolib-tests-executable)
  if (NOT TARGET check)
	add_custom_target(check)
  endif()
  add_dependencies(check egolib-check)
endif()

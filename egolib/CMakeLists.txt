# Minimum required CMake version.
cmake_minimum_required(VERSION 3.6)

# Project name and language.
project(egolib-library CXX)

# Project compiles against C++ 17.
if (NOT MSVC)
  set(CMAKE_CXX_STANDARD 17)
endif()

# Set the default properties.
set_project_default_properties()

set(SOURCE_FILES "")

# Include directories for project.
include_directories(${PROJECT_SOURCE_DIR}/src)

# Enumerate cpp and c files.
file(GLOB_RECURSE CPP_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
list(APPEND SOURCE_FILES ${CPP_FILES})

file(GLOB_RECURSE C_FILES ${PROJECT_SOURCE_DIR}/src/*.c)
SET_SOURCE_FILES_PROPERTIES(${C_FILES} PROPERTIES LANGUAGE CXX)
list(APPEND SOURCE_FILES ${C_FILES})

# Exclude OS specific cpp and c files.
list(REMOVE_ITEM SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/egolib/Platform/file_win.c")
list(REMOVE_ITEM SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/egolib/Platform/file_linux.c")

# Enumerate hpp and h files.
file(GLOB_RECURSE HPP_FILES ${PROJECT_SOURCE_DIR}/src/*.hpp)
set_source_files_properties(${HPP_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${HPP_FILES} PROPERTIES LANGUAGE CXX)
list(APPEND SOURCE_FILES ${HPP_FILES})

file(GLOB_RECURSE H_FILES ${PROJECT_SOURCE_DIR}/src/*.h)
set_source_files_properties(${H_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${H_FILES} PROPERTIES LANGUAGE CXX)
list(APPEND SOURCE_FILES ${H_FILES})

# Add OS specific files.
if (UNIX AND NOT APPLE)
  set(SOURCE_FILES ${SOURCE_FILES} ${PROJECT_SOURCE_DIR}/src/egolib/Platform/file_linux.c)
endif()

if (APPLE)
  file(GLOB_RECURSE MM_FILES ${PROJECT_SOURCE_DIR}/src/*.mm)
  list(APPEND SOURCE_FILES ${MM_FILES})
  SET_SOURCE_FILES_PROPERTIES(${MM_FILES} PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

if (WIN32)
  set(SOURCE_FILES ${SOURCE_FILES} ${PROJECT_SOURCE_DIR}/src/egolib/Platform/file_win.c)
endif()

# Define product.
add_library(egolib-library STATIC ${SOURCE_FILES})
target_include_directories(egolib-library INTERFACE "${PROJECT_SOURCE_DIR}/src/")
target_link_libraries(egolib-library idlib-game-engine-library)
if (WIN32)
  target_link_libraries(egolib-library Shlwapi.lib)
endif()

# == Doxygen ==================================================================================== #

FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
    ADD_CUSTOM_TARGET(egolib-doc ${DOXYGEN_EXECUTABLE} COMMENT "Building documentation")
ELSE(DOXYGEN_FOUND)
    MESSAGE(STATUS "Doxygen not found. You won't be able to build documentation.")
ENDIF(DOXYGEN_FOUND)

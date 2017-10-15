# Minimum required CMake version.
cmake_minimum_required(VERSION 3.6)

# Project name and language.
project(egolib-library CXX)

# Set the default properties.
set_project_default_properties()

# Include directories for "egolib".
include_directories(${PROJECT_SOURCE_DIR}/src)

# Determine source files and compile them as C++.
#file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Compute source files.

# Exclude OS specific files.
file(GLOB_RECURSE CPP_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
#file(GLOB_RECURSE C_FILES ${PROJECT_SOURCE_DIR}/src/*.c)
set(SOURCE_FILES ${CPP_FILES} ${C_FILES})
list(REMOVE_ITEM SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/egolib/Platform/file_win.cpp")
list(REMOVE_ITEM SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/egolib/Platform/file_linux.cpp")

# Add OS specific files.
if (UNIX)
  set(SOURCE_FILES ${SOURCE_FILES} ${PROJECT_SOURCE_DIR}/src/egolib/Platform/file_linux.cpp)
endif()

if (APPLE)
  set(SOURCE_FILES ${SOURCE_FILES} ${PROJECT_SOURCE_DIR}/src/egolib/Platform/file_mac.m)
endif()

if (WIN32)
  set(SOURCE_FILES ${SOURCE_FILES} ${PROJECT_SOURCE_DIR}/src/egolib/Platform/file_win.cpp)
endif()

#message("SDL 2 include directories ${SDL2_INCLUDE_DIRS}")
#message("SDL 2 mixer include directories ${SDL2_MIXER_INCLUDE_DIRS}")
#message("SDL 2 ttf include directories ${SDL2_TTF_INCLUDE_DIRS}")
#message("SDL 2 image include directories ${SDL2_IMAGE_INCLUDE_DIRS}")

# Define executable.
add_library(egolib-library STATIC ${SOURCE_FILES})
# idlib.
include_directories(${CMAKE_BINARY_DIR}/idlib/library/src)
target_link_libraries(egolib-library idlib-library)
# physfs.
include_directories(${PHYSFS_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(egolib-library ${PHYSFS_LIBRARY})

include_directories(${PHYSFS_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(egolib-library ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_MIXER_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENGL_gl_LIBRARY})

# == Doxygen ==================================================================================== #

FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
    ADD_CUSTOM_TARGET(egolib-doc ${DOXYGEN_EXECUTABLE} COMMENT "Building documentation")
ELSE(DOXYGEN_FOUND)
    MESSAGE(STATUS "Doxygen not found. You won't be able to build documentation.")
ENDIF(DOXYGEN_FOUND)

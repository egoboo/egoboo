cmake_minimum_required(VERSION 3.8)
project(egoboo CXX)

#Enable ccache if it is available
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

#Set compile flags
set(CMAKE_CXX_STANDARD 17)
add_definitions("-std=c++17 -Wall -Wextra")
#add_definitions("-O3 -flto -fuse-ld=gold")
add_definitions(-ggdb3 -Og)

#Disable some warnings
add_definitions("-Wno-reorder -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter ")

#Include directory locations
INCLUDE_DIRECTORIES(
        game/src
        egolib/src
        idlib/src)

#TODO: Tests
set(EGOLIB_SOURCE_FILES
        #egolib/tests/egolib/Tests/Math/Closure.cpp
        #egolib/tests/egolib/Tests/Math/Constants.cpp
        #egolib/tests/egolib/Tests/Math/Contains.cpp
        #egolib/tests/egolib/Tests/Math/Interpolate_Linear.cpp
        #egolib/tests/egolib/Tests/Math/Intersects.cpp
        #egolib/tests/egolib/Tests/Math/MatrixMath.cpp
        #egolib/tests/egolib/Tests/Math/PointMath.cpp
        #egolib/tests/egolib/Tests/Math/Translate.cpp
        #egolib/tests/egolib/Tests/Math/VectorMath.cpp
        #egolib/tests/egolib/Tests/Compilation.cpp
        #egolib/tests/egolib/Tests/MeshInfoIterator.cpp
        #egolib/tests/egolib/Tests/QuadTree.cpp
        #egolib/tests/egolib/Tests/Singleton.cpp
        #egolib/tests/egolib/Tests/StringUtilities.cpp
        )

file(GLOB_RECURSE IDLIB_SOURCE_FILES idlib/src/idlib/*.cpp)
file(GLOB_RECURSE EGOLIB_SOURCE_FILES egolib/src/egolib/*.cpp egolib/src/egolib/*.c)
file(GLOB_RECURSE GAME_SOURCE_FILES game/src/game/*.cpp game/src/game/*.c)

#Exclude OS specific files, they are added later
list(REMOVE_ITEM EGOLIB_SOURCE_FILES "${PROJECT_SOURCE_DIR}/egolib/src/egolib/Platform/file_win.c")
list(REMOVE_ITEM EGOLIB_SOURCE_FILES "${PROJECT_SOURCE_DIR}/egolib/src/egolib/Platform/file_linux.c")

set(EGOBOO_SOURCE_FILES
        ${IDLIB_SOURCE_FILES}
        ${EGOLIB_SOURCE_FILES}
        ${GAME_SOURCE_FILES}
        )

#OS specific files
if (UNIX)
    set(EGOBOO_SOURCE_FILES ${EGOBOO_SOURCE_FILES} unix/main.cpp egolib/src/egolib/Platform/file_linux.c)
endif (UNIX)
if (APPLE)
    set(EGOBOO_SOURCE_FILES ${EGOBOO_SOURCE_FILES} egolib/src/egolib/Platform/file_mac.m)
endif (APPLE)
if (WINDOWS)
    set(EGOBOO_SOURCE_FILES ${EGOBOO_SOURCE_FILES} egolib/src/egolib/Platform/file_win.c)
endif (WINDOWS)

#Compile everything as C++
SET_SOURCE_FILES_PROPERTIES(${IDLIB_SOURCE_FILES} PROPERTIES LANGUAGE CXX )
SET_SOURCE_FILES_PROPERTIES(${EGOBLIB_SOURCE_FILES} PROPERTIES LANGUAGE CXX )
SET_SOURCE_FILES_PROPERTIES(${EGOBOO_SOURCE_FILES} PROPERTIES LANGUAGE CXX )

#Define compilation output
add_library(idlib STATIC ${IDLIB_SOURCE_FILES})
add_library(egolib STATIC ${EGOLIB_SOURCE_FILES})
add_executable(egoboo ${EGOBOO_SOURCE_FILES})

#Include dependency on SDL
INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2_IMAGE REQUIRED SDL2_image>=2.0.0)
PKG_SEARCH_MODULE(SDL2_MIXER REQUIRED SDL2_mixer>=2.0.0)
PKG_SEARCH_MODULE(SDL2_TTF REQUIRED SDL2_ttf>=2.0.0)
#PKG_SEARCH_MODULE(PHYSFS REQUIRED PHYSFS>=2.0.0)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(PhysFS REQUIRED)

INCLUDE_DIRECTORIES(${PHYSFS_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_MIXER_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENGL_gl_LIBRARY})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PHYSFS_LIBRARY})

# Find Google test framework
add_subdirectory("${CMAKE_SOURCE_DIR}/external/googletest/googletest/")
enable_testing()
include_directories(${GTEST_INCLUDE_DIRS})

#Also build tests
add_subdirectory(idlib/tests)
add_subdirectory(egolib/tests)

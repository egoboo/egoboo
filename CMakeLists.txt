cmake_minimum_required(VERSION 3.6)

project(egoboo)

# Add Idlib.
add_subdirectory(idlib)

include(${CMAKE_CURRENT_SOURCE_DIR}/idlib/buildsystem/set_project_default_properties.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/idlib-game-engine/buildsystem/download_and_unpack.cmake)

# Add Idlib: Game Engine.
set(idlib-with-tests OFF CACHE BOOL "enable/disable compilation of tests. ON enables compilation of tests, OFF disables compilation of tests. Initial value is OFF." FORCE)
set(IDLIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/idlib)
add_subdirectory(idlib-game-engine)

# Add Egolib.
add_subdirectory(egolib)

# Add Egoboo.
add_subdirectory(egoboo)

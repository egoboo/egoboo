# Minimum required CMake version.
cmake_minimum_required(VERSION 3.8)

# Project name and language.
project(egoboo CXX)

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

# Enumerate hpp and h files.
file(GLOB_RECURSE HPP_FILES ${PROJECT_SOURCE_DIR}/src/*.hpp)
set_source_files_properties(${HPP_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${HPP_FILES} PROPERTIES LANGUAGE CXX)
list(APPEND SOURCE_FILES ${HPP_FILES})

file(GLOB_RECURSE H_FILES ${PROJECT_SOURCE_DIR}/src/*.h)
set_source_files_properties(${H_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${H_FILES} PROPERTIES LANGUAGE CXX)
list(APPEND SOURCE_FILES ${H_FILES})

# Define product.
add_executable(egoboo ${SOURCE_FILES})

# Link libraries.
target_link_libraries(egoboo egolib-library)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  get_property(runtime-libraries TARGET idlib-game-engine-library PROPERTY runtime-libraries)
  foreach( runtime-library ${runtime-libraries} )
    get_filename_component(barename ${runtime-library} NAME)
    message("copying ${runtime-library} to $<TARGET_FILE_DIR:egoboo>/${barename}")
    add_custom_command(TARGET egoboo
                       PRE_LINK
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different ${runtime-library} $<TARGET_FILE_DIR:egoboo>/${barename})
	set_target_properties(egoboo PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
  endforeach()
endif()

# == Doxygen ==================================================================================== #

FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
    ADD_CUSTOM_TARGET(egoboo-doc ${DOXYGEN_EXECUTABLE} COMMENT "Building documentation")
ELSE(DOXYGEN_FOUND)
    MESSAGE(STATUS "Doxygen not found. You won't be able to build documentation.")
ENDIF(DOXYGEN_FOUND)

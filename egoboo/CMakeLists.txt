# Minimum required CMake version.
cmake_minimum_required(VERSION 3.8)

# Project name and settings.
project(egoboo CXX)
message("building Egoboo Executable")
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

# Copy runtime dependencies (e.g. SDL2 DLLs) of idlib-game-engine-library to the executable.
if (${IDLIB_CXX_COMPILER_ID} EQUAL ${IDLIB_CXX_COMPILER_ID_MSVC})
  get_property(runtime-libraries TARGET idlib-game-engine-library PROPERTY runtime-libraries)
  foreach( runtime-library ${runtime-libraries} )
    get_filename_component(barename ${runtime-library} NAME)
    #message("copying ${runtime-library} to $<TARGET_FILE_DIR:egoboo>/${barename}")
    add_custom_command(TARGET egoboo
                       PRE_LINK
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different ${runtime-library} $<TARGET_FILE_DIR:egoboo>/${barename})
  endforeach()
  set_target_properties(egoboo PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
endif()

# Install runtime dependencies (e.g. SDL2 DLLs) of idlib-game-engine-library with the executable.
if (${IDLIB_CXX_COMPILER_ID} EQUAL ${IDLIB_CXX_COMPILER_ID_MSVC})
  get_property(runtime-libraries TARGET idlib-game-engine-library PROPERTY runtime-libraries)
  foreach( runtime-library ${runtime-libraries} )
	install(FILES ${runtime-library}
			DESTINATION bin
			COMPONENT applications)
  endforeach()
endif()

IF(DOXYGEN_FOUND)
    ADD_CUSTOM_TARGET(egoboo-doc ${DOXYGEN_EXECUTABLE} COMMENT "Building documentation")
ENDIF(DOXYGEN_FOUND)

install(TARGETS egoboo
        RUNTIME
        DESTINATION bin
        COMPONENT applications)

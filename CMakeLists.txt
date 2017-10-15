cmake_minimum_required(VERSION 3.6)

project(egoboo)

macro(set_project_default_properties)
	# (1) GNU GCC specific settings
	# TODO: Raise an error if the MSVC version is too low.
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
	  message("compiler is GNUCC")
	  # Set standard to C++ 17, enable -Wall and -Wextra
	  add_definitions("-std=gnu++17 -Wall -Wextra -D_GNU_SOURCE")
	  # Disable some warnings
	  # TODO: IdLib can have these warnings enabled.
	  add_definitions("-Wno-reorder -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter")
	  # Enable optimizations that do not interfere with debug experience.
	  add_definitions("-Og")
	  # Enable extra debug information.
	  add_definitions("-ggdb3")
	endif()

	# (2) MSVC specific settings
	# TODO: Raise an error if the MSVC version is too low.
	if (MSVC_VERSION GREATER_EQUAL "1900")
		include(CheckCXXCompilerFlag)
		CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
		if (_cpp_latest_flag_supported)
			add_compile_options("/std:c++latest")
		endif()
	endif()

	# (2) MSVC specific settings
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	  message("compiler is MSVC")
	  set(variables
		  CMAKE_C_FLAGS_DEBUG
		  CMAKE_C_FLAGS_MINSIZEREL
		  CMAKE_C_FLAGS_RELEASE
		  CMAKE_C_FLAGS_RELWITHDEBINFO
		  CMAKE_CXX_FLAGS_DEBUG
		  CMAKE_CXX_FLAGS_MINSIZEREL
		  CMAKE_CXX_FLAGS_RELEASE
		  CMAKE_CXX_FLAGS_RELWITHDEBINFO)
	  foreach(variable ${variables})
		if(${variable} MATCHES "/MD")
		  string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
		endif()
	  endforeach()
	  set(MSVC_RUNTIME "static")
	endif()

	# (3) MSVC specific settings
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
	endif()
endmacro(set_project_default_properties)

#Enable ccache if it is available
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

function(download_and_unpack remote local unpack)
	#message("downloading ${remote} to ${local}")
	# @todo Error handling.
	file(DOWNLOAD ${remote} ${local}
		 STATUS status
		 LOG log)
	#message("unpacking ${local} to ${unpack}")
	# @todo Error handling.
	file(MAKE_DIRECTORY ${unpack})
	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xfz ${local}
                    WORKING_DIRECTORY ${unpack}
                    RESULT_VARIABLE status)
endfunction()

####################################################################################################################
# If on Windows and not Cygwin, download SDL 2, SDL 2 mixer, SDL 2 ttf, and SDL 2 image.

# SDL 2.
download_and_unpack("https://www.libsdl.org/release/SDL2-devel-2.0.6-VC.zip"
                    "${CMAKE_BINARY_DIR}/SDL2-devel-2.0.6-VC.zip"
					"${CMAKE_BINARY_DIR}/SDL2-devel-2.0.6-VC")

# SDL 2 mixer.
download_and_unpack("https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.1-VC.zip"
                    "${CMAKE_BINARY_DIR}/SDL2_mixer-devel-2.0.1-VC.zip"
                    "${CMAKE_BINARY_DIR}/SDL2_mixer-devel-2.0.1-VC")

# SDL 2 image.
download_and_unpack("https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-2.0.1-VC.zip"
                    "${CMAKE_BINARY_DIR}/SDL2_image-devel-2.0.1-VC.zip"
 					 "${CMAKE_BINARY_DIR}/SDL2_image-devel-2.0.1-VC")
	 
# SDL 2 ttf.
download_and_unpack("https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-2.0.14-VC.zip"
                    "${CMAKE_BINARY_DIR}/SDL2_ttf-devel-2.0.14-VC.zip"
					"${CMAKE_BINARY_DIR}/SDL2_ttf-devel-2.0.14-VC")

####################################################################################################################
# Download `physfs-3.0.0.tar.bz2`, unpack contents to `${CMAKE_BINARY_DIR}/physfs-3.0.0`, configure and build.

download_and_unpack("https://icculus.org/physfs/downloads/physfs-3.0.0.tar.bz2"
                    "${CMAKE_BINARY_DIR}/physfs-3.0.0.tar.bz2"
					"${CMAKE_BINARY_DIR}/physfs-3.0.0")

# Build as static library.
option(PHYSFS_BUILD_STATIC "" TRUE)
add_subdirectory(physfs-3.0.0)
# Store paths and library.
set(PHYSFS_LIBRARY physfs)
set(PHYSFS_INCLUDE_DIR ${CMAKE_BINARY_DIR}/physfs-3.0.0/src)

####################################################################################################################

# Threads" and "OpenGL" work both on Windows.
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)

# The following modules do not work on MSVC.
if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNUCC)
#Include dependency on SDL
INCLUDE(FindPkgConfig)
	PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
	PKG_SEARCH_MODULE(SDL2_IMAGE REQUIRED SDL2_image>=2.0.0)
	PKG_SEARCH_MODULE(SDL2_MIXER REQUIRED SDL2_mixer>=2.0.0)
	PKG_SEARCH_MODULE(SDL2_TTF REQUIRED SDL2_ttf>=2.0.0)
endif()
#
if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	#
	set(SDL2_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/SDL2-devel-2.0.6-VC/SDL2-2.0.6/include)
	message("SDL 2 include directories ${SDL2_INCLUDE_DIRS}")
	set(SDL2_LIBRARIES ${CMAKE_BINARY_DIR}/SDL2-devel-2.0.6-VC/SDL2-2.0.6/lib/x86/SDL2.lib)
	message("SDL 2 libraries ${SDL2_LIBRARIES}")
	#
	set(SDL2_MIXER_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/SDL2_mixer-devel-2.0.1-VC/SDL2_mixer-2.0.1/include)
	set(SDL2_MIXER_LIBRARIES ${CMAKE_BINARY_DIR}/SDL2_mixer-devel-2.0.1-VC/SDL2_mixer-2.0.1/lib/x86/SDL2_mixer.lib)
	message("SDL 2 mixer libraries ${SDL2_MIXER_LIBRARIES}")
	#
	set(SDL2_TTF_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/SDL2_ttf-devel-2.0.14-VC/SDL2_ttf-2.0.14/include)
	message("SDL 2 ttf include directories ${SDL2_TTF_INCLUDE_DIRS}")
	set(SDL2_TTF_LIBRARIES ${CMAKE_BINARY_DIR}/SDL2_ttf-devel-2.0.14-VC/SDL2_ttf-2.0.14/lib/x86/SDL2_ttf.lib)
	message("SDL 2 ttf libraries ${SDL2_TTF_LIBRARIES}")
	#
	set(SDL2_IMAGE_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/SDL2_image-devel-2.0.1-VC/SDL2_image-2.0.1/include)
	message("SDL 2 image include directories ${SDL2_IMAGE_INCLUDE_DIRS}")
	set(SDL2_IMAGE_LIBRARIES ${CMAKE_BINARY_DIR}/SDL2_image-devel-2.0.1-VC/SDL2_image-2.0.1/lib/x86/SDL2_image.lib)
	message("SDL 2 image librarie ${SDL2_IMAGE_LIBRARIES}")
endif()

####################################################################################################################

# Add idlib.
add_subdirectory(idlib)

# Add egolib.
add_subdirectory(egolib)

# Add egoboo.
add_subdirectory(game)

cmake_minimum_required(VERSION 3.6)

project(egoboo)

# Set path to Idlib.
set(IDLIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/idlib)

# Set path to Idlib: Game Engine.
set(IDLIB_GAME_ENGINE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/idlib-game-engine)

# Add Idlib.
set(idlib-with-tests OFF CACHE BOOL
    "enable/disable compilation of tests. ON enables compilation of tests, OFF disables compilation of tests. Initial value is OFF." FORCE)
set(idlib-with-demos OFF CACHE BOOL
    "enable/disable compilation of demos. ON enables compilation of demos, OFF disables compilation of demos. Initial value is OFF." FORCE)
set(idlib-with-documentation OFF CACHE BOOL
    "enable/disable compilation of documentation. ON enables compilation of documentation, OFF disables compilation of documentation. Initial value is OFF." FORCE)
add_subdirectory(${IDLIB_PATH})

# Add Idlib: Game Engine.
set(idlib-game-engine-with-tests OFF CACHE BOOL
    "enable/disable compilation of tests. ON enables compilation of tests, OFF disables compilation of tests. Initial value is OFF." FORCE)
set(idlib-game-engine-with-demos OFF CACHE BOOL
    "enable/disable compilation of demos. ON enables compilation of demos, OFF disables compilation of demos. Initial value is OFF." FORCE)
add_subdirectory(${IDLIB_GAME_ENGINE_PATH})

include(${IDLIB_PATH}/buildsystem/set_project_default_properties.cmake)
include(${IDLIB_GAME_ENGINE_PATH}/buildsystem/download_and_unpack.cmake)

# Add Egolib.
add_subdirectory(egolib)

# Add Egoboo.
add_subdirectory(egoboo)

# Define installer (for MSCV only atm)
if (${IDLIB_CXX_COMPILER_ID} EQUAL ${IDLIB_CXX_COMPILER_ID_MSVC})
	set(CPACK_PACKAGE_NAME "Egoboo")
	set(CPACK_PACKAGE_VENDOR "https://egoboo.org/")
	if (${IDLIB_PLATFORM_ID} EQUAL ${IDLIB_PLATFORM_ID_X64})
	  #message("building Windows 10 x64 installer")
	  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Egoboo - Windows 10 x64 Installer")
	else()
	  #message("building Windows 10 x86 installer")
	  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Egoboo - Windows 10 x86 Installer")
	endif()
	set(CPACK_PACKAGE_VERSION "1.0.0")
	set(CPACK_PACKAGE_VERSION_MAJOR "1")
	set(CPACK_PACKAGE_VERSION_MINOR "0")
	set(CPACK_PACKAGE_VERSION_PATCH "0")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "Egoboo")
    
	set(CPACK_COMPONENTS_ALL applications libraries headers)
endif()

# This must always be last!
include(CPack)

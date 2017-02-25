//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/platform.h
/// @brief System-dependent global parameters.
///   @todo  some of this stuff is compiler dependent, rather than system dependent.

#pragma once

#include "idlib/idlib.hpp"

#if defined(ID_IOS) || defined(ID_IOSSIMULATOR)
    #error iOS or iOS simulator are not yet supported
#endif

//--------------------------------------------------------------------------------------------
// SDL.
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>


#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// OSX definitions

#if defined(ID_OSX)
    /// Make this function work cross-platform.
    #define stricmp strcasecmp
#endif

#if defined(ID_LINUX)
    /// Make this function work cross-platform.
    /// @todo Detect POSIX version if this function is available at all.
    #define stricmp strcasecmp
#endif

//--------------------------------------------------------------------------------------------
// windows definitions

#if defined(ID_WINDOWS)
    /// Speeds up compile times a bit.  We don't need everything in windows.h.
    /// @todo MH: Nice, except of that system headers like windows.h should not
    ///       be included in general code at all.
    #define WIN32_LEAN_AND_MEAN
    #define VC_EXTRALEAN

    /// Special win32 macro that lets windows know that you are going to be
    /// starting from a console. This is useful because you can get real-time
    /// output to the screen just by using printf()!
    #if defined(_CONSOLE)
        #define CONSOLE_MODE
    #else
        #undef CONSOLE_MODE
    #endif

#endif

//--------------------------------------------------------------------------------------------

// os-dependent pathname conventions

#define C_SLASH_CHR     '/'
#define C_SLASH_STR     "/"

#define C_BACKSLASH_CHR '\\'
#define C_BACKSLASH_STR "\\"

#define WIN32_SLASH_CHR C_BACKSLASH_CHR
#define WIN32_SLASH_STR C_BACKSLASH_STR

// everyone uses the same convention for the internet...
#define NETWORK_SLASH_CHR C_SLASH_CHR
#define NETWORK_SLASH_STR C_SLASH_STR

#define NET_SLASH_CHR NETWORK_SLASH_CHR
#define NET_SLASH_STR NETWORK_SLASH_STR

#if defined(ID_WINDOWS)
    #define SYSTEM_SLASH_STR WIN32_SLASH_STR
    #define SYSTEM_SLASH_CHR WIN32_SLASH_CHR
#else
    #define SYSTEM_SLASH_STR NET_SLASH_STR
    #define SYSTEM_SLASH_CHR NET_SLASH_CHR
#endif

#define SLASH_STR SYSTEM_SLASH_STR
#define SLASH_CHR SYSTEM_SLASH_CHR


//--------------------------------------------------------------------------------------------

// Compiler-specific definitions

// MSVC does not support noexcept yet.
#if defined(_MSC_VER)
    #define EGO_NOEXCEPT throw()
#else
    #define EGO_NOEXCEPT noexcept
#endif

//--------------------------------------------------------------------------------------------
// Turn off warnings that we don't care about.
/// @todo MH: This should be reviewed.

#if defined(_MSC_VER)
    #pragma warning(disable : 4090) ///< '=' : different 'const' qualifiers (totally unimportant in C)
    #pragma warning(disable : 4200) ///< zero-sized array in struct/union (used in the md2 loader)
    #pragma warning(disable : 4201) ///< nameless struct/union (nameless unions and nameless structs used in defining the vector structs)
    #pragma warning(disable : 4204) ///< non-constant aggregate initializer (used to simplify some vector initializations)
    #pragma warning(disable : 4244) ///< conversion from 'double' to 'float'
    #pragma warning(disable : 4305) ///< truncation from 'double' to 'float'

    #if !defined(_DEBUG)
        #pragma warning(disable : 4554) ///< possibly operator precendence error
    #endif
#endif

//--------------------------------------------------------------------------------------------

#if !defined(SET_PACKED)
    // Set the packing of a data structure at declaration time.
    #if !defined(USE_PACKING)
        // Do not actually do anything about the packing.
        #define SET_PACKED()
    #else
    // Use compiler-specific macro definitions.
    #if defined(__GNUC__)
        #define SET_PACKED() __attribute__ ((__packed__))
    #elif defined(_MSC_VER)
        #define SET_PACKED()
    #endif
#endif
#endif

//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------

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

#if defined(__cplusplus)
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#if defined(__cplusplus)
#include <cstddef>
#else
#include <stddef.h>
#endif

#if defined(__cplusplus)
#include <cctype>
#else
#include <ctype.h>
#endif

#if defined(__cplusplus)
#include <cstdarg>
#else
#include <stdarg.h>
#endif

#if defined(__cplusplus)
#include <cstdio>
#else
#include <stdio.h>
#endif

#if defined(__cplusplus)
#include <cassert>
#else
#include <assert.h>
#endif

#if defined(__cplusplus)
#include <cmath>
#else
#include <math.h>
#endif

#if defined(__cplusplus)
#include <cfloat>
#else
#include <float.h>
#endif

#if defined(__cplusplus)
#include <ctime>
#else
#include <time.h>
#endif

#if defined(__cplusplus)
#include <memory>
#else
#include <memory.h>
#endif

#if defined(__cplusplus)
#include <cstring>
#else
#include <string.h>
#endif

#if defined(__cplusplus)
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#if defined(__cplusplus)
#include <cerrno>
#else
#include <errno.h>
#endif

#if defined(__cplusplus)
#include <cstdint>
#else
#include <stdint.h>
#endif

//--------------------------------------------------------------------------------------------
// SDL.
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>

//--------------------------------------------------------------------------------------------
// Exclusive C++ headers from here on (in alphabetic order).
#include <array>
#include <algorithm>
#include <atomic>
#include <bitset>
#include <exception>
#include <forward_list>
#include <functional>
#include <iomanip>
#include <iostream>
#include <locale>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <random>
#include <stdexcept>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// Define __EGO_CURRENT_FILE__, __EGO_CURRENT_LINE__ and __EGO_CURRENT_FUNCTION__.
/// Those constants will either be properly defined or not at all.
#include "egolib/Platform/CurrentFunction.inline"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// OSX definitions

#if defined(__APPLE__) || defined(macintosh)

    // Trap non-OSX Mac builds.
    #if !defined(__MACH__)
        #error Only OS X builds are supported
    #endif

    /// Make this function work cross-platform.
    #define stricmp strcasecmp

#endif

//--------------------------------------------------------------------------------------------
// windows definitions

#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64) || defined(__WIN64)
    // Map all of these possibilities to WIN32.
    #if !defined(WIN32)
        #define WIN32
    #endif

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
// *nix definitions

#if defined(__unix__) || defined(__unix) || defined(_unix) || defined(unix)

    /// Map all of these possibilities __unix__.
    #if !defined(__unix__)
        #define __unix__
    #endif

    #include <unistd.h>

    /// Make this function work cross-platform.
    #define stricmp strcasecmp

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// os-dependent pathname conventions

#define C_SLASH_CHR     '/'
#define C_SLASH_STR     "/"

#define C_BACKSLASH_CHR '\\'
#define C_BACKSLASH_STR "\\"

#define WIN32_SLASH_CHR C_BACKSLASH_CHR
#define WIN32_SLASH_STR C_BACKSLASH_STR

// everyone uses the same convention for the internet...
#define NET_SLASH_CHR C_SLASH_CHR
#define NET_SLASH_STR C_SLASH_STR

#if defined(WIN32) || defined(_WIN32)
    #define SLASH_STR WIN32_SLASH_STR
    #define SLASH_CHR WIN32_SLASH_CHR
#else
    #define SLASH_STR NET_SLASH_STR
    #define SLASH_CHR NET_SLASH_CHR
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Compiler-specific definitions

// MSVC does not support constexpr yet.
#if defined(_MSC_VER)
	#define CONSTEXPR const
#else
    #define CONSTEXPR constexpr
#endif
// MSVC does not support noexcept yet.
#if defined(_MSC_VER)
    #define EGO_NOEXCEPT throw()
#else
    #define EGO_NOEXCEPT noexcept
#endif

// MSCV does not support usual format specifier for size_t (what does it actually support?).
#if defined(_MSC_VER)
    //// printf format specifier for size_t.
    #define PRIuZ "Iu"
    /// printf format specifier for ssize_t.
    #define PRIdZ "Id"
#else
    /// printf format specifier for size_t.
    #define PRIuZ "zu"
    /// printf format specifier for ssize_t.
    #define PRIdZ "zd"
#endif

//------------
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

// Fix the naming of some linux-flavored functions for MSVC.
#if defined(_MSC_VER)

    #define snprintf _snprintf
    #define stricmp  _stricmp

    // This isn't needed anymore since MSCV 2013 and causes errors.
    #if !(_MSC_VER >= 1800)
        #define isnan _isnan
    #endif

    #define strlwr   _strlwr
    #define strupr   _strupr

    /// This isn't needed anymore since MSVC 2008 and causes errors
    #if (_MSC_VER < 1500)
        #define vsnprintf _vsnprintf
    #endif
#endif

//------------
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
// Format attributes for GCC/Clang
#if defined(__GNUC__)
    #define GCC_PRINTF_FUNC(fmtargnum) __attribute__ (( format( __printf__, fmtargnum, fmtargnum+1 ) ))
    #define GCC_SCANF_FUNC(fmtargnum) __attribute__ (( format( __scanf__, fmtargnum, fmtargnum+1 ) ))
#else
    #define GCC_PRINTF_FUNC(fmtargnum)
    #define GCC_SCANF_FUNC(fmtargnum)
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

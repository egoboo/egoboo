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
// Fast file I/O. This is available under Windows, Linux and Mac OS X. 
#include <fcntl.h>

//--------------------------------------------------------------------------------------------
// Exclusive C++ headers from here on.
#include <array>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <list>
#include <forward_list>
#include <unordered_map>
#include <exception>
#include <stack>
#include <bitset>


#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// osx definitions

#if defined(__APPLE__) || defined(macintosh)
#define __OSX__
// trap non-osx mac builds
#    if !defined(__MACH__)
#        error Only OS X builds are supported
#    endif

/// make this function work cross-platform
#    define stricmp  strcasecmp

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// windows definitions

#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64) || defined(__WIN64)
#define __WINDOWS__
// map all of these possibilities to WIN32
#    if !defined(WIN32)
#        define WIN32
#    endif

/// Speeds up compile times a bit.  We don't need everything in windows.h
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

/// special win32 macro that lets windows know that you are going to be
/// starting from a console.  This is useful because you can get real-time
/// output to the screen just by using printf()!
#    if defined(_CONSOLE)
#        define CONSOLE_MODE
#    else
#        undef  CONSOLE_MODE
#    endif

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//*nix definitions
#if defined(__unix__) || defined(__unix) || defined(_unix) || defined(unix)
/// map all of these to __unix__
#    if !defined(__unix__)
#        define __unix__
#    endif

#    include <unistd.h>

/// make this function work cross-platform
#    define stricmp  strcasecmp

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

#    define SLASH_STR WIN32_SLASH_STR
#    define SLASH_CHR WIN32_SLASH_CHR

#else

#    define SLASH_STR NET_SLASH_STR
#    define SLASH_CHR NET_SLASH_CHR

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Compiler-specific definitions

//------------
// Fix how MSVC handles throw specifications on member functions.
#if defined(_MSC_VER)
	#define DECL_THROW(XX) throw(...)
#else
	#define DECL_THROW(XX) throw(XX)
#endif

// MSVC does not support constexpr yet.
#if defined(_MSC_VER)
	#define constexpr const
#endif

//------------
// Localize the inline keyword to the compiler.
#if defined(_MSC_VER)
	// In MSVC, the "inline" keyword seems to be depricated. Must to be promoted to "_inline" or "__inline".
	#define INLINE __inline
#else
	#define INLINE inline
#endif

//------------
// Turn off warnings that we don't care about.
#if defined(_MSC_VER)
#    pragma warning(disable : 4090) ///< '=' : different 'const' qualifiers (totally unimportant in C)
#    pragma warning(disable : 4200) ///< zero-sized array in struct/union (used in the md2 loader)
#    pragma warning(disable : 4201) ///< nameless struct/union (nameless unions and nameless structs used in defining the vector structs)
#    pragma warning(disable : 4204) ///< non-constant aggregate initializer (used to simplify some vector initializations)
#    pragma warning(disable : 4244) ///< conversion from 'double' to 'float'
#    pragma warning(disable : 4305) ///< truncation from 'double' to 'float'

#    if !defined(_DEBUG)
#        pragma warning(disable : 4554) ///< possibly operator precendence error
#    endif

#endif

//------------
// fix the naming of some linux-flavored functions in MSVC
#if defined(_MSC_VER)

#    define snprintf _snprintf
#    define stricmp  _stricmp

// This isnt needed in MSCV 2013 and causes errors
#    if !(_MSC_VER >= 1800)
#    define isnan    _isnan
#    endif
#    define strlwr   _strlwr
#    define strupr   _strupr

/// This isn't needed in MSVC 2008 and causes errors
#    if (_MSC_VER < 1500)
#        define vsnprintf _vsnprintf
#    endif

// something could be done about this, but it would require us
// using special names for the functions and defining
// the other option is to define _CRT_SECURE_NO_WARNINGS
//#if (_MSC_VER >= 1400 ) || !defined( __STDC_SECURE_LIB__ )
//#   define ego_strncpy strncpy_s
//#   define ego_strcat  strcat_s
//#   define ego_fopen   fopen_s
//#   define ego_sscanf  sscanf_s
//#endif

#endif

//------------
// it seems that the gcc community has a bug up its ass about the forward declaration of enums
// to get around this (so we can use the strong type checking of c++ to look for errors in the code)
// we will define
#if !defined(_MSC_VER)
#    define FWD_ENUM(XX) typedef int i_##XX
#else
#    define FWD_ENUM(XX) enum e_##XX; typedef enum e_##XX i_##XX;
#endif

//------------
#if !defined(SET_PACKED)
// set the packing of a data structure at declaration time
#    if !defined(USE_PACKING)
// do not actually do anything about the packing
#        define SET_PACKED()
#    else
// use compiler-specific macro definitions
#        if defined(__GNUC__)
#            define SET_PACKED() __attribute__ ((__packed__))
#        elif defined(_MSC_VER)
#            define SET_PACKED()
#        endif
#    endif
#endif

#if defined(__GNUC__)
#  if __STDC_VERSION__ < 199901L
#    if __GNUC__ >= 2
#        define __func__ __FUNCTION__
#    else
#        define __func__ "<unknown>"
#    endif
#  endif
#endif
    
//--------------------------------------------------------------------------------------------
// format attributes for GCC/Clang
#if defined(__GNUC__)
#   define GCC_PRINTF_FUNC( fmtargnum ) __attribute__ (( format( __printf__, fmtargnum, fmtargnum+1 ) ))
#   define GCC_SCANF_FUNC( fmtargnum ) __attribute__ (( format( __scanf__, fmtargnum, fmtargnum+1 ) ))
#else
#   define GCC_PRINTF_FUNC( fmtargnum )
#   define GCC_SCANF_FUNC( fmtargnum )
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

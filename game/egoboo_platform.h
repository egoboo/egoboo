#pragma once

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

/// @file egoboo_platform.h
/// @brief System-dependent global parameters.
///   @todo  some of this stuff is compiler dependent, rather than system dependent.

#include <SDL.h>  ///< use the basic SDL platform definitions

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// osx definitions

#if defined(__APPLE__) || defined(macintosh)

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

#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__)

// map all of these possibilities to WIN32
#    if !defined(WIN32)
#        define WIN32
#    endif

/// Speeds up compile times a bit.  We don't need everything in windows.h
#    undef WIN32_LEAN_AND_MEAN

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
//amiga definitions
#if defined(__amigaos4__)

/// amigaos uses __unix__
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
// deal with gcc's the warnings about const on return types in C

#define EGO_CONST

//------------
// fix how MSVC handles throw specifications on member functions
#if defined(_MSC_VER)
#    define DECL_THROW(XX) throw(...)
#else
#    define DECL_THROW(XX) throw(XX)
#endif

//------------
// localize the inline keyword to the compiler
#if defined(_MSC_VER)
/// In MS visual C, the "inline" keyword seems to be depricated. Must to be promoted to "_inline" or "__inline"
#    define INLINE __inline
#else
#    define INLINE inline
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
// fix the naming of some linux-flovored functions in MSVC
#if defined(_MSC_VER)
#    define snprintf _snprintf
#    define stricmp  _stricmp
#    define isnan    _isnan
#    define strlwr   _strlwr

/// This isn't needed in MSVC 2008 and causes errors
#    if _MSC_VER < 1500
#        define vsnprintf _vsnprintf
#    endif

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

#define EGOBOO_PLATFORM

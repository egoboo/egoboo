// system-dependent global parameters

// TODO : move more of the typical config stuff to this file
// TODO : add in linux and mac stuff
// TODO : some of this stuff is compiler dependent, rather than system dependent


#pragma once

#define EGOBOO_CONFIG

#include <SDL_endian.h>

#define FREE(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }

// localize the inline keyword to the compiler
#if defined(_MSC_VER)

    // In MS visual C, the "inline" keyword seems to be depreciated. Must to be promoted to "_inline" of "__inline"
#    define INLINE __inline  

    // Turn off warnings that we don't care about.

#    pragma warning(disable : 4090) // '=' : different 'const' qualifiers (totally unimportant in C)
#    pragma warning(disable : 4200) // zero-sized array in struct/union (used in the md2 loader)
#    pragma warning(disable : 4201) // nameless struct/union (nameless unions and nameless structs used in defining the vector structs)
#    pragma warning(disable : 4204) // non-constant aggregate initializer (used to simplify some vector initializations)
#    pragma warning(disable : 4244) // conversion from 'double' to 'float'
#    pragma warning(disable : 4305) // truncation from 'double' to 'float'

#    ifndef _DEBUG
#        pragma warning(disable : 4554) // possibly operator precendence error
#    endif

    // Visual C defines snprintf as _snprintf; that's kind of a pain, so redefine it here
#    define snprintf _snprintf

    // Visual C does not have native support for vsnprintf, it is implemented as _vsnprintf
#    define vsnprintf _vsnprintf

#else

#    define INLINE inline

#endif


#ifdef WIN32

    // Speeds up compile times a bit.  We don't need everything in windows.h
#    define WIN32_LEAN_AND_MEAN  

#endif

#undef DEBUG_ATTRIB
#undef DEBUG_NORMALS
#undef DEBUG_BBOX

#undef DEBUG_UPDATE_CLAMP
#define DEBUG_MESHFX
#undef DEBUG_CVOLUME

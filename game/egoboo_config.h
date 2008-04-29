// system-dependent global parameters

// TODO : move more of the typical config stuff to this file
// TODO : add in linux and mac stuff
// TODO : some of this stuff is compiler dependent, rather than system dependent


#pragma once

#include <SDL_endian.h>

#define FREE(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }

// localize the inline keyword to the compiler
#if defined(_MSC_VER)
#    define INLINE __inline  /* In MS visual C, the "inline" keyword seems to be depreciated. Must to be promoted to "_inline" of "__inline" */
#else
#    define INLINE inline
#endif


#ifdef WIN32

    // Speeds up compile times a bit.  We don't need everything in windows.h
#    define WIN32_LEAN_AND_MEAN  

    // Turn off warnings that we don't care about.

#    pragma warning(disable : 4305) // truncation from 'double' to 'float'
#    pragma warning(disable : 4244) // conversion from 'double' to 'float'

#    ifndef _DEBUG
#        pragma warning(disable : 4554) // possibly operator precendence error
#    endif

    // Windows defines snprintf as _snprintf; that's kind of a pain, so redefine it here
#    define snprintf _snprintf

    // Windows does not have native compatability with vsnprintf, it is implemented as _vsnprintf
#    define vsnprintf _vsnprintf

#endif


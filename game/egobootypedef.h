/* Egoboo - egobootypedef.h
 * Defines some basic types that are used throughout the game code.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef Egoboo_egobootypedef_h
#define Egoboo_egobootypedef_h

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN    // Speeds up compile times a bit.  We don't need everything
// in windows.h

// Turn off warnings that we don't care about.
#pragma warning(disable : 4305) // truncation from 'double' to 'float'
#pragma warning(disable : 4244) // conversion from 'double' to 'float'
#pragma warning(disable : 4554) // possibly operator precendence error
//#pragma warning(disable : 4761)

// Windows defines snprintf as _snprintf; that's kind of a pain, so redefine it here
#define snprintf _snprintf

#endif

#include <SDL.h>
#include <SDL_endian.h>

extern float LoadFloatByteswapped( float *ptr );

typedef struct rect_sint32_t
{
  Sint32 left;
  Sint32 right;
  Sint32 top;
  Sint32 bottom;
} IRect;

typedef struct rect_float_t
{
  float left;
  float right;
  float top;
  float bottom;
} FRect;


typedef enum bool_e
{
  btrue  = ( 1 == 1 ),
  bfalse = ( !btrue )
} bool_t;


typedef char STRING[256];

typedef Uint32 IDSZ;

#ifndef MAKE_IDSZ
#define MAKE_IDSZ(idsz) ((IDSZ)((((idsz)[0]-'A') << 15) | (((idsz)[1]-'A') << 10) | (((idsz)[2]-'A') << 5) | (((idsz)[3]-'A') << 0)))
#endif


typedef struct pair_t
{
  Sint32 ibase;
  Uint32 irand;
} PAIR;

typedef struct range_t
{
  float ffrom, fto;
} RANGE;

typedef Uint16 CHR_REF;
typedef Uint16 TEAM_REF;
typedef Uint16 PRT_REF;
typedef Uint16 PLA_REF;


#undef DEBUG_ATTRIB

#if defined(DEBUG_ATTRIB) && defined(_DEBUG)
#    define ATTRIB_PUSH(TXT, BITS)    { GLint xx=0; glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&xx); glPushAttrib(BITS); log_info("PUSH  ATTRIB: %s before attrib stack push. level == %d\n", TXT, xx); }
#    define ATTRIB_POP(TXT)           { GLint xx=0; glPopAttrib(); glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&xx); log_info("POP   ATTRIB: %s after attrib stack pop. level == %d\n", TXT, xx); }
#    define ATTRIB_GUARD_OPEN(XX)     { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&XX); log_info("OPEN ATTRIB_GUARD: before attrib stack push. level == %d\n", XX); }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&YY); if(XX!=YY) log_error("CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); else log_info("CLOSE ATTRIB_GUARD: after attrib stack pop. level == %d\n", XX); }
#elif defined(_DEBUG)
#    define ATTRIB_PUSH(TXT, BITS)    glPushAttrib(BITS);
#    define ATTRIB_POP(TXT)           glPopAttrib();
#    define ATTRIB_GUARD_OPEN(XX)     { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&XX);  }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&YY); assert(XX==YY); if(XX!=YY) log_error("CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY);  }
#else
#    define ATTRIB_PUSH(TXT, BITS)    glPushAttrib(BITS);
#    define ATTRIB_POP(TXT)           glPopAttrib();
#    define ATTRIB_GUARD_OPEN(XX)
#    define ATTRIB_GUARD_CLOSE(XX,YY)
#endif


#endif // include guard


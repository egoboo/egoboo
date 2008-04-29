/* Egoboo - graphic.h
 *
 *
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

#pragma once

#include <SDL.h>
#include "ogl_texture.h"
#include "Font.h"
#include "mesh.h"

#define TABAND              31                      // Tab size

#define TRANSCOLOR                      0           // Transparent color

// Global lighting stuff
extern float                   lightspek;
extern vect3                   lightspekdir;
extern vect3                   lightspekcol;
extern float                   lightambi;
extern vect3                   lightambicol;

typedef struct renderlist_t
{
  int     num_totl;                            // Number to render, total
  Uint32  totl[MAXMESHRENDER];                 // List of which to render, total

  int     num_shine;                           // ..., reflective
  Uint32  shine[MAXMESHRENDER];                // ..., reflective

  int     num_reflc;                           // ..., has reflection
  Uint32  reflc[MAXMESHRENDER];                // ..., has reflection

  int     num_norm;                            // ..., not reflective, has no reflection
  Uint32  norm[MAXMESHRENDER];                 // ..., not reflective, has no reflection

  int     num_watr;                            // ..., water
  Uint32  watr[MAXMESHRENDER];                 // ..., water
} RENDERLIST;

extern RENDERLIST renderlist;

//Setup values
typedef struct cursor_t
{
  int      x;              // Cursor position
  int      y;              //
  bool_t   pressed;        //
  bool_t   clicked;        //
  bool_t   pending;
} CURSOR;

extern CURSOR cursor;

// JF - Added so that the video mode might be determined outside of the graphics code
extern SDL_Surface *displaySurface;

int draw_string( BMFont * pfnt, float x, float y, GLfloat tint[], char * szFormat, ... );
bool_t draw_texture_box( GLtexture * ptx, FRect * tx_rect, FRect * sc_rect );

void BeginText( GLtexture * pfnt );
void EndText( void );

void Begin2DMode( void );
void End2DMode( void );


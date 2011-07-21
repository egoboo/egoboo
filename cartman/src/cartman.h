#pragma once

//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include <egolib.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_glTexture;
typedef struct s_glTexture glTexture;

struct s_ogl_surface;
typedef struct s_ogl_surface ogl_surface;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define NAME "Cartman"          // Program name
#define VERSION_STR "1.0.0"
#define YEAR 2011               // Year

#define MAXMESSAGE          6                       // Number of messages
#define TOTALMAXDYNA                    64          // Absolute max number of dynamic lights
#define TOTALMAXPRT             2048                // True max number of particles

#define MAXLIGHT 100
#define MAXRADIUS ( 500 * FOURNUM )
#define MINRADIUS ( 50 * FOURNUM )
#define MAP_MAXLEVEL 255
#define MAP_MINLEVEL 50

#define KEYDELAY 12             // Delay for keyboard
#define CAMRATE 8               // Arrow key movement rate

#define FADEBORDER 64           // Darkness at the edge of map

#define WINMODE_NOTHING 0           // Window display mode
#define WINMODE_TILE 1          //
#define WINMODE_VERTEX 2            //
#define WINMODE_SIDE 4          //
#define WINMODE_FX 8                //

//#define ONSIZE 600            // Max size of raise mesh
#define ONSIZE 264          // Max size of raise mesh

#define MAXPOINTS 20480         // Max number of points to draw

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern STRING egoboo_path;

extern int     timclock;

extern int     onscreen_count;
extern Uint32  onscreen_vert[MAXPOINTS];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void cartman_init_SDL_base();

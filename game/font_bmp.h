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

/// @file font_bmp.h
/// @details bitmapped font stuff

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define NUMFONTX            16          ///< Number of fonts in the bitmap
#define NUMFONTY            6
#define NUMFONT             (NUMFONTX*NUMFONTY)
#define FONTADD             4               ///< Gap between letters

#define TABADD              (1<<5)
#define TABAND              (~(TABADD-1))                      ///< Tab size

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern int      fontoffset;                 ///< Line up fonts from top of screen
extern SDL_Rect fontrect[NUMFONT];          ///< The font rectangles
extern Uint8    fontxspacing[NUMFONT];      ///< The spacing stuff
extern Uint8    fontyspacing;

extern Uint8    asciitofont[256];           ///< Conversion table

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void font_bmp_init();
void font_bmp_load_vfs( const char* szBitmap, const char* szSpacing );
int  font_bmp_length_of_word( const char *szText );

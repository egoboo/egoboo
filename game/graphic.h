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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************
#include "SDL_image.h"

/*Special Textures*/
typedef enum e_tx_type
{
    TX_PARTICLE = 0,
    TX_TILE_0,
    TX_TILE_1,
    TX_TILE_2,
    TX_TILE_3,
    TX_WATER_TOP,
    TX_WATER_LOW,
    TX_PHONG,
    TX_LAST
} TX_TYPE;

//Function prototypes
void draw_blip( float sizeFactor, Uint8 color, int x, int y );
void load_graphics();
bool_t load_all_global_icons();

void init_all_icons();
void init_all_titleimages();
void init_bars();
void init_blip();
void init_map();
void init_all_textures();
void init_all_models();

void release_all_icons();
void release_all_titleimages();
void release_bars();
void release_blip();
void release_map();
void release_all_textures();
void release_all_models();

bool_t load_blip_bitmap();
void   load_bars(  const char* szBitmap );
void   load_map(  const char* szModule );

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

/* Egoboo - passage.h
 * Passages and doors and whatnot.  Things that impede your progress!
 */

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_script_state;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXPASS             256                     // Maximum number of passages ( mul 32 )

#define NOOWNER 0xFFFF        // Shop has no owner
#define STOLEN  0xFFFF        // Someone stole a item

// Passages
extern int numpassage;              // Number of passages in the module
extern int passtlx[MAXPASS];          // Passage positions
extern int passtly[MAXPASS];
extern int passbrx[MAXPASS];
extern int passbry[MAXPASS];
extern int passagemusic[MAXPASS];        // Music track appointed to the specific passage
extern Uint8 passmask[MAXPASS];
extern Uint8 passopen[MAXPASS];      // Is the passage open?

// For shops
extern int numshoppassage;
extern Uint16  shoppassage[MAXPASS];  // The passage number
extern Uint16  shopowner[MAXPASS];    // Who gets the gold?

//--------------------------------------------------------------------------------------------
//Passage prototypes
int open_passage( Uint16 passage );
void check_passage_music();
int break_passage( struct s_script_state * pstate, Uint16 passage, Uint16 starttile, Uint16 frames,
                   Uint16 become, Uint8 meshfxor );
void flash_passage( Uint16 passage, Uint8 color );
Uint8 find_tile_in_passage( struct s_script_state * pstate, Uint16 passage, int tiletype );
Uint16 who_is_blocking_passage( Uint16 passage );
Uint16 who_is_blocking_passage_ID( Uint16 passage, IDSZ idsz );
int close_passage( Uint16 passage );
void clear_passages();
void add_shop_passage( Uint16 owner, Uint16 passage );
void add_passage( int tlx, int tly, int brx, int bry, Uint8 open, Uint8 mask );
void setup_passage( const char *modname );

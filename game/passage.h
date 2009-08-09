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
#define MAX_PASS             256                     // Maximum number of passages ( mul 32 )
#define CLOSETOLERANCE       2                       // For closing doors

#define NOOWNER 0xFFFF        // Shop has no owner
#define STOLEN  0xFFFF        // Someone stole a item
#define NO_MUSIC -1            // For passages that play no music

// These are shop orders
enum e_shop_orders
{
    SHOP_BUY       = 0,
    SHOP_SELL,
    SHOP_NOAFFORD,
    SHOP_THEFT,
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Passages

struct s_passage
{
    // Passage positions
    int    topleftx;             // top left X
    int    toplefty;             // top left Y
    int    bottomrightx;         // bottom right X
    int    bottomrighty;         // bottom right Y

    Sint8  music;                // Music track appointed to the specific passage
    Uint8  mask;                 // Is it IMPASSABLE, SLIPPERY or whatever
    bool_t open;                 // Is the passage open?
};

typedef struct s_passage passage_t;

DEFINE_STACK( extern, passage_t, PassageStack, MAX_PASS );

#define VALID_PASSAGE( IPASS )       ( ((IPASS) <= PassageStack.count) && ((IPASS) >= 0) )
#define INVALID_PASSAGE( IPASS )     ( ((IPASS) >  PassageStack.count) && ((IPASS) < 0) )

// For shops
struct s_shop
{
    Uint16  passage;  // The passage number
    Uint16  owner;    // Who gets the gold?
};
typedef struct s_shop shop_t;

DEFINE_STACK( extern, shop_t, ShopStack, MAX_PASS );

#define VALID_SHOP( IPASS )       ( ((IPASS) <= ShopStack.count) && ((IPASS) >= 0) )
#define INVALID_SHOP( IPASS )     ( ((IPASS) >  ShopStack.count) && ((IPASS) < 0) )


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// prototypes

bool_t open_passage( Uint16 passage );
bool_t close_passage( Uint16 passage );
void   check_passage_music();
bool_t break_passage( struct s_script_state * pstate, Uint16 passage, Uint16 starttile, Uint16 frames, Uint16 become, Uint8 meshfxor );
void   flash_passage( Uint16 passage, Uint8 color );
Uint8  find_tile_in_passage( struct s_script_state * pstate, Uint16 passage, int tiletype );
Uint16 who_is_blocking_passage( Uint16 passage, bool_t targetitems, bool_t targetdead, bool_t targetquest, 
							   bool_t requireitem, IDSZ findidsz );
void   clear_all_passages();
void   add_shop_passage( Uint16 owner, Uint16 passage );
void   add_passage( int tlx, int tly, int brx, int bry, bool_t open, Uint8 mask );
void   setup_passage( const char *modname );
Uint16 shop_get_owner( int ix, int iy );
bool_t is_in_passage( Uint16 passage, float xpos, float ypos, float tolerance );

Uint16 shop_get_owner( int ix, int iy );

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

/// @file game/passage.h
/// @Passages and doors and whatnot.  Things that impede your progress!

#pragma once

#include "game/egoboo_typedef.h"

#include "egolib/file_formats/passage_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct script_state_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_shop;
typedef struct s_shop shop_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define MAX_PASS             256                     ///< Maximum number of passages ( mul 32 )
#define MAX_SHOP             MAX_PASS
#define CLOSETOLERANCE       3                       ///< For closing doors

#define SHOP_NOOWNER 0xFFFF        ///< Shop has no owner
#define SHOP_STOLEN  0xFFFF        ///< Someone stole a item

#define INVALID_SHOP_REF ((SHOP_REF) MAX_SHOP)
#define INVALID_PASS_REF ((PASS_REF) MAX_PASS)

/// The pre-defined orders for communicating with shopkeepers
enum e_shop_orders
{
    SHOP_BUY       = 0,
    SHOP_SELL,
    SHOP_NOAFFORD,
    SHOP_THEFT,
    SHOP_ENTER,
    SHOP_EXIT,

    SHOP_LAST
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Passages

DECLARE_STACK_EXTERN( passage_t, PassageStack, MAX_PASS );

#define VALID_PASSAGE_RANGE( IPASS ) ( ((IPASS) >= 0) && ((IPASS) <   MAX_PASS) )
#define VALID_PASSAGE( IPASS )       ( VALID_PASSAGE_RANGE( IPASS ) && ((IPASS) <  PassageStack.count) )
#define INVALID_PASSAGE( IPASS )     ( !VALID_PASSAGE( IPASS ) )

/// The data defining a shop
struct s_shop
{
    PASS_REF passage;  ///< The passage number
    CHR_REF  owner;    ///< Who gets the gold?
};

DECLARE_STACK_EXTERN( shop_t, ShopStack, MAX_SHOP );

#define VALID_SHOP_RANGE( ISHOP ) ( ((ISHOP) >= 0) && ((ISHOP) <   MAX_SHOP) )
#define VALID_SHOP( ISHOP )       ( VALID_SHOP_RANGE( ISHOP ) && ((ISHOP) <  ShopStack.count) )
#define INVALID_SHOP( ISHOP )     ( !VALID_SHOP( ISHOP ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// prototypes

void   check_passage_music( void );
void   clear_all_passages( void );
void   activate_passages_file_vfs( void );

void   add_passage( passage_t * pdata );

bool   open_passage( const PASS_REF ipassage );
bool   close_passage( const PASS_REF ipassage );
void     flash_passage( const PASS_REF ipassage, Uint8 color );
CHR_REF who_is_blocking_passage( const PASS_REF passage, const CHR_REF isrc, IDSZ idsz, const BIT_FIELD targeting_bits, IDSZ require_item );
void   add_shop_passage( const CHR_REF owner, const PASS_REF ipassage );

bool point_is_in_passage( const PASS_REF ipassage, float xpos, float ypos );
bool object_is_in_passage( const PASS_REF ipassage, float xpos, float ypos, float radius );

CHR_REF  shop_get_owner( int ix, int iy );

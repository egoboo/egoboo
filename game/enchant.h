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

/* Egoboo - enchant.h
 * Decleares some stuff used for handling enchants
 */

#include "egoboo_typedef.h"
#include "egoboo.h"

#include "eve_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define LEAVEALL                0
#define LEAVEFIRST              1
#define LEAVENONE               2

// Enchantment template
#define MAX_EVE                          MAX_PROFILE    // One enchant type per model

DEFINE_STACK_EXTERN(eve_t, EveStack, MAX_EVE );

#define VALID_EVE_RANGE( IEVE ) ( ((IEVE) >= 0) && ((IEVE) < MAX_EVE) )
#define VALID_EVE( IEVE )       ( VALID_EVE_RANGE( IEVE ) && EveStack.lst[IEVE].loaded )
#define INVALID_EVE( IEVE )     ( !VALID_EVE_RANGE( IEVE ) || !EveStack.lst[IEVE].loaded )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Enchantment variables

#define MAX_ENC                      200         // Number of enchantments

struct s_enc
{
    EGO_OBJECT_STUFF

    Sint16  time;                    // Time before end
    Uint16  spawntime;               // Time before spawn

    Uint16  iprofile;                // The object  profile index that spawned this enchant
    Uint16  eve;                     // The enchant profile index

    Uint16  target;                  // Who it enchants
    Uint16  owner;                   // Who cast the enchant
    Uint16  spawner;                 // The spellbook character
    Uint16  spawnermodel;            // The spellbook character's CapList index
    Uint16  overlay;                 // The overlay character

    Sint16  ownermana;               // Boost values
    Sint16  ownerlife;
    Sint16  targetmana;
    Sint16  targetlife;

    Uint16  nextenchant;             // Next in the list

    bool_t  setyesno[MAX_ENCHANT_SET];// Was it set?
    bool_t  setsave[MAX_ENCHANT_SET]; // The value to restore
    Sint16  addsave[MAX_ENCHANT_ADD]; // The value to take away
};
typedef struct s_enc enc_t;

DEFINE_LIST_EXTERN(enc_t, EncList, MAX_ENC );

#define VALID_ENC_RANGE( IENC ) ( ((IENC) >= 0) && ((IENC) < MAX_ENC) )
#define VALID_ENC( IENC )       ( VALID_ENC_RANGE( IENC ) && EncList.lst[IENC].on )
#define INVALID_ENC( IENC )     ( !VALID_ENC_RANGE( IENC ) || !EncList.lst[IENC].on )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Prototypes
void EncList_free_all();
void getadd( int min, int value, int max, int* valuetoadd );
void fgetadd( float min, float value, float max, float* valuetoadd );
Uint16 EncList_get_free();
Uint16 enchant_value_filled( Uint16 enchantindex, Uint8 valueindex );
bool_t remove_enchant( Uint16 enchantindex );
void set_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
void add_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
Uint16 spawn_one_enchant( Uint16 owner, Uint16 target,
                      Uint16 spawner, Uint16 enc_override, Uint16 modeloptional );
Uint16 load_one_enchant_profile( const char* szLoadName, Uint16 profile );
void unset_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void remove_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void disenchant_character( Uint16 cnt );

Uint16         enc_get_iowner( Uint16 ienc );
Uint16         enc_get_ieve  ( Uint16 ienc );

struct s_chr * enc_get_powner( Uint16 ienc );
eve_t        * enc_get_peve  ( Uint16 ienc );


IDSZ   enc_get_idszremove( Uint16 ienc );
bool_t enc_is_removed( Uint16 ienc, Uint16 test_profile );
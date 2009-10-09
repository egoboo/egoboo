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

/// @file enchant.h
/// @details Decleares some stuff used for handling enchants

#include "egoboo_typedef.h"
#include "egoboo.h"

#include "eve_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_object_profile;
struct s_chr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define LEAVEALL                0
#define LEAVEFIRST              1
#define LEAVENONE               2

/// Enchantment template
#define MAX_EVE                          MAX_PROFILE    ///< One enchant type per model

DEFINE_STACK_EXTERN(eve_t, EveStack, MAX_EVE );

#define VALID_EVE_RANGE( IEVE ) ( ((IEVE) >= 0) && ((IEVE) < MAX_EVE) )
#define VALID_EVE( IEVE )       ( VALID_EVE_RANGE( IEVE ) && EveStack.lst[IEVE].loaded )
#define INVALID_EVE( IEVE )     ( !VALID_EVE_RANGE( IEVE ) || !EveStack.lst[IEVE].loaded )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Enchantment variables

#define MAX_ENC                      200         ///< Number of enchantments

/// A single egoboo enchantment
/// This "inherits" for ego_object_base_t
struct s_enc
{
    ego_object_base_t obj_base;

    int     time;                    ///< Time before end
    Uint16  spawntime;               ///< Time before spawn

    Uint16  profile_ref;             ///< The object  profile index that spawned this enchant
    Uint16  eve_ref;                 ///< The enchant profile index

    Uint16  target_ref;              ///< Who it enchants
    Uint16  owner_ref;               ///< Who cast the enchant
    Uint16  spawner_ref;             ///< The spellbook character
    Uint16  spawnermodel_ref;        ///< The spellbook character's CapList index
    Uint16  overlay_ref;             ///< The overlay character

    Sint16  ownermana;               ///< Boost values
    Sint16  ownerlife;
    Sint16  targetmana;
    Sint16  targetlife;

    Uint16  nextenchant_ref;             ///< Next in the list

    bool_t  setyesno[MAX_ENCHANT_SET];// Was it set?
    bool_t  setsave[MAX_ENCHANT_SET]; ///< The value to restore
    Sint16  addsave[MAX_ENCHANT_ADD]; ///< The value to take away
};
typedef struct s_enc enc_t;

DEFINE_LIST_EXTERN(enc_t, EncList, MAX_ENC );

#define VALID_ENC_RANGE( IENC ) ( ((IENC) >= 0) && ((IENC) < MAX_ENC) )
#define ALLOCATED_ENC( IENC )   ( VALID_ENC_RANGE( IENC ) && ALLOCATED_OBJ ( &(EncList.lst[IENC].obj_base) ) )
#define ACTIVE_ENC( IENC )      ( VALID_ENC_RANGE( IENC ) && ACTIVE_OBJ    ( &(EncList.lst[IENC].obj_base) ) )
#define WAITING_ENC( IENC )     ( VALID_ENC_RANGE( IENC ) && WAITING_OBJ   ( &(EncList.lst[IENC].obj_base) ) )
#define TERMINATED_ENC( IENC )  ( VALID_ENC_RANGE( IENC ) && TERMINATED_OBJ( &(EncList.lst[IENC].obj_base) ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Prototypes

void init_all_eve();
void release_all_eve();
bool_t release_one_eve( Uint16 ieve );

void EncList_free_all();

void update_all_enchants();
void cleanup_all_enchants();
Uint16 cleanup_enchant_list( Uint16 ienc );

void getadd( int min, int value, int max, int* valuetoadd );
void fgetadd( float min, float value, float max, float* valuetoadd );
Uint16 EncList_get_free();
Uint16 enchant_value_filled( Uint16 enchantindex, Uint8 valueindex );
bool_t remove_enchant( Uint16 enchantindex );
void set_enchant_value( Uint16 enchantindex, Uint8 valueindex, Uint16 profile );
void add_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
Uint16 spawn_one_enchant( Uint16 owner, Uint16 target,
                      Uint16 spawner, Uint16 enc_override, Uint16 modeloptional );
Uint16 load_one_enchant_profile( const char* szLoadName, Uint16 profile );
void unset_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void remove_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void disenchant_character( Uint16 cnt );

Uint16                    enc_get_ipro( Uint16 ienc );
struct s_object_profile * enc_get_ppro( Uint16 ienc );

Uint16         enc_get_iowner( Uint16 ienc );
Uint16         enc_get_ieve  ( Uint16 ienc );

struct s_chr * enc_get_powner( Uint16 ienc );
eve_t        * enc_get_peve  ( Uint16 ienc );

IDSZ   enc_get_idszremove( Uint16 ienc );
bool_t enc_is_removed( Uint16 ienc, Uint16 test_profile );

bool_t enc_request_terminate( Uint16 ienc );
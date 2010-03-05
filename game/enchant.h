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

#include "egoboo_object.h"

#include "egoboo.h"

#include "eve_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_object_profile;
struct s_chr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define ENC_LEAVE_ALL                0
#define ENC_LEAVE_FIRST              1
#define ENC_LEAVE_NONE               2

#define MAX_EVE                 MAX_PROFILE    ///< One enchant type per model
#define MAX_ENC                 200            ///< Number of enchantments

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Enchantment template
DECLARE_STACK_EXTERN( eve_t, EveStack, MAX_EVE );

#define VALID_EVE_RANGE( IEVE ) ( ((IEVE) >= 0) && ((IEVE) < MAX_EVE) )
#define LOADED_EVE( IEVE )      ( VALID_EVE_RANGE( IEVE ) && EveStack.lst[IEVE].loaded )

//--------------------------------------------------------------------------------------------
/// The difinition of a single egoboo enchantment
/// This "inherits" from ego_object_base_t
struct s_enc
{
    ego_object_base_t obj_base;

    int     time;                    ///< Time before end
    int     spawntime;               ///< Time before spawn

    PRO_REF profile_ref;             ///< The object  profile index that spawned this enchant
    EVE_REF eve_ref;                 ///< The enchant profile index

    CHR_REF target_ref;              ///< Who it enchants
    CHR_REF owner_ref;               ///< Who cast the enchant
    CHR_REF spawner_ref;             ///< The spellbook character
    PRO_REF spawnermodel_ref;        ///< The spellbook character's CapStack index
    CHR_REF overlay_ref;             ///< The overlay character

    int     owner_mana;               ///< Boost values
    int     owner_life;
    int     target_mana;
    int     target_life;

    ENC_REF nextenchant_ref;             ///< Next in the list

    bool_t  setyesno[MAX_ENCHANT_SET];  ///< Was it set?
    float   setsave[MAX_ENCHANT_SET];   ///< The value to restore

    bool_t  addyesno[MAX_ENCHANT_ADD];  ///< Was the value adjusted
    float   addsave[MAX_ENCHANT_ADD];   ///< The adjustment
};
typedef struct s_enc enc_t;

DECLARE_LIST_EXTERN( enc_t, EncList, MAX_ENC );

#define VALID_ENC_RANGE( IENC ) ( ((IENC) >= 0) && ((IENC) < MAX_ENC) )
#define ALLOCATED_ENC( IENC )   ( VALID_ENC_RANGE( IENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define ACTIVE_ENC( IENC )      ( VALID_ENC_RANGE( IENC ) && ACTIVE_PBASE    ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define WAITING_ENC( IENC )     ( VALID_ENC_RANGE( IENC ) && WAITING_PBASE   ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define TERMINATED_ENC( IENC )  ( VALID_ENC_RANGE( IENC ) && TERMINATED_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )

#define DEFINED_ENC( IENC )        ( VALID_ENC_RANGE( IENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define PRE_TERMINATED_ENC( IENC ) ( VALID_ENC_RANGE( IENC ) && ( ACTIVE_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) || WAITING_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) ) )

#define GET_INDEX_PENC( PENC )      ((size_t)GET_INDEX_POBJ( PENC, MAX_ENC ))
#define GET_REF_PENC( PENC )        ((ENC_REF)GET_INDEX_PENC( PENC ))
#define VALID_ENC_PTR( PENC )       ( (NULL != (PENC)) && VALID_ENC_RANGE( GET_REF_POBJ( PENC, MAX_ENC) ) )
#define ALLOCATED_PENC( PENC )      ( VALID_ENC_PTR( PENC ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PENC) ) )
#define ACTIVE_PENC( PENC )         ( VALID_ENC_PTR( PENC ) && ACTIVE_PBASE( POBJ_GET_PBASE(PENC) ) )

#define DEFINED_PENC( PENC )        ( VALID_ENC_PTR( PENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PENC) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PENC) ) )
#define PRE_TERMINATED_PENC( PENC ) ( VALID_ENC_PTR( PENC ) && ( ACTIVE_PBASE( POBJ_GET_PBASE(PENC) ) || WAITING_PBASE( POBJ_GET_PBASE(PENC) ) ) )

#define ENC_BEGIN_LOOP_ACTIVE(IT, PENC) {size_t IT##internal; for(IT##internal=0;IT##internal<EncList.used_count;IT##internal++) { ENC_REF IT; enc_t * PENC = NULL; IT = (ENC_REF)EncList.used_ref[IT##internal]; if(!ACTIVE_ENC(IT)) continue; PENC = EncList.lst + IT;
#define ENC_END_LOOP() }}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Prototypes

void   init_all_eve();
void   release_all_eve();
bool_t release_one_eve( const EVE_REF by_reference ieve );

void EncList_free_all();
void EncList_update_used();

void    update_all_enchants();
void    cleanup_all_enchants();
ENC_REF cleanup_enchant_list( const ENC_REF by_reference ienc );

ENC_REF enchant_value_filled( const ENC_REF by_reference enchant_idx, int value_idx );
bool_t  remove_enchant( const ENC_REF by_reference  enchant_idx );
void    enchant_apply_set( const ENC_REF by_reference  enchant_idx, int value_idx, const PRO_REF by_reference profile );
void    enchant_apply_add( const ENC_REF by_reference  enchant_idx, int value_idx, const EVE_REF by_reference enchanttype );
ENC_REF spawn_one_enchant( const CHR_REF by_reference owner, const CHR_REF by_reference target, const CHR_REF by_reference spawner, const ENC_REF by_reference enc_override, const PRO_REF by_reference modeloptional );
EVE_REF load_one_enchant_profile( const char* szLoadName, const EVE_REF by_reference profile );
void    enchant_remove_set( const ENC_REF by_reference  enchant_idx, int value_idx );
void    enchant_remove_add( const ENC_REF by_reference  enchant_idx, int value_idx );

bool_t enc_request_terminate( const ENC_REF by_reference  ienc );

void enchant_system_begin();
void enchant_system_end();

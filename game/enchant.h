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

#include "file_formats/eve_file.h"

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
struct s_enc_spawn_data
{
    CHR_REF owner_ref;
    CHR_REF target_ref;
    CHR_REF spawner_ref;
    PRO_REF profile_ref;
    EVE_REF eve_ref;
};
typedef struct s_enc_spawn_data enc_spawn_data_t;

//--------------------------------------------------------------------------------------------

/// The difinition of a single Egoboo enchantment
/// This "inherits" from obj_data_t
struct s_enc
{
    obj_data_t obj_base;

    enc_spawn_data_t  spawn_data;

    int     lifetime;                ///< Time before end
    int     spawn_timer;             ///< Time before spawn

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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Prototypes

void enchant_system_begin();
void enchant_system_end();

enc_t * enc_ctor( enc_t * penc );
enc_t * enc_dtor( enc_t * penc );

void   init_all_eve();
void   release_all_eve();
bool_t release_one_eve( const EVE_REF ieve );

void    update_all_enchants();
void    cleanup_all_enchants();

void    bump_all_enchants_update_counters( void );

ENC_REF enchant_value_filled( const ENC_REF enchant_idx, int value_idx );
bool_t  remove_enchant( const ENC_REF  enchant_idx, ENC_REF *  enchant_parent );
bool_t  remove_all_enchants_with_idsz( CHR_REF ichr, IDSZ remove_idsz );
//#define  remove_all_character_enchants( PCHR ) remove_all_enchants_with_idsz( PCHR, IDSZ_NONE )
void    enchant_apply_set( const ENC_REF  enchant_idx, int value_idx, const PRO_REF profile );
void    enchant_apply_add( const ENC_REF  enchant_idx, int value_idx, const EVE_REF enchanttype );
ENC_REF spawn_one_enchant( const CHR_REF owner, const CHR_REF target, const CHR_REF spawner, const ENC_REF enc_override, const PRO_REF modeloptional );
EVE_REF load_one_enchant_profile_vfs( const char* szLoadName, const EVE_REF profile );
void    enchant_remove_set( const ENC_REF  enchant_idx, int value_idx );
void    enchant_remove_add( const ENC_REF  enchant_idx, int value_idx );

bool_t enc_request_terminate( const ENC_REF  ienc );

enc_t * enc_run_config( enc_t * penc );

ENC_REF cleanup_enchant_list( const ENC_REF ienc, ENC_REF * enc_parent );

enc_t * enc_config_construct( enc_t * penc, int max_iterations );
enc_t * enc_config_initialize( enc_t * penc, int max_iterations );
enc_t * enc_config_activate( enc_t * penc, int max_iterations );
enc_t * enc_config_deinitialize( enc_t * penc, int max_iterations );
enc_t * enc_config_deconstruct( enc_t * penc, int max_iterations );

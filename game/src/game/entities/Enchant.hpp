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

/// @file  game/entities/Enchant.hpp
/// @brief Enchantment entities.

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/entities/_Include.hpp` instead)
#endif

#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"

// Forward declarations.
struct EnchantManager;
class ObjectProfile;
class GameObject;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define ENC_LEAVE_ALL           0
#define ENC_LEAVE_FIRST         1
#define ENC_LEAVE_NONE          2



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Enchantment template
extern Stack<eve_t, MAX_EVE> EveStack;

#define VALID_EVE_RANGE(IEVE) (((IEVE) >= 0) && ((IEVE) < MAX_EVE))
#define LOADED_EVE(IEVE)      (VALID_EVE_RANGE( IEVE ) && EveStack.lst[IEVE].loaded)

//--------------------------------------------------------------------------------------------
struct enc_spawn_data_t
{
    CHR_REF owner_ref;
    CHR_REF target_ref;
    CHR_REF spawner_ref;
    PRO_REF profile_ref;
    EVE_REF eve_ref;
};

/**
 * @brief
 *  The definition of an enchantment entity.
 * @extends
 *  Ego::Entity
 */
struct enc_t : public _StateMachine < enc_t, EnchantManager >
{
    enc_spawn_data_t spawn_data;

    int     lifetime;                 ///< Time before end
    int     spawn_timer;              ///< Time before spawn

    PRO_REF profile_ref;              ///< The object  profile index that spawned this enchant
    EVE_REF eve_ref;                  ///< The enchant profile index

    CHR_REF target_ref;               ///< Who it enchants
    CHR_REF owner_ref;                ///< Who cast the enchant
    CHR_REF spawner_ref;              ///< The spellbook character
    PRO_REF spawnermodel_ref;         ///< The spellbook character's profile index
    CHR_REF overlay_ref;              ///< The overlay character

    int     owner_mana;               ///< Boost values
    int     owner_life;
    int     target_mana;
    int     target_life;

    ENC_REF nextenchant_ref;          ///< Next in the list

    bool setyesno[MAX_ENCHANT_SET];   ///< Was it set?
    float setsave[MAX_ENCHANT_SET];   ///< The value to restore

    bool addyesno[MAX_ENCHANT_ADD];   ///< Was the value adjusted
    float addsave[MAX_ENCHANT_ADD];   ///< The adjustment

    enc_t *ctor();
    enc_t *dtor();

    static bool request_terminate(enc_t *self);

    static bool free(enc_t *self);

    // enchant state machine function
    enc_t *config_do_init();
    // enchant state machine function
    enc_t *config_do_deinit();
    // enchant state machine function
    enc_t *config_do_active();

};

// enchant_system functions
void enchant_system_begin();
void enchant_system_end();

ENC_REF spawn_one_enchant(const CHR_REF owner, const CHR_REF target, const CHR_REF spawner, const ENC_REF enc_override, const PRO_REF modeloptional);

void update_all_enchants();
void cleanup_all_enchants();

void bump_all_enchants_update_counters();

// enchant list management
bool remove_enchant(const ENC_REF encRef, ENC_REF *enchant_parent);
bool remove_all_enchants_with_idsz(const CHR_REF chrRef, IDSZ remove_idsz);
ENC_REF cleanup_enchant_list(const ENC_REF encRef, ENC_REF *enc_parent);

// enc functions
ENC_REF enc_value_filled(const ENC_REF encRef, int value_idx);
void enc_apply_set(const ENC_REF encRef, int value_idx, const PRO_REF profile);
void enc_apply_add(const ENC_REF encRef, int value_idx, const EVE_REF enchanttype);
void enc_remove_set(const ENC_REF encRef, int value_idx);
void enc_remove_add(const ENC_REF encRef, int value_idx);

// EveStack functions
void EveStack_init_all();
void EveStack_release_all();
bool EveStack_release_one(const EVE_REF eveRef);
EVE_REF EveStack_losd_one(const char* szLoadName, const EVE_REF profile);



//--------------------------------------------------------------------------------------------
// FORWARD DECLARARIONS (inline)
//--------------------------------------------------------------------------------------------
PRO_REF enc_get_ipro(const ENC_REF ienc);
ObjectProfile *enc_get_ppro(const ENC_REF ienc);

CHR_REF enc_get_iowner(const ENC_REF ienc);
GameObject *enc_get_powner(const ENC_REF ienc);

EVE_REF enc_get_ieve(const ENC_REF ienc);
eve_t *enc_get_peve(const ENC_REF ienc);

IDSZ enc_get_idszremove(const ENC_REF ienc);
bool enc_is_removed(const ENC_REF ienc, const PRO_REF test_profile);

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

/// @file  game/Entities/Enchant.cpp
/// @brief Enchantment entities.

#define GAME_ENTITIES_PRIVATE 1
#include "game/Entities/Enchant.hpp"

#include "egolib/Audio/AudioSystem.hpp"
#include "game/game.h"
#include "game/script_functions.h"
#include "game/egoboo.h"

#include "game/char.h"
#include "game/mad.h"

#include "game/Profiles/_Include.hpp"

#include "game/Entities/EnchantHandler.hpp"
#include "game/Entities/ObjectHandler.hpp"
#include "game/Entities/ParticleHandler.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool unlink_enchant( const ENC_REF ienc, ENC_REF * enc_parent );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool enc_t::free(enc_t *self)
{
    if (!ALLOCATED_PENC(self)) return false;
    /* Nothing to do yet. */
    return true;
}

//--------------------------------------------------------------------------------------------
enc_t::enc_t()
{
}
enc_t *enc_t::config_do_ctor()
{
    // grab the base object
    Ego::Entity *parent = POBJ_GET_PBASE(this);

    enc_spawn_data_t::reset(&spawn_data);
    lifetime = 0;
    spawn_timer = 0;
    
    owner_mana = 0;
    owner_life = 0;
    target_mana = 0;
    target_life = 0;

    for (size_t i = 0; i < eve_t::MAX_ENCHANT_SET; ++i)
    {
        _set[i]._modified = false; _set[i]._oldValue = 0.0f;
        _add[i]._modified = false; _add[i]._oldValue = 0.0f;
    }

    this->profile_ref = INVALID_PRO_REF;
    this->eve_ref = INVALID_EVE_REF;

    this->target_ref = INVALID_CHR_REF;
    this->owner_ref = INVALID_CHR_REF;
    this->spawner_ref = INVALID_CHR_REF;
    this->spawnermodel_ref = INVALID_PRO_REF;
    this->overlay_ref = INVALID_CHR_REF;

    this->nextenchant_ref = INVALID_ENC_REF;

    // reset the base counters
    parent->update_count = 0;
    parent->frame_count = 0;
    // we are done constructing. move on to initializing.
    parent->state = Ego::Entity::State::Initializing;

    return this;
}

//--------------------------------------------------------------------------------------------
enc_t::~enc_t()
{
}
enc_t *enc_t::config_do_dtor()
{
    // Destroy the object.
    enc_t::free(this);

    // Destroy the parent object.
    // Sets the state to Ego::Entity::State::Terminated automatically.
    POBJ_TERMINATE(this);

    return this;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool unlink_enchant( const ENC_REF ienc, ENC_REF * enc_parent )
{
    if (!ALLOCATED_ENC(ienc)) return false;
    enc_t *penc = EnchantHandler::get().get_ptr( ienc );

    // Unlink it from the spawner (if possible)
    if ( _gameObjects.exists( penc->spawner_ref ) )
    {
        Object * pspawner = _gameObjects.get( penc->spawner_ref );

        if ( ienc == pspawner->undoenchant )
        {
            pspawner->undoenchant = INVALID_ENC_REF;
        }
    }

    // find the parent reference for the enchant
    if ( NULL == enc_parent && _gameObjects.exists( penc->target_ref ) )
    {
        ENC_REF ienc_last, ienc_now, ienc_nxt;
        size_t ienc_count;

        Object * ptarget;

        ptarget = _gameObjects.get( penc->target_ref );

        if ( ptarget->firstenchant == ienc )
        {
            // It was the first in the list
            enc_parent = &( ptarget->firstenchant );
        }
        else
        {
            // Search until we find it
            ienc_last = ienc_now = ptarget->firstenchant;
            ienc_count = 0;
            while (EnchantHandler::get().isValidRef( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
            {
                ienc_last = ienc_now;
                ienc_nxt  = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

                if ( ienc_now == ienc ) break;

                ienc_now  = ienc_nxt;
                ienc_count++;
            }
            if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

            // Relink the last enchantment
            if ( ienc_now == ienc )
            {
                enc_parent = &( EnchantHandler::get().get_ptr(ienc_last)->nextenchant_ref );
            }
        }
    }

    // unlink the enchant from the parent reference
    if ( NULL != enc_parent )
    {
        *enc_parent = EnchantHandler::get().get_ptr(ienc)->nextenchant_ref;
    }

    return NULL != enc_parent;
}

//--------------------------------------------------------------------------------------------
bool remove_all_enchants_with_idsz( const CHR_REF ichr, IDSZ remove_idsz )
{
    /// @author ZF
    /// @details This function removes all enchants with the character that has the specified
    ///               IDSZ. If idsz [NONE] is specified, all enchants will be removed. Return true
    ///               if at least one enchant was removed.

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;

    eve_t * peve;
    bool retval = false;
    Object *pchr;

    // Stop invalid pointers
    if ( !_gameObjects.exists( ichr ) ) return false;
    pchr = _gameObjects.get( ichr );

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pchr );

    // Check all enchants to see if they are removed
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while (EnchantHandler::get().isValidRef( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
    {
        ienc_nxt  = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

        peve = enc_get_peve( ienc_now );
        if ( NULL != peve && ( IDSZ_NONE == remove_idsz || remove_idsz == peve->removedByIDSZ ) )
        {
            remove_enchant( ienc_now, NULL );
            retval = true;
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool remove_enchant( const ENC_REF ienc, ENC_REF * enc_parent )
{
    /// @author ZZ
    /// @details This function removes a specific enchantment and adds it to the unused list

    int iwave;
    int add_type, set_type;

    enc_t * penc;
    eve_t * peve;
    CHR_REF target_ref, spawner_ref, overlay_ref;
    Object * target_ptr, *spawner_ptr, *overlay_ptr;

    if ( !ALLOCATED_ENC( ienc ) ) return false;
    penc = EnchantHandler::get().get_ptr( ienc );
    peve = enc_get_peve( ienc );

    target_ref = INVALID_CHR_REF;
    target_ptr = NULL;
    if ( _gameObjects.exists( penc->target_ref ) )
    {
        target_ref = penc->target_ref;
        target_ptr = _gameObjects.get( penc->target_ref );
    }

    spawner_ref = INVALID_CHR_REF;
    spawner_ptr = NULL;
    if ( _gameObjects.exists( penc->spawner_ref ) )
    {
        spawner_ref = penc->spawner_ref;
        spawner_ptr = _gameObjects.get( penc->spawner_ref );
    }

    overlay_ref = INVALID_CHR_REF;
    overlay_ptr = NULL;
    if ( _gameObjects.exists( penc->overlay_ref ) )
    {
        overlay_ref = penc->overlay_ref;
        overlay_ptr = _gameObjects.get( penc->overlay_ref );
    }

    // Unsparkle the spellbook
    if ( NULL != spawner_ptr )
    {
        spawner_ptr->sparkle = NOSPARKLE;

        // Make the spawner unable to undo the enchantment
        if ( spawner_ptr->undoenchant == ienc )
        {
            spawner_ptr->undoenchant = INVALID_ENC_REF;
        }
    }

    //---- Remove all the enchant stuff in exactly the opposite order to how it was applied

    // Remove all of the cumulative values first, since we did it
    for (add_type = eve_t::ENC_ADD_LAST; add_type >= eve_t::ENC_ADD_FIRST; add_type--)
    {
        enc_remove_add( ienc, add_type );
    }

    // unset them in the reverse order of setting them, doing morph last
    for (set_type = eve_t::ENC_SET_LAST; set_type >= eve_t::ENC_SET_FIRST; set_type--)
    {
        enc_remove_set( ienc, set_type );
    }

    // Now fix dem weapons
    if ( NULL != target_ptr )
    {
        reset_character_alpha( target_ptr->holdingwhich[SLOT_LEFT] );
        reset_character_alpha( target_ptr->holdingwhich[SLOT_RIGHT] );
    }

    // unlink this enchant from its parent
    unlink_enchant( ienc, enc_parent );

    // Kill overlay too...
    if ( INVALID_CHR_REF != overlay_ref )
    {
        if ( NULL != overlay_ptr )
        {
            switch_team( overlay_ref, overlay_ptr->team_base );
        }

        kill_character( overlay_ref, INVALID_CHR_REF, true );
    }

    // nothing above this demends on having a valid enchant profile
    if ( NULL != peve )
    {
        // Play the end sound
        PRO_REF imodel = penc->spawnermodel_ref;
        if (ProfileSystem::get().isValidProfileID(imodel))
        {
            iwave = peve->endsound_index;
            if ( nullptr != target_ptr )
            {
                AudioSystem::get().playSound(target_ptr->pos_old, ProfileSystem::get().getProfile(imodel)->getSoundID(iwave));
            }
            else
            {
                AudioSystem::get().playSoundFull(ProfileSystem::get().getProfile(imodel)->getSoundID(iwave));
            }
        }

        // See if we spit out an end message
        if ( peve->endmessage >= 0 )
        {
            _display_message( target_ref, penc->profile_ref, peve->endmessage, NULL );
        }

        // Check to see if we spawn a poof
        if ( peve->poofonend )
        {
            spawn_poof( target_ref, penc->profile_ref );
        }

        //Remove special skills gained by the enchant
        if ( NULL != target_ptr )
        {
            //Reset see kurses
            if ( 0 != peve->seeKurses )
            {
                target_ptr->see_kurse_level = chr_get_skill( target_ptr, MAKE_IDSZ( 'C', 'K', 'U', 'R' ) );
            }

            //Reset darkvision
            if ( 0 != peve->darkvision )
            {
                target_ptr->darkvision_level = chr_get_skill( target_ptr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
            }
        }
    }

    EnchantHandler::get().free_one( ienc );

    /// @note all of the values in the penc are now invalid. we have to use previously evaluated
    /// values of target_ref and penc to kill the target (if necessary)
    // save this until the enchant is completely dead, since kill character can generate a
    // recursive call to this function through cleanup_one_character()
    if ( NULL != peve && peve->killtargetonend && INVALID_CHR_REF != target_ref )
    {
        if ( NULL != target_ptr )
        {
            switch_team( target_ref, target_ptr->team_base );
        }

        kill_character( target_ref, INVALID_CHR_REF, true );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
ENC_REF enc_value_filled( const ENC_REF  ienc, int value_idx )
{
    /// @author ZZ
    /// @details This function returns INVALID_ENC_REF if the enchantment's target has no conflicting
    ///    set values in its other enchantments.  Otherwise it returns the ienc
    ///    of the conflicting enchantment

    CHR_REF character;
    Object * pchr;

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;

    if ( value_idx < 0 || value_idx >= eve_t::MAX_ENCHANT_SET ) return INVALID_ENC_REF;

    if ( !INGAME_ENC( ienc ) ) return INVALID_ENC_REF;

    character = EnchantHandler::get().get_ptr(ienc)->target_ref;
    if ( !_gameObjects.exists( character ) ) return INVALID_ENC_REF;
    pchr = _gameObjects.get( character );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // scan the enchant list
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while (EnchantHandler::get().isValidRef( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
    {
        ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

        if (INGAME_ENC( ienc_now ) && EnchantHandler::get().get_ptr(ienc_now)->_set[value_idx]._modified)
        {
            break;
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    return ienc_now;
}

//--------------------------------------------------------------------------------------------
void enc_apply_set( const ENC_REF  ienc, int value_idx, const PRO_REF profile )
{
    /// @author ZZ
    /// @details This function sets and saves one of the character's stats

    ENC_REF conflict;
    CHR_REF character;
    enc_t * penc;
    eve_t * peve;
    Object * ptarget;

    if (value_idx < 0 || value_idx >= eve_t::MAX_ENCHANT_SET) return;

    if ( !DEFINED_ENC( ienc ) ) return;
    penc = EnchantHandler::get().get_ptr( ienc );

    peve = ProfileSystem::get().pro_get_peve(profile);
    if ( NULL == peve ) return;

    penc->_set[value_idx]._modified = false;
    if ( peve->_set[value_idx].apply )
    {
        conflict = enc_value_filled( ienc, value_idx );
        if ( peve->_override || INVALID_ENC_REF == conflict )
        {
            // Check for multiple enchantments
            if ( DEFINED_ENC( conflict ) )
            {
                // Multiple enchantments aren't allowed for sets
                if ( peve->remove_overridden )
                {
                    // Kill the old enchantment
                    remove_enchant( conflict, NULL );
                }
                else
                {
                    // Just unset the old enchantment's value
                    enc_remove_set( conflict, value_idx );
                }
            }

            // Set the value, and save the character's real stat
            if ( _gameObjects.exists( penc->target_ref ) )
            {
                character = penc->target_ref;
                ptarget = _gameObjects.get( character );

                penc->_set[value_idx]._modified = true;

                switch ( value_idx )
                {
                    case eve_t::SETDAMAGETYPE:
                        penc->_set[value_idx]._oldValue  = ptarget->damagetarget_damagetype;
                        ptarget->damagetarget_damagetype = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETNUMBEROFJUMPS:
                        penc->_set[value_idx]._oldValue = ptarget->jumpnumberreset;
                        ptarget->jumpnumberreset = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETLIFEBARCOLOR:
                        penc->_set[value_idx]._oldValue = ptarget->life_color;
                        ptarget->life_color       = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETMANABARCOLOR:
                        penc->_set[value_idx]._oldValue = ptarget->mana_color;
                        ptarget->mana_color = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETSLASHMODIFIER:
                        penc->_set[value_idx]._oldValue = ptarget->damage_modifier[DAMAGE_SLASH];
                        ptarget->damage_modifier[DAMAGE_SLASH] = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETCRUSHMODIFIER:
                        penc->_set[value_idx]._oldValue = ptarget->damage_modifier[DAMAGE_CRUSH];
                        ptarget->damage_modifier[DAMAGE_CRUSH] = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETPOKEMODIFIER:
                        penc->_set[value_idx]._oldValue = ptarget->damage_modifier[DAMAGE_POKE];
                        ptarget->damage_modifier[DAMAGE_POKE] = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETHOLYMODIFIER:
                        penc->_set[value_idx]._oldValue = ptarget->damage_modifier[DAMAGE_HOLY];
                        ptarget->damage_modifier[DAMAGE_HOLY] = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETEVILMODIFIER:
                        penc->_set[value_idx]._oldValue  = ptarget->damage_modifier[DAMAGE_EVIL];
                        ptarget->damage_modifier[DAMAGE_EVIL] = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETFIREMODIFIER:
                        penc->_set[value_idx]._oldValue = ptarget->damage_modifier[DAMAGE_FIRE];
                        ptarget->damage_modifier[DAMAGE_FIRE] = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETICEMODIFIER:
                        penc->_set[value_idx]._oldValue = ptarget->damage_modifier[DAMAGE_ICE];
                        ptarget->damage_modifier[DAMAGE_ICE] = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETZAPMODIFIER:
                        penc->_set[value_idx]._oldValue = ptarget->damage_modifier[DAMAGE_ZAP];
                        ptarget->damage_modifier[DAMAGE_ZAP] = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETFLASHINGAND:
                        penc->_set[value_idx]._oldValue = ptarget->flashand;
                        ptarget->flashand        = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETLIGHTBLEND:
                        penc->_set[value_idx]._oldValue = ptarget->inst.light;
                        ptarget->setLight(peve->_set[value_idx].value);
                        break;

                    case eve_t::SETALPHABLEND:
                        penc->_set[value_idx]._oldValue = ptarget->inst.alpha;
                        ptarget->setAlpha(peve->_set[value_idx].value);
                        break;

                    case eve_t::SETSHEEN:
                        penc->_set[value_idx]._oldValue = ptarget->inst.sheen;
                        ptarget->setSheen(peve->_set[value_idx].value);
                        break;

                    case eve_t::SETFLYTOHEIGHT:
                        penc->_set[value_idx]._oldValue = ptarget->flyheight;
                        if ( 0 == ptarget->flyheight && ptarget->getPosZ() > -2 )
                        {
                            ptarget->flyheight = peve->_set[value_idx].value;
                        }
                        break;

                    case eve_t::SETWALKONWATER:
                        penc->_set[value_idx]._oldValue = ptarget->waterwalk;
                        if ( !ptarget->waterwalk )
                        {
                            ptarget->waterwalk = ( 0 != peve->_set[value_idx].value );
                        }
                        break;

                    case eve_t::SETCANSEEINVISIBLE:
                        penc->_set[value_idx]._oldValue = ptarget->see_invisible_level > 0;
                        ptarget->see_invisible_level = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETMISSILETREATMENT:
                        penc->_set[value_idx]._oldValue  = ptarget->missiletreatment;
                        ptarget->missiletreatment = peve->_set[value_idx].value;
                        break;

                    case eve_t::SETCOSTFOREACHMISSILE:
                        penc->_set[value_idx]._oldValue = ptarget->missilecost;
                        ptarget->missilecost = peve->_set[value_idx].value;
                        ptarget->missilehandler = penc->owner_ref;
                        break;

                    case eve_t::SETMORPH:
                        // Special handler for morph
                        penc->_set[value_idx]._oldValue = ptarget->skin;
                        change_character( character, profile, 0, ENC_LEAVE_ALL ); // ENC_LEAVE_FIRST);
                        break;

                    case eve_t::SETCHANNEL:
                        penc->_set[value_idx]._oldValue = ptarget->canchannel;
                        ptarget->canchannel      = ( 0 != peve->_set[value_idx].value );
                        break;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void enc_apply_add( const ENC_REF ienc, int value_idx, const EVE_REF ieve )
{
    /// @author ZZ
    /// @details This function does cumulative modification to character stats

    int valuetoadd, newvalue;
    float fvaluetoadd, fnewvalue;
    CHR_REF character;
    enc_t * penc;
    eve_t * peve;
    Object * ptarget;

    if ( value_idx < 0 || value_idx >= eve_t::MAX_ENCHANT_ADD ) return;

    if ( !DEFINED_ENC( ienc ) ) return;
    penc = EnchantHandler::get().get_ptr( ienc );

    if ( ieve >= ENCHANTPROFILES_MAX || !EveStack.get_ptr(ieve)->_loaded ) return;
    peve = EveStack.get_ptr( ieve );

    if ( !peve->_add[value_idx].apply )
    {
        penc->_add[value_idx]._modified = false;
        penc->_add[value_idx]._oldValue  = 0.0f;
        return;
    }

    if ( !_gameObjects.exists( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = _gameObjects.get( character );

    valuetoadd  = 0;
    fvaluetoadd = 0.0f;
    switch ( value_idx )
    {
        case eve_t::ADDJUMPPOWER:
            fnewvalue = ptarget->jump_power;
            fvaluetoadd = peve->_add[value_idx].value;
            getadd_flt( 0.0f, fnewvalue, 30.0f, &fvaluetoadd );
            ptarget->jump_power += fvaluetoadd;
            break;

        case eve_t::ADDBUMPDAMPEN:
            fnewvalue = ptarget->phys.bumpdampen;
            fvaluetoadd = peve->_add[value_idx].value;
            getadd_flt( 0.0f, fnewvalue, 1.0f, &fvaluetoadd );
            ptarget->phys.bumpdampen += fvaluetoadd;
            break;

        case eve_t::ADDBOUNCINESS:
            fnewvalue = ptarget->phys.dampen;
            fvaluetoadd = peve->_add[value_idx].value;
            getadd_flt( 0.0f, fnewvalue, 0.95f, &fvaluetoadd );
            ptarget->phys.dampen += fvaluetoadd;
            break;

        case eve_t::ADDDAMAGE:
            newvalue = ptarget->damage_boost;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( 0, newvalue, 4096, &valuetoadd );
            ptarget->damage_boost += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDSIZE:
            fnewvalue = ptarget->fat_goto;
            fvaluetoadd = peve->_add[value_idx].value;
            getadd_flt( 0.5f, fnewvalue, 2.0f, &fvaluetoadd );
            ptarget->fat_goto += fvaluetoadd;
            ptarget->fat_goto_time = SIZETIME;
            break;

        case eve_t::ADDACCEL:
            fnewvalue = ptarget->maxaccel_reset;
            fvaluetoadd = peve->_add[value_idx].value;
            getadd_flt( 0.0f, fnewvalue, 1.50f, &fvaluetoadd );
            chr_set_maxaccel( ptarget, ptarget->maxaccel_reset + fvaluetoadd );
            break;

        case eve_t::ADDRED:
            newvalue = ptarget->inst.redshift;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( 0, newvalue, 6, &valuetoadd );
            chr_set_redshift( ptarget, ptarget->inst.redshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDGRN:
            newvalue = ptarget->inst.grnshift;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( 0, newvalue, 6, &valuetoadd );
            chr_set_grnshift( ptarget, ptarget->inst.grnshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDBLU:
            newvalue = ptarget->inst.blushift;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( 0, newvalue, 6, &valuetoadd );
            chr_set_blushift( ptarget, ptarget->inst.blushift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDDEFENSE:
            newvalue = ptarget->defense;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( 55, newvalue, 255, &valuetoadd );  // Don't fix again! /// @note ZF@> why limit min to 55?
            ptarget->defense = ptarget->defense + valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDMANA:
            newvalue = ptarget->mana_max;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( 0, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->mana_max += valuetoadd;
            //ptarget->mana    += valuetoadd;                       /// @note ZF@> bit of a problem here, we dont want players to heal or lose life by requipping magic ornaments
            ptarget->mana = CLIP( (UFP8_T)ptarget->mana, (UFP8_T)0, ptarget->mana_max );
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDLIFE:
            newvalue = ptarget->life_max;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( LOWSTAT, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->life_max += valuetoadd;
            //ptarget->life += valuetoadd;                        /// @note ZF@> bit of a problem here, we dont want players to heal or lose life by requipping magic ornaments
            ptarget->life = CLIP( (UFP8_T)ptarget->life, (UFP8_T)1, ptarget->life_max );
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDSTRENGTH:
            newvalue = ptarget->strength;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->strength += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDWISDOM:
            newvalue = ptarget->wisdom;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->wisdom += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDINTELLIGENCE:
            newvalue = ptarget->intelligence;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->intelligence += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDDEXTERITY:
            newvalue = ptarget->dexterity;
            valuetoadd = peve->_add[value_idx].value;
            getadd_int( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->dexterity += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case eve_t::ADDSLASHRESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_SLASH];
            fvaluetoadd = peve->_add[value_idx].value;
            ptarget->damage_resistance[DAMAGE_SLASH] += fvaluetoadd;
            break;

        case eve_t::ADDCRUSHRESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_CRUSH];
            fvaluetoadd = peve->_add[value_idx].value;
            ptarget->damage_resistance[DAMAGE_CRUSH] += fvaluetoadd;
            break;

        case eve_t::ADDPOKERESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_POKE];
            fvaluetoadd = peve->_add[value_idx].value;
            ptarget->damage_resistance[DAMAGE_POKE] += fvaluetoadd;
            break;

        case eve_t::ADDHOLYRESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_HOLY];
            fvaluetoadd = peve->_add[value_idx].value;
            ptarget->damage_resistance[DAMAGE_HOLY] += fvaluetoadd;
            break;

        case eve_t::ADDEVILRESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_EVIL];
            fvaluetoadd = peve->_add[value_idx].value;
            ptarget->damage_resistance[DAMAGE_EVIL] += fvaluetoadd;
            break;

        case eve_t::ADDFIRERESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_FIRE];
            fvaluetoadd = peve->_add[value_idx].value;
            ptarget->damage_resistance[DAMAGE_FIRE] += fvaluetoadd;
            break;

        case eve_t::ADDICERESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_ICE];
            fvaluetoadd = peve->_add[value_idx].value;
            ptarget->damage_resistance[DAMAGE_ICE] += fvaluetoadd;
            break;

        case eve_t::ADDZAPRESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_ZAP];
            fvaluetoadd = peve->_add[value_idx].value;
            ptarget->damage_resistance[DAMAGE_ZAP] += fvaluetoadd;
            break;
    }

    // save whether there was any change in the value
    penc->_add[value_idx]._modified = ( 0.0f != fvaluetoadd );

    // Save the value for undo
    penc->_add[value_idx]._oldValue  = fvaluetoadd;
}

enc_t *enc_t::config_do_init()
{
    enc_t *penc = this;

    enc_spawn_data_t * pdata;
    ENC_REF ienc;
    CHR_REF overlay;
    float lifetime;

    eve_t * peve;
    Object * ptarget;

    int add_type, set_type;

    if ( NULL == penc ) return NULL;
    ienc  = GET_REF_PENC( penc );

    // get the profile data
    pdata = &( penc->spawn_data );

    // store the profile
    penc->profile_ref  = pdata->profile_ref;

    // Convert from local pdata->eve_ref to global enchant profile
    if ( !LOADED_EVE( pdata->eve_ref ) )
    {
        log_debug( "spawn_one_enchant() - cannot spawn enchant with invalid enchant template (\"eve\") == %d\n", REF_TO_INT( pdata->eve_ref ) );

        return NULL;
    }
    penc->eve_ref = pdata->eve_ref;
    peve = EveStack.get_ptr( pdata->eve_ref );

    // turn the enchant on here. you can't fail to spawn after this point.
    POBJ_ACTIVATE( penc, peve->_name );

    // does the target exist?
    if ( !_gameObjects.exists( pdata->target_ref ) )
    {
        penc->target_ref   = INVALID_CHR_REF;
        ptarget            = NULL;
    }
    else
    {
        penc->target_ref = pdata->target_ref;
        ptarget = _gameObjects.get( penc->target_ref );
    }
    penc->target_mana  = peve->_target._manaDrain;
    penc->target_life  = peve->_target._lifeDrain;

    // does the owner exist?
    if ( !_gameObjects.exists( pdata->owner_ref ) )
    {
        penc->owner_ref = INVALID_CHR_REF;
    }
    else
    {
        penc->owner_ref  = pdata->owner_ref;
    }
    penc->owner_mana = peve->_owner._manaDrain;
    penc->owner_life = peve->_owner._lifeDrain;

    // does the spawner exist?
    if ( !_gameObjects.exists( pdata->spawner_ref ) )
    {
        penc->spawner_ref      = INVALID_CHR_REF;
        penc->spawnermodel_ref = INVALID_PRO_REF;
    }
    else
    {
        penc->spawner_ref = pdata->spawner_ref;
        penc->spawnermodel_ref = chr_get_ipro( pdata->spawner_ref );

        _gameObjects.get(penc->spawner_ref)->undoenchant = ienc;
    }

    //recuce enchant duration with damage resistance
    penc->spawn_timer    = 1;
    lifetime             = peve->lifetime;
    if ( lifetime > 0 && peve->required_damagetype < DAMAGE_COUNT && ptarget )
    {
        lifetime -= std::max( 1.0f, CEIL(( ptarget->damage_resistance[peve->required_damagetype] ) * peve->lifetime ) );
        printf( "Damage resistance reduced duration from %i to %3.0f\n", peve->lifetime, lifetime );
    }
    penc->lifetime       = lifetime;

    // Now set all of the specific values, morph first
    for ( set_type = eve_t::ENC_SET_FIRST; set_type <= eve_t::ENC_SET_LAST; set_type++ )
    {
        enc_apply_set( ienc, set_type, pdata->profile_ref );
    }

    // Now do all of the stat adds
    for ( add_type = eve_t::ENC_ADD_FIRST; add_type <= eve_t::ENC_ADD_LAST; add_type++ )
    {
        enc_apply_add( ienc, add_type, pdata->eve_ref );
    }

    // Add it as first in the list
    if ( NULL != ptarget )
    {
        penc->nextenchant_ref = ptarget->firstenchant;
        ptarget->firstenchant = ienc;
    }

    // Create an overlay character?
    if ( peve->spawn_overlay && NULL != ptarget )
    {
        overlay = spawn_one_character(ptarget->getPosition(), pdata->profile_ref, ptarget->team, 0, ptarget->ori.facing_z, NULL, INVALID_CHR_REF );
        if ( _gameObjects.exists( overlay ) )
        {
            Object *povl;
            mad_t *povl_mad;
            int action;

            povl     = _gameObjects.get( overlay );
            povl_mad = chr_get_pmad( overlay );

            penc->overlay_ref = overlay;  // Kill this character on end...
            povl->ai.target   = pdata->target_ref;
            povl->is_overlay  = true;
            chr_set_ai_state( povl, peve->spawn_overlay );  // ??? WHY DO THIS ???

            // Start out with ActionMJ...  Object activated
            action = mad_get_action_ref( chr_get_imad( overlay ), ACTION_MJ );
            if ( !ACTION_IS_TYPE( action, D ) )
            {
                chr_start_anim( povl, action, false, true );
            }

            // Assume it's transparent...
            povl->setLight(254);
            povl->setAlpha(0);
        }
    }

    //Apply special skill effects
    if ( NULL != ptarget )
    {

        // Allow them to see kurses?
        if ( 0 != peve->seeKurses )
        {
            ptarget->see_kurse_level = peve->seeKurses;
        }

        // Allow them to see in darkness (or blindness if negative)
        if ( 0 != peve->darkvision )
        {
            ptarget->darkvision_level = peve->darkvision;
        }

    }

    return penc;
}

enc_t *enc_t::config_do_active()
{
    enc_t *penc = this;
    /// @author ZZ
    /// @details This function allows enchantments to update, spawn particles,
    ///  do drains, stat boosts and despawn.

    ENC_REF  ienc;
    CHR_REF  owner, target;
    EVE_REF  eve;
    eve_t * peve;
    Object * ptarget;

    if ( NULL == penc ) return penc;
    ienc = GET_REF_PENC( penc );

    // the following functions should not be done the first time through the update loop
    if ( 0 == clock_wld ) return penc;

    peve = enc_get_peve( ienc );
    if ( NULL == peve ) return penc;

    // check to see whether the enchant needs to spawn some particles
    if ( penc->spawn_timer > 0 ) penc->spawn_timer--;

    if ( 0 == penc->spawn_timer && peve->contspawn._amount <= 0 )
    {
        int      tnc;
        FACING_T facing;
        penc->spawn_timer = peve->contspawn._delay;
        ptarget = _gameObjects.get( penc->target_ref );

        facing = ptarget->ori.facing_z;
        for ( tnc = 0; tnc < peve->contspawn._amount; tnc++ )
        {
            spawn_one_particle( ptarget->getPosition(), facing, penc->profile_ref, peve->contspawn._lpip,
                                INVALID_CHR_REF, GRIP_LAST, chr_get_iteam( penc->owner_ref ), penc->owner_ref, INVALID_PRT_REF, tnc, INVALID_CHR_REF );

            facing += peve->contspawn._facingAdd;
        }
    }

    // Do enchant drains and regeneration
    if ( clock_enc_stat >= ONESECOND )
    {
        if ( 0 == penc->lifetime )
        {
            EnchantHandler::get().request_terminate( ienc );
        }
        else
        {
            // Do enchant timer
            if ( penc->lifetime > 0 ) penc->lifetime--;

            // To make life easier
            owner  = enc_get_iowner( ienc );
            target = penc->target_ref;
            eve    = enc_get_ieve( ienc );
            Object *powner = _gameObjects.get(owner);

            // Do drains
            if ( powner && powner->alive )
            {

                // Change life
                if ( 0 != penc->owner_life )
                {
                    powner->life += penc->owner_life;
                    if ( powner->life <= 0 )
                    {
                        kill_character( owner, target, false );
                    }
                    if ( powner->life > powner->life_max )
                    {
                        powner->life = powner->life_max;
                    }
                }

                // Change mana
                if ( 0 != penc->owner_mana )
                {
                    bool mana_paid = cost_mana( owner, -penc->owner_mana, target );
                    if ( EveStack.get_ptr(eve)->endIfCannotPay && !mana_paid )
                    {
                        EnchantHandler::get().request_terminate( ienc );
                    }
                }

            }
            else if ( !EveStack.get_ptr(eve)->_owner._stay )
            {
                EnchantHandler::get().request_terminate( ienc );
            }

            // the enchant could have been inactivated by the stuff above
            // check it again
            if ( INGAME_ENC( ienc ) )
            {
                if ( powner && powner->alive )
                {

                    // Change life
                    if ( 0 != penc->target_life )
                    {
                        powner->life += penc->target_life;
                        if ( powner->life <= 0 )
                        {
                            kill_character( target, owner, false );
                        }
                        if ( powner->life > powner->life_max )
                        {
                            powner->life = powner->life_max;
                        }
                    }

                    // Change mana
                    if ( 0 != penc->target_mana )
                    {
                        bool mana_paid = cost_mana( target, -penc->target_mana, owner );
                        if ( EveStack.get_ptr(eve)->endIfCannotPay && !mana_paid )
                        {
                            EnchantHandler::get().request_terminate( ienc );
                        }
                    }

                }
                else if ( !EveStack.get_ptr(eve)->_target._stay )
                {
                    EnchantHandler::get().request_terminate( ienc );
                }
            }
        }
    }

    return penc;
}

enc_t *enc_t::config_do_deinit()
{
    enc_t *self = this;

    // Go to next state.
    self->obj_base.state = Ego::Entity::State::Destructing;
    self->obj_base.on = false;

    return self;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ENC_REF spawn_one_enchant( const CHR_REF owner, const CHR_REF target, const CHR_REF spawner, const ENC_REF enc_override, const PRO_REF modeloptional )
{
    /// @author ZZ
    /// @details This function enchants a target, returning the enchantment index or INVALID_ENC_REF
    ///    if failed

    ENC_REF enc_ref;
    EVE_REF eve_ref;

    eve_t * peve;
    enc_t * penc;
    Object * ptarget;

    PRO_REF loc_profile;
    CHR_REF loc_target;

    // Target must both be alive and on and valid
    loc_target = target;
    if ( !_gameObjects.exists( loc_target ) )
    {
        log_warning( "spawn_one_enchant() - failed because the target does not exist.\n" );
        return INVALID_ENC_REF;
    }
    ptarget = _gameObjects.get( loc_target );

    // you should be able to enchant dead stuff to raise the dead...
    // if( !ptarget->alive ) return INVALID_ENC_REF;

    if (ProfileSystem::get().isValidProfileID(modeloptional))
    {
        // The enchantment type is given explicitly
        loc_profile = modeloptional;
    }
    else
    {
        // The enchantment type is given by the spawner
        loc_profile = chr_get_ipro( spawner );

        if (!ProfileSystem::get().isValidProfileID(loc_profile))
        {
            log_warning( "spawn_one_enchant() - no valid profile for the spawning character \"%s\"(%d).\n", _gameObjects.get(spawner)->Name, REF_TO_INT( spawner ) );
            return INVALID_ENC_REF;
        }
    }

    eve_ref = ProfileSystem::get().pro_get_ieve(loc_profile);
    if ( !LOADED_EVE( eve_ref ) )
    {
        log_warning("spawn_one_enchant() - the object \"%s\"(%d) does not have an enchant profile.\n", ProfileSystem::get().getProfile(loc_profile)->getFilePath().c_str(), REF_TO_INT(loc_profile));

        return INVALID_ENC_REF;
    }
    peve = EveStack.get_ptr( eve_ref );

    // count all the requests for this enchantment type
    peve->_spawnRequestCount++;

    // Owner must both be alive and on and valid if it isn't a stayifnoowner enchant
    if ( !peve->_owner._stay && ( !_gameObjects.exists( owner ) || !_gameObjects.get(owner)->alive ) )
    {
        log_warning( "spawn_one_enchant() - failed because the required enchant owner cannot be found.\n" );
        return INVALID_ENC_REF;
    }

    // do retargeting, if necessary
    // Should it choose an inhand item?
    if ( peve->retarget )
    {
        // Left, right, or both are valid
        if ( _gameObjects.exists( ptarget->holdingwhich[SLOT_LEFT] ) )
        {
            // Only right hand is valid
            loc_target = ptarget->holdingwhich[SLOT_RIGHT];
        }
        else if ( _gameObjects.exists( ptarget->holdingwhich[SLOT_LEFT] ) )
        {
            // Pick left hand
            loc_target = ptarget->holdingwhich[SLOT_LEFT];
        }
        else
        {
            // No weapons to pick, should it pick itself???
            loc_target = INVALID_CHR_REF;
        }
    }

    // make sure the loc_target is alive
    if ( nullptr == ( ptarget ) || !ptarget->alive )
    {
        log_warning( "spawn_one_enchant() - failed because the target is not alive.\n" );
        return INVALID_ENC_REF;
    }
    ptarget = _gameObjects.get( loc_target );

    // Check peve->required_damagetype, 90% damage resistance is enough to resist the enchant
    if ( peve->required_damagetype < DAMAGE_COUNT )
    {
        if ( ptarget->damage_resistance[peve->required_damagetype] >= 0.90f ||
             HAS_SOME_BITS( ptarget->damage_modifier[peve->required_damagetype], DAMAGEINVICTUS ) )
        {
            log_debug( "spawn_one_enchant() - failed because the target is immune to the enchant.\n" );
            return INVALID_ENC_REF;
        }
    }

    // Check peve->require_damagetarget_damagetype
    if ( peve->require_damagetarget_damagetype < DAMAGE_COUNT )
    {
        if ( ptarget->damagetarget_damagetype != peve->require_damagetarget_damagetype )
        {
            log_warning( "spawn_one_enchant() - failed because the target not have the right damagetarget_damagetype.\n" );
            return INVALID_ENC_REF;
        }
    }

    // Find an enchant index to use
    enc_ref = EnchantHandler::get().allocate( enc_override );

    if ( !ALLOCATED_ENC( enc_ref ) )
    {
        log_warning( "spawn_one_enchant() - could not allocate an enchant.\n" );
        return INVALID_ENC_REF;
    }
    penc = EnchantHandler::get().get_ptr( enc_ref );

    POBJ_BEGIN_SPAWN( penc );

    penc->spawn_data.owner_ref   = owner;
    penc->spawn_data.target_ref  = loc_target;
    penc->spawn_data.spawner_ref = spawner;
    penc->spawn_data.profile_ref = loc_profile;
    penc->spawn_data.eve_ref     = eve_ref;

    // actually force the character to spawn
    penc = enc_t::config_activate( penc, 100 );

    // log all the successful spawns
    if ( NULL != penc )
    {
        POBJ_END_SPAWN( penc );
        peve->_spawnCount++;
    }

    return enc_ref;
}

//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
void enc_remove_set( const ENC_REF ienc, int value_idx )
{
    /// @author ZZ
    /// @details This function unsets a set value
    CHR_REF character;
    enc_t * penc;
    Object * ptarget;

    if ( value_idx < 0 || value_idx >= eve_t::MAX_ENCHANT_SET ) return;

    if ( !ALLOCATED_ENC( ienc ) ) return;
    penc = EnchantHandler::get().get_ptr( ienc );

    if ( value_idx >= eve_t::MAX_ENCHANT_SET || !penc->_set[value_idx]._modified ) return;

    if ( !_gameObjects.exists( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget   = _gameObjects.get( penc->target_ref );

    switch ( value_idx )
    {
        case eve_t::SETDAMAGETYPE:
            ptarget->damagetarget_damagetype = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETNUMBEROFJUMPS:
            ptarget->jumpnumberreset = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETLIFEBARCOLOR:
            ptarget->life_color = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETMANABARCOLOR:
            ptarget->mana_color = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETSLASHMODIFIER:
            ptarget->damage_modifier[DAMAGE_SLASH] = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETCRUSHMODIFIER:
            ptarget->damage_modifier[DAMAGE_CRUSH] = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETPOKEMODIFIER:
            ptarget->damage_modifier[DAMAGE_POKE] = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETHOLYMODIFIER:
            ptarget->damage_modifier[DAMAGE_HOLY] = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETEVILMODIFIER:
            ptarget->damage_modifier[DAMAGE_EVIL] = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETFIREMODIFIER:
            ptarget->damage_modifier[DAMAGE_FIRE] = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETICEMODIFIER:
            ptarget->damage_modifier[DAMAGE_ICE] = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETZAPMODIFIER:
            ptarget->damage_modifier[DAMAGE_ZAP] = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETFLASHINGAND:
            ptarget->flashand = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETLIGHTBLEND:
            ptarget->setLight(penc->_set[value_idx]._oldValue);
            break;

        case eve_t::SETALPHABLEND:
            ptarget->setAlpha(penc->_set[value_idx]._oldValue);
            break;

        case eve_t::SETSHEEN:
            ptarget->setSheen(penc->_set[value_idx]._oldValue);
            break;

        case eve_t::SETFLYTOHEIGHT:
            ptarget->flyheight = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETWALKONWATER:
            ptarget->waterwalk = ( 0 != penc->_set[value_idx]._oldValue );
            break;

        case eve_t::SETCANSEEINVISIBLE:
            ptarget->see_invisible_level = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETMISSILETREATMENT:
            ptarget->missiletreatment = penc->_set[value_idx]._oldValue;
            break;

        case eve_t::SETCOSTFOREACHMISSILE:
            ptarget->missilecost = penc->_set[value_idx]._oldValue;
            ptarget->missilehandler = character;
            break;

        case eve_t::SETMORPH:
            // Need special handler for when this is removed
            change_character( character, ptarget->basemodel_ref, penc->_set[value_idx]._oldValue, ENC_LEAVE_ALL );
            break;

        case eve_t::SETCHANNEL:
            ptarget->canchannel = ( 0 != penc->_set[value_idx]._oldValue );
            break;
    }

    penc->_set[value_idx]._modified = false;
}

//--------------------------------------------------------------------------------------------
void enc_remove_add( const ENC_REF ienc, int value_idx )
{
    /// @author ZZ
    /// @details This function undoes cumulative modification to character stats

    float fvaluetoadd;
    int valuetoadd;
    CHR_REF character;
    enc_t * penc;
    Object * ptarget;

    if (value_idx < 0 || value_idx >= eve_t::MAX_ENCHANT_ADD) return;

    if ( !ALLOCATED_ENC( ienc ) ) return;
    penc = EnchantHandler::get().get_ptr( ienc );

    if ( !_gameObjects.exists( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = _gameObjects.get( penc->target_ref );

    if ( penc->_add[value_idx]._modified )
    {
        switch ( value_idx )
        {
        case eve_t::ADDJUMPPOWER:
                fvaluetoadd = penc->_add[value_idx]._oldValue;
                ptarget->jump_power -= fvaluetoadd;
                break;

            case eve_t::ADDBUMPDAMPEN:
                fvaluetoadd = penc->_add[value_idx]._oldValue;
                ptarget->phys.bumpdampen -= fvaluetoadd;
                break;

            case eve_t::ADDBOUNCINESS:
                fvaluetoadd = penc->_add[value_idx]._oldValue;
                ptarget->phys.dampen -= fvaluetoadd;
                break;

            case eve_t::ADDDAMAGE:
                valuetoadd = penc->_add[value_idx]._oldValue;
                ptarget->damage_boost -= valuetoadd;
                break;

            case eve_t::ADDSIZE:
                fvaluetoadd = penc->_add[value_idx]._oldValue;
                ptarget->fat_goto -= fvaluetoadd;
                ptarget->fat_goto_time = SIZETIME;
                break;

            case eve_t::ADDACCEL:
                fvaluetoadd = penc->_add[value_idx]._oldValue;
                chr_set_maxaccel( ptarget, ptarget->maxaccel_reset - fvaluetoadd );
                break;

            case eve_t::ADDRED:
                valuetoadd = penc->_add[value_idx]._oldValue;
                chr_set_redshift( ptarget, ptarget->inst.redshift - valuetoadd );
                break;

            case eve_t::ADDGRN:
                valuetoadd = penc->_add[value_idx]._oldValue;
                chr_set_grnshift( ptarget, ptarget->inst.grnshift - valuetoadd );
                break;

            case eve_t::ADDBLU:
                valuetoadd = penc->_add[value_idx]._oldValue;
                chr_set_blushift( ptarget, ptarget->inst.blushift - valuetoadd );
                break;

            case eve_t::ADDDEFENSE:
                {
                    int def_val;
                    valuetoadd = penc->_add[value_idx]._oldValue;
                    def_val = ptarget->defense - valuetoadd;
                    ptarget->defense = std::max( 0, def_val );
                }
                break;

            case eve_t::ADDMANA:
                valuetoadd = penc->_add[value_idx]._oldValue;
                ptarget->mana_max -= valuetoadd;
                ptarget->mana -= valuetoadd;
                if ( ptarget->mana < 0 ) ptarget->mana = 0;
                break;

            case eve_t::ADDLIFE:
                valuetoadd = penc->_add[value_idx]._oldValue;
                ptarget->life_max -= valuetoadd;
                ptarget->life -= valuetoadd;
                if ( ptarget->life < 1 ) ptarget->life = 1;
                break;

            case eve_t::ADDSTRENGTH:
                valuetoadd = penc->_add[value_idx]._oldValue;
                ptarget->strength -= valuetoadd;
                break;

            case eve_t::ADDWISDOM:
                valuetoadd = penc->_add[value_idx]._oldValue;
                ptarget->wisdom -= valuetoadd;
                break;

            case eve_t::ADDINTELLIGENCE:
                valuetoadd = penc->_add[value_idx]._oldValue;
                ptarget->intelligence -= valuetoadd;
                break;

            case eve_t::ADDDEXTERITY:
                valuetoadd = penc->_add[value_idx]._oldValue;
                ptarget->dexterity -= valuetoadd;
                break;

            case eve_t::ADDSLASHRESIST:
                ptarget->damage_resistance[DAMAGE_SLASH] -= penc->_add[value_idx]._oldValue;
                break;

            case eve_t::ADDCRUSHRESIST:
                ptarget->damage_resistance[DAMAGE_CRUSH] -= penc->_add[value_idx]._oldValue;
                break;

            case eve_t::ADDPOKERESIST:
                ptarget->damage_resistance[DAMAGE_POKE]  -= penc->_add[value_idx]._oldValue;
                break;

            case eve_t::ADDHOLYRESIST:
                ptarget->damage_resistance[DAMAGE_HOLY] -= penc->_add[value_idx]._oldValue;
                break;

            case eve_t::ADDEVILRESIST:
                ptarget->damage_resistance[DAMAGE_EVIL] -= penc->_add[value_idx]._oldValue;
                break;

            case eve_t::ADDFIRERESIST:
                ptarget->damage_resistance[DAMAGE_FIRE] -= penc->_add[value_idx]._oldValue;
                break;

            case eve_t::ADDICERESIST:
                ptarget->damage_resistance[DAMAGE_ICE] -= penc->_add[value_idx]._oldValue;
                break;

            case eve_t::ADDZAPRESIST:
                ptarget->damage_resistance[DAMAGE_ZAP] -= penc->_add[value_idx]._oldValue;
                break;
        }

        penc->_add[value_idx]._modified = false;
    }
}

//--------------------------------------------------------------------------------------------
void update_all_enchants()
{
    ENC_REF ienc;

    // update all enchants
    for ( ienc = 0; ienc < ENCHANTS_MAX; ienc++ )
    {
        enc_t::run_config( EnchantHandler::get().get_ptr( ienc ) );
    }

    // fix the stat timer
    if ( clock_enc_stat >= ONESECOND )
    {
        // Reset the clock
        clock_enc_stat -= ONESECOND;
    }
}

//--------------------------------------------------------------------------------------------
ENC_REF cleanup_enchant_list( const ENC_REF ienc, ENC_REF * enc_parent )
{
    /// @author BB
    /// @details remove all the dead enchants from the enchant list
    ///     and report back the first non-dead enchant in the list.

    bool enc_used[ENCHANTS_MAX];

    ENC_REF first_valid_enchant;

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;

    if ( !VALID_ENC_RANGE( ienc ) ) return ENCHANTS_MAX;

    // clear the list
    BLANK_ARY( enc_used )

    // scan the list of enchants
    ienc_nxt            = INVALID_ENC_REF;
    first_valid_enchant = ienc_now = ienc;
    ienc_count = 0;
    while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
    {
        ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

        // coerce the list of enchants to a valid value
        if ( !VALID_ENC_RANGE( ienc_nxt ) )
        {
            ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref = INVALID_ENC_REF;
        }

        // fix any loops in the enchant list
        if ( enc_used[ienc_nxt] )
        {
            EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref = INVALID_ENC_REF;
            break;
        }

        //( !_gameObjects.exists( EncList.lst[ienc_now].target_ref ) && !EveStack.lst[EncList.lst[ienc_now].eve_ref].stayiftargetdead )

        // remove any expired enchants
        if ( !INGAME_ENC( ienc_now ) )
        {
            remove_enchant( ienc_now, enc_parent );
            enc_used[ienc_now] = true;
        }
        else
        {
            // store this enchant in the list of used enchants
            enc_used[ienc_now] = true;

            // keep track of the first valid enchant
            if ( INVALID_ENC_REF == first_valid_enchant )
            {
                first_valid_enchant = ienc_now;
            }
        }

        enc_parent = &(EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref );
        ienc_now    = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    return first_valid_enchant;
}

//--------------------------------------------------------------------------------------------
void cleanup_all_enchants()
{
    /// @author ZZ
    /// @details this function scans all the enchants and removes any dead ones.
    ///               this happens only once a loop

    ENC_BEGIN_LOOP_ACTIVE( ienc, penc )
    {
        ENC_REF * enc_lst;
        eve_t   * peve;
        bool    do_remove;
        bool valid_owner, valid_target;

        // try to determine something about the parent
        enc_lst = NULL;
        valid_target = false;
        if ( _gameObjects.exists( penc->target_ref ) )
        {
            valid_target = _gameObjects.get(penc->target_ref)->alive;

            // this is linked to a known character
            enc_lst = &( _gameObjects.get(penc->target_ref)->firstenchant );
        }

        //try to determine if the owner exists and is alive
        valid_owner = false;
        if ( _gameObjects.exists( penc->owner_ref ) )
        {
            valid_owner = _gameObjects.get(penc->owner_ref)->alive;
        }

        if ( !LOADED_EVE( penc->eve_ref ) )
        {
            // this should never happen
            EGOBOO_ASSERT( false );
            continue;
        }
        peve = EveStack.get_ptr( penc->eve_ref );

        do_remove = false;
        if ( WAITING_PBASE( POBJ_GET_PBASE( penc ) ) )
        {
            // the enchant has been marked for removal
            do_remove = true;
        }
        else if ( !valid_owner && !peve->_owner._stay )
        {
            // the enchant's owner has died
            do_remove = true;
        }
        else if ( !valid_target && !peve->_target._stay )
        {
            // the enchant's target has died
            do_remove = true;
        }
        else if ( valid_owner && peve->endIfCannotPay )
        {
            // Undo enchants that cannot be sustained anymore
            if ( 0 == _gameObjects.get(penc->owner_ref)->mana ) do_remove = true;
        }
        else
        {
            // the enchant has timed out
            do_remove = ( 0 == penc->lifetime );
        }

        if ( do_remove )
        {
            remove_enchant( ienc, NULL );
        }
    }
    ENC_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void bump_all_enchants_update_counters()
{
    for (ENC_REF ref = 0; ref < ENCHANTS_MAX; ++ref)
    {
        Ego::Entity *entity = POBJ_GET_PBASE( EnchantHandler::get().get_ptr(ref));
        if (!ACTIVE_PBASE(entity)) continue;

        entity->update_count++;
    }
}

//--------------------------------------------------------------------------------------------
bool enc_t::request_terminate( enc_t * penc )
{
    if ( NULL == penc || !ALLOCATED_PENC( penc ) || TERMINATED_PENC( penc ) ) return false;

    POBJ_REQUEST_TERMINATE( penc );

    return true;
}

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION (inline)
//--------------------------------------------------------------------------------------------
CHR_REF enc_get_iowner( const ENC_REF ienc )
{
    if ( !DEFINED_ENC( ienc ) ) return INVALID_CHR_REF;
    enc_t *penc = EnchantHandler::get().get_ptr(ienc);

    if ( !_gameObjects.exists( penc->owner_ref ) ) return INVALID_CHR_REF;

    return penc->owner_ref;
}

//--------------------------------------------------------------------------------------------
Object * enc_get_powner(const ENC_REF ienc)
{
    if (!DEFINED_ENC(ienc)) return nullptr;
    enc_t *penc = EnchantHandler::get().get_ptr(ienc);

    if (!_gameObjects.exists(penc->owner_ref)) return nullptr;

    return _gameObjects.get(penc->owner_ref);
}

//--------------------------------------------------------------------------------------------
EVE_REF enc_get_ieve(const ENC_REF ienc)
{
    if (!DEFINED_ENC(ienc)) return INVALID_EVE_REF;
    enc_t *penc = EnchantHandler::get().get_ptr(ienc);

    if (!LOADED_EVE(penc->eve_ref)) return INVALID_EVE_REF;

    return penc->eve_ref;
}

//--------------------------------------------------------------------------------------------
eve_t *enc_get_peve(const ENC_REF ienc)
{
    if (!DEFINED_ENC(ienc)) return nullptr;
    enc_t *penc = EnchantHandler::get().get_ptr(ienc);

    if (!LOADED_EVE(penc->eve_ref)) return nullptr;

    return EveStack.get_ptr(penc->eve_ref);
}

//--------------------------------------------------------------------------------------------
PRO_REF  enc_get_ipro(const ENC_REF ienc)
{
    if (!DEFINED_ENC(ienc)) return INVALID_PRO_REF;
    enc_t *penc = EnchantHandler::get().get_ptr(ienc);

    if (!ProfileSystem::get().isValidProfileID(penc->profile_ref)) return INVALID_PRO_REF;

    return penc->profile_ref;
}

//--------------------------------------------------------------------------------------------
ObjectProfile * enc_get_ppro( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return NULL;
    penc = EnchantHandler::get().get_ptr( ienc );

    if (!ProfileSystem::get().isValidProfileID(penc->profile_ref)) return NULL;

    return ProfileSystem::get().getProfile(penc->profile_ref).get();
}

//--------------------------------------------------------------------------------------------
IDSZ enc_get_idszremove( const ENC_REF ienc )
{
    eve_t * peve = enc_get_peve( ienc );
    if ( NULL == peve ) return IDSZ_NONE;

    return peve->removedByIDSZ;
}

//--------------------------------------------------------------------------------------------
bool enc_is_removed( const ENC_REF ienc, const PRO_REF test_profile )
{
    IDSZ idsz_remove;

    if ( !INGAME_ENC( ienc ) ) return false;
    idsz_remove = enc_get_idszremove( ienc );

    // if nothing can remove it, just go on with your business
    if ( IDSZ_NONE == idsz_remove ) return false;

    // check vs. every IDSZ that could have something to do with cancelling the enchant
    if ( idsz_remove == enc_get_ppro(ienc)->getIDSZ(IDSZ_TYPE) ) return true;
    if ( idsz_remove == enc_get_ppro(ienc)->getIDSZ(IDSZ_PARENT) ) return true;

    return false;
}

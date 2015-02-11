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

/// @file game/enchant.c
/// @brief handles enchantments attached to objects
/// @details

#include "game/enchant.h"

#include "game/audio/AudioSystem.hpp"
#include "game/game.h"
#include "game/script_functions.h"
#include "game/egoboo.h"

#include "game/char.h"
#include "game/mad.h"
#include "game/particle.h"
#include "game/profiles/Profile.hpp"
#include "game/profiles/ProfileSystem.hpp"

#include "game/EncList.h"
#include "game/module/ObjectHandler.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Stack<eve_t, MAX_EVE> EveStack;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool enc_free(enc_t *self);

static enc_t *enc_config_ctor(enc_t *self);
static enc_t *enc_config_init(enc_t *self);
static enc_t *enc_config_deinit(enc_t *self);
static enc_t *enc_config_active(enc_t *self);
static enc_t *enc_config_dtor(enc_t *self);

static enc_t *enc_config_do_active(enc_t *self);
static enc_t *enc_config_do_init(enc_t *self);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool unlink_enchant( const ENC_REF ienc, ENC_REF * enc_parent );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void enchant_system_begin()
{
    EnchantManager::ctor();
    EveStack_init_all();
}

//--------------------------------------------------------------------------------------------
void enchant_system_end()
{
    EveStack_release_all();
    EnchantManager::dtor();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool enc_free(enc_t *self)
{
    if (!ALLOCATED_PENC(self)) return false;

    // nothing to do yet
    return true;
}

//--------------------------------------------------------------------------------------------
enc_t *enc_t::ctor(enc_t *self)
{
    // grab the base object
    if (!self) return nullptr;
    Ego::Entity *pbase = POBJ_GET_PBASE(self);

	// Save the entity data.
	Ego::Entity save_base;
    memcpy(&save_base, pbase, sizeof(Ego::Entity));

    BLANK_STRUCT_PTR(self);

    // Restore the entity data.
	memcpy(pbase, &save_base, sizeof(Ego::Entity));

    // reset the base counters
    pbase->update_count = 0;
    pbase->frame_count = 0;

    self->profile_ref = INVALID_PRO_REF;
    self->eve_ref = INVALID_EVE_REF;

    self->target_ref = INVALID_CHR_REF;
    self->owner_ref = INVALID_CHR_REF;
    self->spawner_ref = INVALID_CHR_REF;
    self->spawnermodel_ref = INVALID_PRO_REF;
    self->overlay_ref = INVALID_CHR_REF;

    self->nextenchant_ref = INVALID_ENC_REF;

    // we are done constructing. move on to initializing.
    pbase->state = Ego::Entity::State::Initializing;

    return self;
}

//--------------------------------------------------------------------------------------------
enc_t *enc_t::dtor(enc_t * self)
{
    if (!self) return nullptr;

    // destroy the object
    enc_free(self);

    // Destroy the base object.
    // Sets the state to ego_object_terminated automatically.
    POBJ_TERMINATE(self);

    return self;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool unlink_enchant( const ENC_REF ienc, ENC_REF * enc_parent )
{
    enc_t * penc;

    if ( !ALLOCATED_ENC( ienc ) ) return false;
    penc = EncList.get_ptr( ienc );

    // Unlink it from the spawner (if possible)
    if ( _gameObjects.exists( penc->spawner_ref ) )
    {
        GameObject * pspawner = _gameObjects.get( penc->spawner_ref );

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

        GameObject * ptarget;

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
            while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
            {
                ienc_last = ienc_now;
                ienc_nxt  = EncList.lst[ienc_now].nextenchant_ref;

                if ( ienc_now == ienc ) break;

                ienc_now  = ienc_nxt;
                ienc_count++;
            }
            if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

            // Relink the last enchantment
            if ( ienc_now == ienc )
            {
                enc_parent = &( EncList.lst[ienc_last].nextenchant_ref );
            }
        }
    }

    // unlink the enchant from the parent reference
    if ( NULL != enc_parent )
    {
        *enc_parent = EncList.lst[ienc].nextenchant_ref;
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
    GameObject *pchr;

    // Stop invalid pointers
    if ( !_gameObjects.exists( ichr ) ) return false;
    pchr = _gameObjects.get( ichr );

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pchr );

    // Check all enchants to see if they are removed
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt  = EncList.lst[ienc_now].nextenchant_ref;

        peve = enc_get_peve( ienc_now );
        if ( NULL != peve && ( IDSZ_NONE == remove_idsz || remove_idsz == peve->removedbyidsz ) )
        {
            remove_enchant( ienc_now, NULL );
            retval = true;
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

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
    GameObject * target_ptr, *spawner_ptr, *overlay_ptr;

    if ( !ALLOCATED_ENC( ienc ) ) return false;
    penc = EncList.get_ptr( ienc );
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
    for ( add_type = ENC_ADD_LAST; add_type >= ENC_ADD_FIRST; add_type-- )
    {
        enc_remove_add( ienc, add_type );
    }

    // unset them in the reverse order of setting them, doing morph last
    for ( set_type = ENC_SET_LAST; set_type >= ENC_SET_FIRST; set_type-- )
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
        if ( _profileSystem.isValidProfileID( imodel ) )
        {
            iwave = peve->endsound_index;
            if ( nullptr != target_ptr )
            {
                _audioSystem.playSound( target_ptr->pos_old, _profileSystem.getProfile(imodel)->getSoundID(iwave) );
            }
            else
            {
                _audioSystem.playSoundFull( _profileSystem.getProfile(imodel)->getSoundID(iwave) );
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
            if ( 0 != peve->seekurse )
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

    EncList_free_one( ienc );

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
    /// @details This function returns MAX_ENC if the enchantment's target has no conflicting
    ///    set values in its other enchantments.  Otherwise it returns the ienc
    ///    of the conflicting enchantment

    CHR_REF character;
    GameObject * pchr;

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return INVALID_ENC_REF;

    if ( !INGAME_ENC( ienc ) ) return INVALID_ENC_REF;

    character = EncList.lst[ienc].target_ref;
    if ( !_gameObjects.exists( character ) ) return INVALID_ENC_REF;
    pchr = _gameObjects.get( character );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // scan the enchant list
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        if ( INGAME_ENC( ienc_now ) && EncList.lst[ienc_now].setyesno[value_idx] )
        {
            break;
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

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
    GameObject * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return;

    if ( !DEFINED_ENC( ienc ) ) return;
    penc = EncList.get_ptr( ienc );

    peve = _profileSystem.pro_get_peve( profile );
    if ( NULL == peve ) return;

    penc->setyesno[value_idx] = false;
    if ( peve->setyesno[value_idx] )
    {
        conflict = enc_value_filled( ienc, value_idx );
        if ( peve->override || INVALID_ENC_REF == conflict )
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

                penc->setyesno[value_idx] = true;

                switch ( value_idx )
                {
                    case SETDAMAGETYPE:
                        penc->setsave[value_idx]  = ptarget->damagetarget_damagetype;
                        ptarget->damagetarget_damagetype = peve->setvalue[value_idx];
                        break;

                    case SETNUMBEROFJUMPS:
                        penc->setsave[value_idx] = ptarget->jumpnumberreset;
                        ptarget->jumpnumberreset = peve->setvalue[value_idx];
                        break;

                    case SETLIFEBARCOLOR:
                        penc->setsave[value_idx] = ptarget->life_color;
                        ptarget->life_color       = peve->setvalue[value_idx];
                        break;

                    case SETMANABARCOLOR:
                        penc->setsave[value_idx] = ptarget->mana_color;
                        ptarget->mana_color       = peve->setvalue[value_idx];
                        break;

                    case SETSLASHMODIFIER:
                        penc->setsave[value_idx]              = ptarget->damage_modifier[DAMAGE_SLASH];
                        ptarget->damage_modifier[DAMAGE_SLASH] = peve->setvalue[value_idx];
                        break;

                    case SETCRUSHMODIFIER:
                        penc->setsave[value_idx]              = ptarget->damage_modifier[DAMAGE_CRUSH];
                        ptarget->damage_modifier[DAMAGE_CRUSH] = peve->setvalue[value_idx];
                        break;

                    case SETPOKEMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damage_modifier[DAMAGE_POKE];
                        ptarget->damage_modifier[DAMAGE_POKE] = peve->setvalue[value_idx];
                        break;

                    case SETHOLYMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damage_modifier[DAMAGE_HOLY];
                        ptarget->damage_modifier[DAMAGE_HOLY] = peve->setvalue[value_idx];
                        break;

                    case SETEVILMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damage_modifier[DAMAGE_EVIL];
                        ptarget->damage_modifier[DAMAGE_EVIL] = peve->setvalue[value_idx];
                        break;

                    case SETFIREMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damage_modifier[DAMAGE_FIRE];
                        ptarget->damage_modifier[DAMAGE_FIRE] = peve->setvalue[value_idx];
                        break;

                    case SETICEMODIFIER:
                        penc->setsave[value_idx]            = ptarget->damage_modifier[DAMAGE_ICE];
                        ptarget->damage_modifier[DAMAGE_ICE] = peve->setvalue[value_idx];
                        break;

                    case SETZAPMODIFIER:
                        penc->setsave[value_idx]            = ptarget->damage_modifier[DAMAGE_ZAP];
                        ptarget->damage_modifier[DAMAGE_ZAP] = peve->setvalue[value_idx];
                        break;

                    case SETFLASHINGAND:
                        penc->setsave[value_idx] = ptarget->flashand;
                        ptarget->flashand        = peve->setvalue[value_idx];
                        break;

                    case SETLIGHTBLEND:
                        penc->setsave[value_idx] = ptarget->inst.light;
                        ptarget->setLight(peve->setvalue[value_idx]);
                        break;

                    case SETALPHABLEND:
                        penc->setsave[value_idx] = ptarget->inst.alpha;
                        ptarget->setAlpha(peve->setvalue[value_idx]);
                        break;

                    case SETSHEEN:
                        penc->setsave[value_idx] = ptarget->inst.sheen;
                        ptarget->setSheen(peve->setvalue[value_idx]);
                        break;

                    case SETFLYTOHEIGHT:
                        penc->setsave[value_idx] = ptarget->flyheight;
                        if ( 0 == ptarget->flyheight && ptarget->getPosZ() > -2 )
                        {
                            ptarget->flyheight = peve->setvalue[value_idx];
                        }
                        break;

                    case SETWALKONWATER:
                        penc->setsave[value_idx] = ptarget->waterwalk;
                        if ( !ptarget->waterwalk )
                        {
                            ptarget->waterwalk = ( 0 != peve->setvalue[value_idx] );
                        }
                        break;

                    case SETCANSEEINVISIBLE:
                        penc->setsave[value_idx]     = ptarget->see_invisible_level > 0;
                        ptarget->see_invisible_level = peve->setvalue[value_idx];
                        break;

                    case SETMISSILETREATMENT:
                        penc->setsave[value_idx]  = ptarget->missiletreatment;
                        ptarget->missiletreatment = peve->setvalue[value_idx];
                        break;

                    case SETCOSTFOREACHMISSILE:
                        penc->setsave[value_idx] = ptarget->missilecost;
                        ptarget->missilecost     = peve->setvalue[value_idx] * 16.0f;    // adjustment to the value stored in the file
                        ptarget->missilehandler  = penc->owner_ref;
                        break;

                    case SETMORPH:
                        // Special handler for morph
                        penc->setsave[value_idx] = ptarget->skin;
                        change_character( character, profile, 0, ENC_LEAVE_ALL ); // ENC_LEAVE_FIRST);
                        break;

                    case SETCHANNEL:
                        penc->setsave[value_idx] = ptarget->canchannel;
                        ptarget->canchannel      = ( 0 != peve->setvalue[value_idx] );
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
    GameObject * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_ADD ) return;

    if ( !DEFINED_ENC( ienc ) ) return;
    penc = EncList.get_ptr( ienc );

    if ( ieve >= MAX_EVE || !EveStack.lst[ieve].loaded ) return;
    peve = EveStack.get_ptr( ieve );

    if ( !peve->addyesno[value_idx] )
    {
        penc->addyesno[value_idx] = false;
        penc->addsave[value_idx]  = 0.0f;
        return;
    }

    if ( !_gameObjects.exists( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = _gameObjects.get( character );

    valuetoadd  = 0;
    fvaluetoadd = 0.0f;
    switch ( value_idx )
    {
        case ADDJUMPPOWER:
            fnewvalue = ptarget->jump_power;
            fvaluetoadd = peve->addvalue[value_idx];
            getadd_flt( 0.0f, fnewvalue, 30.0f, &fvaluetoadd );
            ptarget->jump_power += fvaluetoadd;
            break;

        case ADDBUMPDAMPEN:
            fnewvalue = ptarget->phys.bumpdampen;
            fvaluetoadd = peve->addvalue[value_idx];
            getadd_flt( 0.0f, fnewvalue, 1.0f, &fvaluetoadd );
            ptarget->phys.bumpdampen += fvaluetoadd;
            break;

        case ADDBOUNCINESS:
            fnewvalue = ptarget->phys.dampen;
            fvaluetoadd = peve->addvalue[value_idx];
            getadd_flt( 0.0f, fnewvalue, 0.95f, &fvaluetoadd );
            ptarget->phys.dampen += fvaluetoadd;
            break;

        case ADDDAMAGE:
            newvalue = ptarget->damage_boost;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( 0, newvalue, 4096, &valuetoadd );
            ptarget->damage_boost += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDSIZE:
            fnewvalue = ptarget->fat_goto;
            fvaluetoadd = peve->addvalue[value_idx];
            getadd_flt( 0.5f, fnewvalue, 2.0f, &fvaluetoadd );
            ptarget->fat_goto += fvaluetoadd;
            ptarget->fat_goto_time = SIZETIME;
            break;

        case ADDACCEL:
            fnewvalue = ptarget->maxaccel_reset;
            fvaluetoadd = peve->addvalue[value_idx];
            getadd_flt( 0.0f, fnewvalue, 1.50f, &fvaluetoadd );
            chr_set_maxaccel( ptarget, ptarget->maxaccel_reset + fvaluetoadd );
            break;

        case ADDRED:
            newvalue = ptarget->inst.redshift;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( 0, newvalue, 6, &valuetoadd );
            chr_set_redshift( ptarget, ptarget->inst.redshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDGRN:
            newvalue = ptarget->inst.grnshift;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( 0, newvalue, 6, &valuetoadd );
            chr_set_grnshift( ptarget, ptarget->inst.grnshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDBLU:
            newvalue = ptarget->inst.blushift;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( 0, newvalue, 6, &valuetoadd );
            chr_set_blushift( ptarget, ptarget->inst.blushift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDDEFENSE:
            newvalue = ptarget->defense;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( 55, newvalue, 255, &valuetoadd );  // Don't fix again! /// @note ZF@> why limit min to 55?
            ptarget->defense = ptarget->defense + valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDMANA:
            newvalue = ptarget->mana_max;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( 0, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->mana_max += valuetoadd;
            //ptarget->mana    += valuetoadd;                       /// @note ZF@> bit of a problem here, we dont want players to heal or lose life by requipping magic ornaments
            ptarget->mana = CLIP( (UFP8_T)ptarget->mana, (UFP8_T)0, ptarget->mana_max );
            fvaluetoadd = valuetoadd;
            break;

        case ADDLIFE:
            newvalue = ptarget->life_max;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( LOWSTAT, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->life_max += valuetoadd;
            //ptarget->life += valuetoadd;                        /// @note ZF@> bit of a problem here, we dont want players to heal or lose life by requipping magic ornaments
            ptarget->life = CLIP( (UFP8_T)ptarget->life, (UFP8_T)1, ptarget->life_max );
            fvaluetoadd = valuetoadd;
            break;

        case ADDSTRENGTH:
            newvalue = ptarget->strength;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->strength += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDWISDOM:
            newvalue = ptarget->wisdom;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->wisdom += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDINTELLIGENCE:
            newvalue = ptarget->intelligence;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->intelligence += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDDEXTERITY:
            newvalue = ptarget->dexterity;
            valuetoadd = peve->addvalue[value_idx];
            getadd_int( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->dexterity += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDSLASHRESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_SLASH];
            fvaluetoadd = peve->addvalue[value_idx];
            ptarget->damage_resistance[DAMAGE_SLASH] += fvaluetoadd;
            break;

        case ADDCRUSHRESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_CRUSH];
            fvaluetoadd = peve->addvalue[value_idx];
            ptarget->damage_resistance[DAMAGE_CRUSH] += fvaluetoadd;
            break;

        case ADDPOKERESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_POKE];
            fvaluetoadd = peve->addvalue[value_idx];
            ptarget->damage_resistance[DAMAGE_POKE] += fvaluetoadd;
            break;

        case ADDHOLYRESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_HOLY];
            fvaluetoadd = peve->addvalue[value_idx];
            ptarget->damage_resistance[DAMAGE_HOLY] += fvaluetoadd;
            break;

        case ADDEVILRESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_EVIL];
            fvaluetoadd = peve->addvalue[value_idx];
            ptarget->damage_resistance[DAMAGE_EVIL] += fvaluetoadd;
            break;

        case ADDFIRERESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_FIRE];
            fvaluetoadd = peve->addvalue[value_idx];
            ptarget->damage_resistance[DAMAGE_FIRE] += fvaluetoadd;
            break;

        case ADDICERESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_ICE];
            fvaluetoadd = peve->addvalue[value_idx];
            ptarget->damage_resistance[DAMAGE_ICE] += fvaluetoadd;
            break;

        case ADDZAPRESIST:
            fnewvalue = ptarget->damage_resistance[DAMAGE_ZAP];
            fvaluetoadd = peve->addvalue[value_idx];
            ptarget->damage_resistance[DAMAGE_ZAP] += fvaluetoadd;
            break;
    }

    // save whether there was any change in the value
    penc->addyesno[value_idx] = ( 0.0f != fvaluetoadd );

    // Save the value for undo
    penc->addsave[value_idx]  = fvaluetoadd;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enc_t * enc_config_do_init( enc_t * penc )
{
    enc_spawn_data_t * pdata;
    ENC_REF ienc;
    CHR_REF overlay;
    float lifetime;

    eve_t * peve;
    GameObject * ptarget;

    int add_type, set_type;

    if ( NULL == penc ) return NULL;
    ienc  = GET_INDEX_PENC( penc );

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
    POBJ_ACTIVATE( penc, peve->name );

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
    penc->target_mana  = peve->target_mana;
    penc->target_life  = peve->target_life;

    // does the owner exist?
    if ( !_gameObjects.exists( pdata->owner_ref ) )
    {
        penc->owner_ref = INVALID_CHR_REF;
    }
    else
    {
        penc->owner_ref  = pdata->owner_ref;
    }
    penc->owner_mana = peve->owner_mana;
    penc->owner_life = peve->owner_life;

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
    for ( set_type = ENC_SET_FIRST; set_type <= ENC_SET_LAST; set_type++ )
    {
        enc_apply_set( ienc, set_type, pdata->profile_ref );
    }

    // Now do all of the stat adds
    for ( add_type = ENC_ADD_FIRST; add_type <= ENC_ADD_LAST; add_type++ )
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
            GameObject *povl;
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
        if ( 0 != peve->seekurse )
        {
            ptarget->see_kurse_level = peve->seekurse;
        }

        // Allow them to see in darkness (or blindness if negative)
        if ( 0 != peve->darkvision )
        {
            ptarget->darkvision_level = peve->darkvision;
        }

    }

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_do_active( enc_t * penc )
{
    /// @author ZZ
    /// @details This function allows enchantments to update, spawn particles,
    ///  do drains, stat boosts and despawn.

    ENC_REF  ienc;
    CHR_REF  owner, target;
    EVE_REF  eve;
    eve_t * peve;
    GameObject * ptarget;

    if ( NULL == penc ) return penc;
    ienc = GET_REF_PENC( penc );

    // the following functions should not be done the first time through the update loop
    if ( 0 == clock_wld ) return penc;

    peve = enc_get_peve( ienc );
    if ( NULL == peve ) return penc;

    // check to see whether the enchant needs to spawn some particles
    if ( penc->spawn_timer > 0 ) penc->spawn_timer--;

    if ( 0 == penc->spawn_timer && peve->contspawn_amount <= 0 )
    {
        int      tnc;
        FACING_T facing;
        penc->spawn_timer = peve->contspawn_delay;
        ptarget = _gameObjects.get( penc->target_ref );

        facing = ptarget->ori.facing_z;
        for ( tnc = 0; tnc < peve->contspawn_amount; tnc++ )
        {
            spawn_one_particle( ptarget->getPosition(), facing, penc->profile_ref, peve->contspawn_lpip,
                                INVALID_CHR_REF, GRIP_LAST, chr_get_iteam( penc->owner_ref ), penc->owner_ref, INVALID_PRT_REF, tnc, INVALID_CHR_REF );

            facing += peve->contspawn_facingadd;
        }
    }

    // Do enchant drains and regeneration
    if ( clock_enc_stat >= ONESECOND )
    {
        if ( 0 == penc->lifetime )
        {
            EncList_request_terminate( ienc );
        }
        else
        {
            // Do enchant timer
            if ( penc->lifetime > 0 ) penc->lifetime--;

            // To make life easier
            owner  = enc_get_iowner( ienc );
            target = penc->target_ref;
            eve    = enc_get_ieve( ienc );
            GameObject *powner = _gameObjects.get(owner);

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
                    if ( EveStack.lst[eve].endifcantpay && !mana_paid )
                    {
                        EncList_request_terminate( ienc );
                    }
                }

            }
            else if ( !EveStack.lst[eve].stayifnoowner )
            {
                EncList_request_terminate( ienc );
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
                        if ( EveStack.lst[eve].endifcantpay && !mana_paid )
                        {
                            EncList_request_terminate( ienc );
                        }
                    }

                }
                else if ( !EveStack.lst[eve].stayiftargetdead )
                {
                    EncList_request_terminate( ienc );
                }
            }
        }
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enc_t * enc_config_construct( enc_t * penc, int max_iterations )
{
    if ( NULL == penc ) return NULL;

    Ego::Entity *pbase = POBJ_GET_PBASE( penc );
    if ( !pbase->allocated ) return NULL;

    // if the enchant is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( Ego::Entity::State::Constructing + 1 ) )
    {
        enc_t * tmp_enc = enc_config_deconstruct( penc, max_iterations );
        if ( tmp_enc == penc ) return NULL;
    }

    int iterations = 0;
    while ( NULL != penc && pbase->state <= Ego::Entity::State::Constructing && iterations < max_iterations )
    {
        enc_t * ptmp = enc_run_config( penc );
        if ( ptmp != penc ) return NULL;
        iterations++;
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_initialize( enc_t * penc, int max_iterations )
{
    if ( NULL == penc ) return NULL;

    Ego::Entity *pbase = POBJ_GET_PBASE( penc );
    if ( !pbase->allocated ) return NULL;

    // if the enchant is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( Ego::Entity::State::Initializing + 1 ) )
    {
        enc_t * tmp_enc = enc_config_deconstruct( penc, max_iterations );
        if ( tmp_enc == penc ) return NULL;
    }

    int iterations = 0;
    while ( NULL != penc && pbase->state <= Ego::Entity::State::Initializing && iterations < max_iterations )
    {
        enc_t * ptmp = enc_run_config( penc );
        if ( ptmp != penc ) return NULL;
        iterations++;
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_activate( enc_t * penc, int max_iterations )
{
    if ( NULL == penc ) return NULL;

    Ego::Entity *pbase = POBJ_GET_PBASE( penc );
    if ( !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( Ego::Entity::State::Active + 1 ) )
    {
        enc_t * tmp_enc = enc_config_deconstruct( penc, max_iterations );
        if ( tmp_enc == penc ) return NULL;
    }

    int iterations = 0;
    while ( NULL != penc && pbase->state < Ego::Entity::State::Active && iterations < max_iterations )
    {
        enc_t * ptmp = enc_run_config( penc );
        if ( ptmp != penc ) return NULL;
        iterations++;
    }

    EGOBOO_ASSERT( pbase->state == Ego::Entity::State::Active );
    if ( pbase->state == Ego::Entity::State::Active )
    {
        EncList_push_used( GET_INDEX_PENC( penc ) );
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_deinitialize( enc_t * penc, int max_iterations )
{
    if ( NULL == penc ) return NULL;

    Ego::Entity *pbase = POBJ_GET_PBASE( penc );
    if ( !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deinitialize it
    if ( pbase->state > ( int )( Ego::Entity::State::DeInitializing + 1 ) )
    {
        return penc;
    }
    else if ( pbase->state < Ego::Entity::State::DeInitializing )
    {
        pbase->state = Ego::Entity::State::DeInitializing;
    }

    int iterations = 0;
    while ( NULL != penc && pbase->state <= Ego::Entity::State::DeInitializing && iterations < max_iterations )
    {
        enc_t * ptmp = enc_run_config( penc );
        if ( ptmp != penc ) return NULL;
        iterations++;
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_deconstruct( enc_t * penc, int max_iterations )
{
    if ( NULL == penc ) return NULL;

    Ego::Entity *pbase = POBJ_GET_PBASE( penc );
    if ( !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it
    if ( pbase->state > ( int )( Ego::Entity::State::Destructing + 1 ) )
    {
        return penc;
    }
    else if ( pbase->state < Ego::Entity::State::DeInitializing )
    {
        // make sure that you deinitialize before destructing
        pbase->state = Ego::Entity::State::DeInitializing;
    }

	int iterations = 0;
    while ( NULL != penc && pbase->state <= Ego::Entity::State::Destructing && iterations < max_iterations )
    {
        enc_t * ptmp = enc_run_config( penc );
        if ( ptmp != penc ) return NULL;
        iterations++;
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enc_t * enc_run_config( enc_t * penc )
{
    if ( NULL == penc ) return NULL;

    Ego::Entity *pbase = POBJ_GET_PBASE( penc );
    if ( !pbase->allocated ) return NULL;

    // set the object to deinitialize if it is not "dangerous" and if was requested
    if ( pbase->kill_me )
    {
        if ( pbase->state > Ego::Entity::State::Constructing && pbase->state < Ego::Entity::State::DeInitializing )
        {
            pbase->state = Ego::Entity::State::DeInitializing;
        }

        pbase->kill_me = false;
    }

    switch ( pbase->state )
    {
        default:
		case Ego::Entity::State::Invalid:
            penc = NULL;
            break;

		case Ego::Entity::State::Constructing:
            penc = enc_config_ctor( penc );
            break;

		case Ego::Entity::State::Initializing:
            penc = enc_config_init( penc );
            break;

		case Ego::Entity::State::Active:
            penc = enc_config_active( penc );
            break;

		case Ego::Entity::State::DeInitializing:
            penc = enc_config_deinit( penc );
            break;

		case Ego::Entity::State::Destructing:
            penc = enc_config_dtor( penc );
            break;

		case Ego::Entity::State::Waiting:
		case Ego::Entity::State::Terminated:
            /* do nothing */
            break;
    }

    if ( NULL == penc )
    {
        pbase->update_guid = INVALID_UPDATE_GUID;
    }
    else if ( Ego::Entity::State::Active == pbase->state )
    {
        pbase->update_guid = EncList.update_guid;
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_ctor( enc_t * penc )
{
    // grab the base object
    if ( NULL == penc ) return NULL;
    Ego::Entity *pbase = POBJ_GET_PBASE( penc );

    // if we aren't in the correct state, abort.
    if ( !STATE_CONSTRUCTING_PBASE( pbase ) ) return penc;

    return enc_t::ctor( penc );
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_init( enc_t * penc )
{
    if ( NULL == penc ) return NULL;

    Ego::Entity *pbase = POBJ_GET_PBASE( penc );
    if ( !STATE_INITIALIZING_PBASE( pbase ) ) return penc;

    penc = enc_config_do_init( penc );
    if ( NULL == penc ) return NULL;

    if ( 0 == EncList.getLockCount() )
    {
        penc->obj_base.on = true;
    }
    else
    {
        EncList.add_activation( GET_INDEX_PENC( penc ) );
    }

    pbase->state = Ego::Entity::State::Active;

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_active( enc_t * penc )
{
    // there's nothing to configure if the object is active...
    if ( NULL == penc ) return NULL;

    Ego::Entity *pbase = POBJ_GET_PBASE( penc );
    if ( !pbase->allocated ) return NULL;

    if ( !STATE_ACTIVE_PBASE( pbase ) ) return penc;

    POBJ_END_SPAWN( penc );

    penc = enc_config_do_active( penc );

    return penc;
}

//--------------------------------------------------------------------------------------------
/// DeInitialize an enchantment.
enc_t * enc_config_deinit( enc_t * penc )
{
	if ( NULL == penc ) return NULL;
    Ego::Entity *pbase = POBJ_GET_PBASE( penc );

    if ( !STATE_DEINITIALIZING_PBASE( pbase ) ) return penc;

    POBJ_END_SPAWN( penc );

    pbase->state = Ego::Entity::State::Destructing;
    pbase->on    = false;

    return penc;
}

//--------------------------------------------------------------------------------------------
/// Destruct an enchantment.
enc_t * enc_config_dtor( enc_t * penc )
{
    if ( NULL == penc ) return NULL;
    Ego::Entity *pbase = POBJ_GET_PBASE( penc );

    if ( !STATE_DESTRUCTING_PBASE( pbase ) ) return penc;

    POBJ_END_SPAWN( penc );

    return enc_t::dtor( penc );
}

//--------------------------------------------------------------------------------------------
ENC_REF spawn_one_enchant( const CHR_REF owner, const CHR_REF target, const CHR_REF spawner, const ENC_REF enc_override, const PRO_REF modeloptional )
{
    /// @author ZZ
    /// @details This function enchants a target, returning the enchantment index or MAX_ENC
    ///    if failed

    ENC_REF enc_ref;
    EVE_REF eve_ref;

    eve_t * peve;
    enc_t * penc;
    GameObject * ptarget;

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

    if ( _profileSystem.isValidProfileID( modeloptional ) )
    {
        // The enchantment type is given explicitly
        loc_profile = modeloptional;
    }
    else
    {
        // The enchantment type is given by the spawner
        loc_profile = chr_get_ipro( spawner );

        if ( !_profileSystem.isValidProfileID( loc_profile ) )
        {
            log_warning( "spawn_one_enchant() - no valid profile for the spawning character \"%s\"(%d).\n", _gameObjects.get(spawner)->Name, REF_TO_INT( spawner ) );
            return INVALID_ENC_REF;
        }
    }

    eve_ref = _profileSystem.pro_get_ieve( loc_profile );
    if ( !LOADED_EVE( eve_ref ) )
    {
        log_warning( "spawn_one_enchant() - the object \"%s\"(%d) does not have an enchant profile.\n", _profileSystem.getProfile(loc_profile)->getFilePath().c_str(), REF_TO_INT( loc_profile ) );

        return INVALID_ENC_REF;
    }
    peve = EveStack.get_ptr( eve_ref );

    // count all the requests for this enchantment type
    peve->request_count++;

    // Owner must both be alive and on and valid if it isn't a stayifnoowner enchant
    if ( !peve->stayifnoowner && ( !_gameObjects.exists( owner ) || !_gameObjects.get(owner)->alive ) )
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
    enc_ref = EncList_allocate( enc_override );

    if ( !ALLOCATED_ENC( enc_ref ) )
    {
        log_warning( "spawn_one_enchant() - could not allocate an enchant.\n" );
        return INVALID_ENC_REF;
    }
    penc = EncList.get_ptr( enc_ref );

    POBJ_BEGIN_SPAWN( penc );

    penc->spawn_data.owner_ref   = owner;
    penc->spawn_data.target_ref  = loc_target;
    penc->spawn_data.spawner_ref = spawner;
    penc->spawn_data.profile_ref = loc_profile;
    penc->spawn_data.eve_ref     = eve_ref;

    // actually force the character to spawn
    penc = enc_config_activate( penc, 100 );

    // log all the successful spawns
    if ( NULL != penc )
    {
        POBJ_END_SPAWN( penc );
        peve->create_count++;
    }

    return enc_ref;
}

//--------------------------------------------------------------------------------------------
EVE_REF EveStack_losd_one( const char* szLoadName, const EVE_REF ieve )
{
    /// @author ZZ
    /// @details This function loads an enchantment profile into the EveStack

    EVE_REF retval = INVALID_EVE_REF;

    if ( VALID_EVE_RANGE( ieve ) )
    {
        eve_t * peve = EveStack.get_ptr( ieve );

        if ( NULL != load_one_enchant_file_vfs( szLoadName, peve ) )
        {
            retval = ieve;

            // limit the endsound_index
            peve->endsound_index = CLIP<Sint16>( peve->endsound_index, INVALID_SOUND_ID, MAX_WAVE );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void enc_remove_set( const ENC_REF ienc, int value_idx )
{
    /// @author ZZ
    /// @details This function unsets a set value
    CHR_REF character;
    enc_t * penc;
    GameObject * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return;

    if ( !ALLOCATED_ENC( ienc ) ) return;
    penc = EncList.get_ptr( ienc );

    if ( value_idx >= MAX_ENCHANT_SET || !penc->setyesno[value_idx] ) return;

    if ( !_gameObjects.exists( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget   = _gameObjects.get( penc->target_ref );

    switch ( value_idx )
    {
        case SETDAMAGETYPE:
            ptarget->damagetarget_damagetype = penc->setsave[value_idx];
            break;

        case SETNUMBEROFJUMPS:
            ptarget->jumpnumberreset = penc->setsave[value_idx];
            break;

        case SETLIFEBARCOLOR:
            ptarget->life_color = penc->setsave[value_idx];
            break;

        case SETMANABARCOLOR:
            ptarget->mana_color = penc->setsave[value_idx];
            break;

        case SETSLASHMODIFIER:
            ptarget->damage_modifier[DAMAGE_SLASH] = penc->setsave[value_idx];
            break;

        case SETCRUSHMODIFIER:
            ptarget->damage_modifier[DAMAGE_CRUSH] = penc->setsave[value_idx];
            break;

        case SETPOKEMODIFIER:
            ptarget->damage_modifier[DAMAGE_POKE] = penc->setsave[value_idx];
            break;

        case SETHOLYMODIFIER:
            ptarget->damage_modifier[DAMAGE_HOLY] = penc->setsave[value_idx];
            break;

        case SETEVILMODIFIER:
            ptarget->damage_modifier[DAMAGE_EVIL] = penc->setsave[value_idx];
            break;

        case SETFIREMODIFIER:
            ptarget->damage_modifier[DAMAGE_FIRE] = penc->setsave[value_idx];
            break;

        case SETICEMODIFIER:
            ptarget->damage_modifier[DAMAGE_ICE] = penc->setsave[value_idx];
            break;

        case SETZAPMODIFIER:
            ptarget->damage_modifier[DAMAGE_ZAP] = penc->setsave[value_idx];
            break;

        case SETFLASHINGAND:
            ptarget->flashand = penc->setsave[value_idx];
            break;

        case SETLIGHTBLEND:
            ptarget->setLight(penc->setsave[value_idx]);
            break;

        case SETALPHABLEND:
            ptarget->setAlpha(penc->setsave[value_idx]);
            break;

        case SETSHEEN:
            ptarget->setSheen(penc->setsave[value_idx]);
            break;

        case SETFLYTOHEIGHT:
            ptarget->flyheight = penc->setsave[value_idx];
            break;

        case SETWALKONWATER:
            ptarget->waterwalk = ( 0 != penc->setsave[value_idx] );
            break;

        case SETCANSEEINVISIBLE:
            ptarget->see_invisible_level = penc->setsave[value_idx];
            break;

        case SETMISSILETREATMENT:
            ptarget->missiletreatment = penc->setsave[value_idx];
            break;

        case SETCOSTFOREACHMISSILE:
            ptarget->missilecost = penc->setsave[value_idx];
            ptarget->missilehandler = character;
            break;

        case SETMORPH:
            // Need special handler for when this is removed
            change_character( character, ptarget->basemodel_ref, penc->setsave[value_idx], ENC_LEAVE_ALL );
            break;

        case SETCHANNEL:
            ptarget->canchannel = ( 0 != penc->setsave[value_idx] );
            break;
    }

    penc->setyesno[value_idx] = false;
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
    GameObject * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_ADD ) return;

    if ( !ALLOCATED_ENC( ienc ) ) return;
    penc = EncList.get_ptr( ienc );

    if ( !_gameObjects.exists( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = _gameObjects.get( penc->target_ref );

    if ( penc->addyesno[value_idx] )
    {
        switch ( value_idx )
        {
            case ADDJUMPPOWER:
                fvaluetoadd = penc->addsave[value_idx];
                ptarget->jump_power -= fvaluetoadd;
                break;

            case ADDBUMPDAMPEN:
                fvaluetoadd = penc->addsave[value_idx];
                ptarget->phys.bumpdampen -= fvaluetoadd;
                break;

            case ADDBOUNCINESS:
                fvaluetoadd = penc->addsave[value_idx];
                ptarget->phys.dampen -= fvaluetoadd;
                break;

            case ADDDAMAGE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->damage_boost -= valuetoadd;
                break;

            case ADDSIZE:
                fvaluetoadd = penc->addsave[value_idx];
                ptarget->fat_goto -= fvaluetoadd;
                ptarget->fat_goto_time = SIZETIME;
                break;

            case ADDACCEL:
                fvaluetoadd = penc->addsave[value_idx];
                chr_set_maxaccel( ptarget, ptarget->maxaccel_reset - fvaluetoadd );
                break;

            case ADDRED:
                valuetoadd = penc->addsave[value_idx];
                chr_set_redshift( ptarget, ptarget->inst.redshift - valuetoadd );
                break;

            case ADDGRN:
                valuetoadd = penc->addsave[value_idx];
                chr_set_grnshift( ptarget, ptarget->inst.grnshift - valuetoadd );
                break;

            case ADDBLU:
                valuetoadd = penc->addsave[value_idx];
                chr_set_blushift( ptarget, ptarget->inst.blushift - valuetoadd );
                break;

            case ADDDEFENSE:
                {
                    int def_val;
                    valuetoadd = penc->addsave[value_idx];
                    def_val = ptarget->defense - valuetoadd;
                    ptarget->defense = std::max( 0, def_val );
                }
                break;

            case ADDMANA:
                valuetoadd = penc->addsave[value_idx];
                ptarget->mana_max -= valuetoadd;
                ptarget->mana -= valuetoadd;
                if ( ptarget->mana < 0 ) ptarget->mana = 0;
                break;

            case ADDLIFE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->life_max -= valuetoadd;
                ptarget->life -= valuetoadd;
                if ( ptarget->life < 1 ) ptarget->life = 1;
                break;

            case ADDSTRENGTH:
                valuetoadd = penc->addsave[value_idx];
                ptarget->strength -= valuetoadd;
                break;

            case ADDWISDOM:
                valuetoadd = penc->addsave[value_idx];
                ptarget->wisdom -= valuetoadd;
                break;

            case ADDINTELLIGENCE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->intelligence -= valuetoadd;
                break;

            case ADDDEXTERITY:
                valuetoadd = penc->addsave[value_idx];
                ptarget->dexterity -= valuetoadd;
                break;

            case ADDSLASHRESIST:
                ptarget->damage_resistance[DAMAGE_SLASH] -= penc->addsave[value_idx];
                break;

            case ADDCRUSHRESIST:
                ptarget->damage_resistance[DAMAGE_CRUSH] -= penc->addsave[value_idx];
                break;

            case ADDPOKERESIST:
                ptarget->damage_resistance[DAMAGE_POKE]  -= penc->addsave[value_idx];
                break;

            case ADDHOLYRESIST:
                ptarget->damage_resistance[DAMAGE_HOLY] -= penc->addsave[value_idx];
                break;

            case ADDEVILRESIST:
                ptarget->damage_resistance[DAMAGE_EVIL] -= penc->addsave[value_idx];
                break;

            case ADDFIRERESIST:
                ptarget->damage_resistance[DAMAGE_FIRE] -= penc->addsave[value_idx];
                break;

            case ADDICERESIST:
                ptarget->damage_resistance[DAMAGE_ICE] -= penc->addsave[value_idx];
                break;

            case ADDZAPRESIST:
                ptarget->damage_resistance[DAMAGE_ZAP] -= penc->addsave[value_idx];
                break;
        }

        penc->addyesno[value_idx] = false;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void EveStack_init_all()
{
    EVE_REF cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        eve_init( EveStack.get_ptr( cnt ) );
    }
}

//--------------------------------------------------------------------------------------------
void EveStack_release_all()
{
    EVE_REF cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        EveStack_release_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool EveStack_release_one( const EVE_REF ieve )
{
    eve_t * peve;

    if ( !VALID_EVE_RANGE( ieve ) ) return false;
    peve = EveStack.get_ptr( ieve );

    if ( !peve->loaded ) return true;

    eve_init( peve );

    return true;
}

//--------------------------------------------------------------------------------------------
void update_all_enchants()
{
    ENC_REF ienc;

    // update all enchants
    for ( ienc = 0; ienc < MAX_ENC; ienc++ )
    {
        enc_run_config( EncList.get_ptr( ienc ) );
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

    bool enc_used[MAX_ENC];

    ENC_REF first_valid_enchant;

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;

    if ( !VALID_ENC_RANGE( ienc ) ) return MAX_ENC;

    // clear the list
    BLANK_ARY( enc_used )

    // scan the list of enchants
    ienc_nxt            = INVALID_ENC_REF;
    first_valid_enchant = ienc_now = ienc;
    ienc_count = 0;
    while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        // coerce the list of enchants to a valid value
        if ( !VALID_ENC_RANGE( ienc_nxt ) )
        {
            ienc_nxt = EncList.lst[ienc_now].nextenchant_ref = INVALID_ENC_REF;
        }

        // fix any loops in the enchant list
        if ( enc_used[ienc_nxt] )
        {
            EncList.lst[ienc_now].nextenchant_ref = INVALID_ENC_REF;
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

        enc_parent = &( EncList.lst[ienc_now].nextenchant_ref );
        ienc_now    = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

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
        else if ( !valid_owner && !peve->stayifnoowner )
        {
            // the enchant's owner has died
            do_remove = true;
        }
        else if ( !valid_target && !peve->stayiftargetdead )
        {
            // the enchant's target has died
            do_remove = true;
        }
        else if ( valid_owner && peve->endifcantpay )
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
    for ( ENC_REF cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        Ego::Entity *pbase = POBJ_GET_PBASE( EncList.lst + cnt );
        if ( !ACTIVE_PBASE( pbase ) ) continue;

        pbase->update_count++;
    }
}

//--------------------------------------------------------------------------------------------
bool enc_request_terminate( enc_t * penc )
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
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return INVALID_CHR_REF;
    penc = EncList.get_ptr( ienc );

    if ( !_gameObjects.exists( penc->owner_ref ) ) return INVALID_CHR_REF;

    return penc->owner_ref;
}

//--------------------------------------------------------------------------------------------
GameObject * enc_get_powner( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return NULL;
    penc = EncList.get_ptr( ienc );

    if ( !_gameObjects.exists( penc->owner_ref ) ) return NULL;

    return _gameObjects.get( penc->owner_ref );
}

//--------------------------------------------------------------------------------------------
EVE_REF enc_get_ieve( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return INVALID_EVE_REF;
    penc = EncList.get_ptr( ienc );

    if ( !LOADED_EVE( penc->eve_ref ) ) return INVALID_EVE_REF;

    return penc->eve_ref;
}

//--------------------------------------------------------------------------------------------
eve_t * enc_get_peve( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return NULL;
    penc = EncList.get_ptr( ienc );

    if ( !LOADED_EVE( penc->eve_ref ) ) return NULL;

    return EveStack.get_ptr( penc->eve_ref );
}

//--------------------------------------------------------------------------------------------
PRO_REF  enc_get_ipro( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return INVALID_PRO_REF;
    penc = EncList.get_ptr( ienc );

    if ( !_profileSystem.isValidProfileID( penc->profile_ref ) ) return INVALID_PRO_REF;

    return penc->profile_ref;
}

//--------------------------------------------------------------------------------------------
ObjectProfile * enc_get_ppro( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return NULL;
    penc = EncList.get_ptr( ienc );

    if ( !_profileSystem.isValidProfileID( penc->profile_ref ) ) return NULL;

    return _profileSystem.getProfile( penc->profile_ref ).get();
}

//--------------------------------------------------------------------------------------------
IDSZ enc_get_idszremove( const ENC_REF ienc )
{
    eve_t * peve = enc_get_peve( ienc );
    if ( NULL == peve ) return IDSZ_NONE;

    return peve->removedbyidsz;
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

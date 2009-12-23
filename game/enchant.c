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

/// @file enchant.c
/// @brief handles enchantments attached to objects
/// @details

#include "enchant.h"

#include "char.h"
#include "mad.h"
#include "particle.h"
#include "profile.h"

#include "sound.h"
#include "camera.h"
#include "game.h"
#include "script_functions.h"
#include "log.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
DECLARE_STACK( ACCESS_TYPE_NONE, eve_t, EveStack );
DECLARE_LIST( ACCESS_TYPE_NONE, enc_t, EncList );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static enc_t * enc_init( enc_t * penc );

static void   EncList_init();
static Uint16 EncList_get_free();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void enchant_system_init()
{
    EncList_init();
    init_all_eve();
}

//--------------------------------------------------------------------------------------------
void EncList_init()
{
    int cnt;

    EncList.free_count = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        enc_t * penc = EncList.lst + cnt;

        // blank out all the data, including the obj_base data
        memset( penc, 0, sizeof( *penc ) );

        // enchant "destructor"
        enc_init( penc );

        EncList.free_ref[EncList.free_count] = EncList.free_count;
        EncList.free_count++;
    }
}

//--------------------------------------------------------------------------------------------
void EncList_update_used()
{
    int cnt;

    EncList.used_count = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        if ( !ACTIVE_ENC( cnt ) ) continue;

        EncList.used_ref[EncList.used_count] = cnt;
        EncList.used_count++;
    }

    for ( cnt = EncList.used_count; cnt < MAX_ENC; cnt++ )
    {
        EncList.used_ref[EncList.used_count] = MAX_ENC;
    }
}

//--------------------------------------------------------------------------------------------
bool_t EncList_free_one( Uint16 ienc )
{
    /// @details ZZ@> This function sticks an enchant back on the free enchant stack
    ///
    /// @note Tying ALLOCATED_ENC() and EGO_OBJECT_TERMINATE() to EncList_free_one()
    /// should be enough to ensure that no enchant is freed more than once

    bool_t retval;
    enc_t * penc;

    if ( !ALLOCATED_ENC( ienc ) ) return bfalse;
    penc = EncList.lst + ienc;

    // enchant "destructor"
    enc_init( penc );

#if defined(USE_DEBUG) && defined(DEBUG_ENC_LIST)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < EncList.free_count; cnt++ )
        {
            if ( ienc == EncList.free_ref[cnt] ) return bfalse;
        }
    }
#endif

    // push it on the free stack
    retval = bfalse;
    if ( EncList.free_count < MAX_ENC )
    {
        EncList.free_ref[EncList.free_count] = ienc;
        EncList.free_count++;

        retval = btrue;
    }

    EGO_OBJECT_TERMINATE( penc );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t remove_enchant( Uint16 ienc )
{
    /// @details ZZ@> This function removes a specific enchantment and adds it to the unused list

    Sint16 iwave;
    Uint16 overlay_ref;
    Uint16 lastenchant, currentenchant;
    int add_type, set_type;


    enc_t * penc;
    eve_t * peve;
    Uint16 itarget, ispawner;

    if ( !ALLOCATED_ENC( ienc ) ) return bfalse;
    penc = EncList.lst + ienc;

    itarget  = penc->target_ref;
    ispawner = penc->spawner_ref;
    peve     = enc_get_peve( ienc );

    // Unsparkle the spellbook
    if ( ACTIVE_CHR( ispawner ) )
    {
        chr_t * pspawner = ChrList.lst + ispawner;

        pspawner->sparkle = NOSPARKLE;

        // Make the spawner unable to undo the enchantment
        if ( pspawner->undoenchant == ienc )
        {
            pspawner->undoenchant = MAX_ENC;
        }
    }

    // Remove all the enchant stuff in exactly the opposite order to how it was applied

    // Remove all of the cumulative values first, since we did it
    for ( add_type = ENC_ADD_LAST; add_type >= ENC_ADD_FIRST; add_type-- )
    {
        enchant_remove_add( ienc, add_type );
    }

    // unset them in the reverse order of setting them, doing morph last
    for ( set_type = ENC_SET_LAST; set_type >= ENC_SET_FIRST; set_type-- )
    {
        enchant_remove_set( ienc, set_type );
    }

    // Now fix dem weapons
    if ( ACTIVE_CHR( penc->target_ref ) )
    {
        chr_t * ptarget = ChrList.lst + penc->target_ref;
        reset_character_alpha( ptarget->holdingwhich[SLOT_LEFT] );
        reset_character_alpha( ptarget->holdingwhich[SLOT_RIGHT] );
    }

    // Unlink it
    if ( ACTIVE_CHR( penc->target_ref ) )
    {
        chr_t * ptarget = ChrList.lst + penc->target_ref;

        if ( ptarget->firstenchant == ienc )
        {
            // It was the first in the list
            ptarget->firstenchant = EncList.lst[ienc].nextenchant_ref;
        }
        else
        {
            // Search until we find it
            lastenchant = currentenchant = ptarget->firstenchant;
            while ( MAX_ENC != currentenchant && currentenchant != ienc )
            {
                lastenchant    = currentenchant;
                currentenchant = EncList.lst[currentenchant].nextenchant_ref;
            }

            // Relink the last enchantment
            if ( currentenchant == ienc )
            {
                EncList.lst[lastenchant].nextenchant_ref = EncList.lst[ienc].nextenchant_ref;
            }
        }
    }

    // Kill overlay too...
    overlay_ref = penc->overlay_ref;
    if ( ACTIVE_CHR( overlay_ref ) )
    {
        chr_t * povl = ChrList.lst + overlay_ref;

        if ( povl->invictus )
        {
            chr_get_pteam_base( overlay_ref )->morale++;
        }

        kill_character( overlay_ref, MAX_CHR, btrue );
    }

    // nothing above this demends on having a valid enchant profile
    if ( NULL != peve )
    {
        // Play the end sound
        iwave = peve->endsound_index;
        if ( VALID_SND( iwave ) )
        {
            Uint16 imodel = penc->spawnermodel_ref;
            if ( LOADED_PRO( imodel ) )
            {
                if ( ACTIVE_CHR( itarget ) )
                {
                    sound_play_chunk( ChrList.lst[itarget].pos_old, pro_get_chunk( imodel, iwave ) );
                }
                else
                {
                    sound_play_chunk( PCamera->track_pos, pro_get_chunk( imodel, iwave ) );
                }
            }
        }

        // See if we spit out an end message
        if ( peve->endmessage >= 0 )
        {
            _display_message( penc->target_ref, penc->profile_ref, peve->endmessage, NULL );
        }

        // Check to see if we spawn a poof
        if ( peve->poofonend )
        {
            spawn_poof( penc->target_ref, penc->profile_ref );
        }

        if ( ACTIVE_CHR( itarget ) )
        {
            chr_t * ptarget = ChrList.lst + penc->target_ref;

            // Remove see kurse enchant
            if ( peve->seekurse && !chr_get_pcap( itarget )->canseekurse )
            {
                ptarget->canseekurse = bfalse;
            }
        }
    }

    EncList_free_one( ienc );

    // save this until the enchant is completely dead, since kill character can generate a
    // recursive call to this function through cleanup_one_character()
    // @note all of the values in the penc are now invalid. we have to use previously evaluated
    // values of itarget and penc to kill the target (if necessary)
    if ( ACTIVE_CHR( itarget ) && NULL != peve && peve->killtargetonend )
    {
        chr_t * ptarget = ChrList.lst + itarget;

        if ( ptarget->invictus )  chr_get_pteam_base( itarget )->morale++;

        //ptarget->invictus = bfalse;   ZF> no longer needed because ignoreinvictus is added in kill_character()?
        kill_character( itarget, MAX_CHR, btrue );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 enchant_value_filled( Uint16 ienc, Uint8 valueindex )
{
    /// @details ZZ@> This function returns MAX_ENC if the enchantment's target has no conflicting
    ///    set values in its other enchantments.  Otherwise it returns the ienc
    ///    of the conflicting enchantment

    Uint16 character, currenchant;

    if ( !ACTIVE_ENC( ienc ) ) return MAX_ENC;

    character = EncList.lst[ienc].target_ref;

    currenchant = ChrList.lst[character].firstenchant;
    while ( currenchant != MAX_ENC )
    {
        if ( ACTIVE_ENC( currenchant ) && EncList.lst[currenchant].setyesno[valueindex] )
        {
            break;
        }
        currenchant = EncList.lst[currenchant].nextenchant_ref;
    }

    return currenchant;
}

//--------------------------------------------------------------------------------------------
void enchant_apply_set( Uint16 ienc, Uint8 valueindex, Uint16 profile )
{
    /// @details ZZ@> This function sets and saves one of the character's stats

    Uint16 conflict, character;
    enc_t * penc;
    eve_t * peve;
    chr_t * ptarget;

    if ( !ACTIVE_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    peve = pro_get_peve( profile );
    if ( NULL == peve ) return;

    penc->setyesno[valueindex] = bfalse;
    if ( peve->setyesno[valueindex] )
    {
        conflict = enchant_value_filled( ienc, valueindex );
        if ( conflict == MAX_ENC || peve->override )
        {
            // Check for multiple enchantments
            if ( conflict < MAX_ENC )
            {
                // Multiple enchantments aren't allowed for sets
                if ( peve->removeoverridden )
                {
                    // Kill the old enchantment
                    remove_enchant( conflict );
                }
                else
                {
                    // Just unset the old enchantment's value
                    enchant_remove_set( conflict, valueindex );
                }
            }

            // Set the value, and save the character's real stat
            if ( ACTIVE_CHR( penc->target_ref ) )
            {
                character = penc->target_ref;
                ptarget = ChrList.lst + character;

                penc->setyesno[valueindex] = btrue;

                switch ( valueindex )
                {
                    case SETDAMAGETYPE:
                        penc->setsave[valueindex] = ptarget->damagetargettype;
                        ptarget->damagetargettype = peve->setvalue[valueindex];
                        break;

                    case SETNUMBEROFJUMPS:
                        penc->setsave[valueindex] = ptarget->jumpnumberreset;
                        ptarget->jumpnumberreset = peve->setvalue[valueindex];
                        break;

                    case SETLIFEBARCOLOR:
                        penc->setsave[valueindex] = ptarget->lifecolor;
                        ptarget->lifecolor = peve->setvalue[valueindex];
                        break;

                    case SETMANABARCOLOR:
                        penc->setsave[valueindex] = ptarget->manacolor;
                        ptarget->manacolor = peve->setvalue[valueindex];
                        break;

                    case SETSLASHMODIFIER:
                        penc->setsave[valueindex] = ptarget->damagemodifier[DAMAGE_SLASH];
                        ptarget->damagemodifier[DAMAGE_SLASH] = peve->setvalue[valueindex];
                        break;

                    case SETCRUSHMODIFIER:
                        penc->setsave[valueindex] = ptarget->damagemodifier[DAMAGE_CRUSH];
                        ptarget->damagemodifier[DAMAGE_CRUSH] = peve->setvalue[valueindex];
                        break;

                    case SETPOKEMODIFIER:
                        penc->setsave[valueindex] = ptarget->damagemodifier[DAMAGE_POKE];
                        ptarget->damagemodifier[DAMAGE_POKE] = peve->setvalue[valueindex];
                        break;

                    case SETHOLYMODIFIER:
                        penc->setsave[valueindex] = ptarget->damagemodifier[DAMAGE_HOLY];
                        ptarget->damagemodifier[DAMAGE_HOLY] = peve->setvalue[valueindex];
                        break;

                    case SETEVILMODIFIER:
                        penc->setsave[valueindex] = ptarget->damagemodifier[DAMAGE_EVIL];
                        ptarget->damagemodifier[DAMAGE_EVIL] = peve->setvalue[valueindex];
                        break;

                    case SETFIREMODIFIER:
                        penc->setsave[valueindex] = ptarget->damagemodifier[DAMAGE_FIRE];
                        ptarget->damagemodifier[DAMAGE_FIRE] = peve->setvalue[valueindex];
                        break;

                    case SETICEMODIFIER:
                        penc->setsave[valueindex] = ptarget->damagemodifier[DAMAGE_ICE];
                        ptarget->damagemodifier[DAMAGE_ICE] = peve->setvalue[valueindex];
                        break;

                    case SETZAPMODIFIER:
                        penc->setsave[valueindex] = ptarget->damagemodifier[DAMAGE_ZAP];
                        ptarget->damagemodifier[DAMAGE_ZAP] = peve->setvalue[valueindex];
                        break;

                    case SETFLASHINGAND:
                        penc->setsave[valueindex] = ptarget->flashand;
                        ptarget->flashand = peve->setvalue[valueindex];
                        break;

                    case SETLIGHTBLEND:
                        penc->setsave[valueindex] = ptarget->inst.light;
                        chr_set_light( ptarget, peve->setvalue[valueindex] );
                        break;

                    case SETALPHABLEND:
                        penc->setsave[valueindex] = ptarget->inst.alpha;
                        chr_set_alpha( ptarget, peve->setvalue[valueindex] );
                        break;

                    case SETSHEEN:
                        penc->setsave[valueindex] = ptarget->inst.sheen;
                        chr_set_sheen( ptarget, peve->setvalue[valueindex] );
                        break;

                    case SETFLYTOHEIGHT:
                        penc->setsave[valueindex] = ptarget->flyheight;
                        if ( ptarget->flyheight == 0 && ptarget->pos.z > -2 )
                        {
                            ptarget->flyheight = peve->setvalue[valueindex];
                        }
                        break;

                    case SETWALKONWATER:
                        penc->setsave[valueindex] = ptarget->waterwalk;
                        if ( !ptarget->waterwalk )
                        {
                            ptarget->waterwalk = peve->setvalue[valueindex];
                        }
                        break;

                    case SETCANSEEINVISIBLE:
                        penc->setsave[valueindex] = ptarget->see_invisible_level;
                        ptarget->see_invisible_level = peve->setvalue[valueindex];
                        break;

                    case SETMISSILETREATMENT:
                        penc->setsave[valueindex] = ptarget->missiletreatment;
                        ptarget->missiletreatment = peve->setvalue[valueindex];
                        break;

                    case SETCOSTFOREACHMISSILE:
                        penc->setsave[valueindex] = ptarget->missilecost;
                        ptarget->missilecost = peve->setvalue[valueindex];
                        ptarget->missilehandler = penc->owner_ref;
                        break;

                    case SETMORPH:
                        // Special handler for morph
                        penc->setsave[valueindex] = ptarget->skin;
                        change_character( character, profile, 0, LEAVEALL ); // LEAVEFIRST);
                        break;

                    case SETCHANNEL:
                        penc->setsave[valueindex] = ptarget->canchannel;
                        ptarget->canchannel = peve->setvalue[valueindex];
                        break;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void enchant_apply_add( Uint16 ienc, Uint8 valueindex, Uint16 ieve )
{
    /// @details ZZ@> This function does cumulative modification to character stats

    int valuetoadd, newvalue;
    float fvaluetoadd, fnewvalue;
    Uint16 character;
    enc_t * penc;
    eve_t * peve;
    chr_t * ptarget;

    if ( !ACTIVE_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    if ( ieve >= MAX_EVE || !EveStack.lst[ieve].loaded ) return;
    peve = EveStack.lst + ieve;

    if( !peve->addyesno[valueindex] ) 
    {
        penc->addyesno[valueindex] = bfalse;
        penc->addsave[valueindex]  = 0.0f;
        return;
    }

    if ( !ACTIVE_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = ChrList.lst + character;

    valuetoadd  = 0;
    fvaluetoadd = 0.0f;
    switch ( valueindex )
    {
        case ADDJUMPPOWER:
            fnewvalue = ptarget->jump_power;
            fvaluetoadd = peve->addvalue[valueindex] / 16.0f;
            fgetadd( 0, fnewvalue, 30.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 16.0f; // Get save value
            fvaluetoadd = valuetoadd / 16.0f;
            ptarget->jump_power += fvaluetoadd;
            break;

        case ADDBUMPDAMPEN:
            fnewvalue = ptarget->phys.bumpdampen;
            fvaluetoadd = peve->addvalue[valueindex] / 128.0f;
            fgetadd( 0, fnewvalue, 1.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            ptarget->phys.bumpdampen += fvaluetoadd;
            break;

        case ADDBOUNCINESS:
            fnewvalue = ptarget->phys.dampen;
            fvaluetoadd = peve->addvalue[valueindex] / 128.0f;
            fgetadd( 0, fnewvalue, 0.95f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            ptarget->phys.dampen += fvaluetoadd;
            break;

        case ADDDAMAGE:
            newvalue = ptarget->damageboost;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, 4096, &valuetoadd );
            ptarget->damageboost += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDSIZE:
            fnewvalue = ptarget->fat_goto;
            fvaluetoadd = peve->addvalue[valueindex] / 128.0f;
            fgetadd( 0.5f, fnewvalue, 2.0f, &fvaluetoadd );
            //valuetoadd = fvaluetoadd * 128.0f; // Get save value
            //fvaluetoadd = valuetoadd / 128.0f;
            ptarget->fat_goto += fvaluetoadd;
            ptarget->fat_goto_time = SIZETIME;
            break;

        case ADDACCEL:
            fnewvalue = ptarget->maxaccel;
            fvaluetoadd = peve->addvalue[valueindex] / 80.0f;
            fgetadd( 0, fnewvalue, 1.5f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 1000.0f; // Get save value
            fvaluetoadd = valuetoadd / 1000.0f;
            ptarget->maxaccel += fvaluetoadd;
            break;

        case ADDRED:
            newvalue = ptarget->inst.redshift;
            valuetoadd = peve->addvalue[valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr_set_redshift( ptarget, ptarget->inst.redshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDGRN:
            newvalue = ptarget->inst.grnshift;
            valuetoadd = peve->addvalue[valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr_set_grnshift( ptarget, ptarget->inst.grnshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDBLU:
            newvalue = ptarget->inst.blushift;
            valuetoadd = peve->addvalue[valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr_set_blushift( ptarget, ptarget->inst.blushift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDDEFENSE:
            newvalue = ptarget->defense;
            valuetoadd = peve->addvalue[valueindex];
            getadd( 55, newvalue, 255, &valuetoadd );  // Don't fix again!
            ptarget->defense += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDMANA:
            newvalue = ptarget->manamax;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->manamax += valuetoadd;
            ptarget->mana    += valuetoadd;
            if ( ptarget->mana < 0 )  ptarget->mana = 0;
            fvaluetoadd = valuetoadd;
            break;

        case ADDLIFE:
            newvalue = ptarget->lifemax;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( LOWSTAT, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->lifemax += valuetoadd;
            ptarget->life += valuetoadd;
            if ( ptarget->life < 1 )  ptarget->life = 1;
            fvaluetoadd = valuetoadd;
            break;

        case ADDSTRENGTH:
            newvalue = ptarget->strength;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->strength += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDWISDOM:
            newvalue = ptarget->wisdom;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->wisdom += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDINTELLIGENCE:
            newvalue = ptarget->intelligence;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->intelligence += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDDEXTERITY:
            newvalue = ptarget->dexterity;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->dexterity += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;
    }

    // save whether there was any change in the value
    penc->addyesno[valueindex] = (0.0f != fvaluetoadd); 

    // Save the value for undo
    penc->addsave[valueindex]  = valuetoadd;

}

//--------------------------------------------------------------------------------------------
enc_t * enc_init( enc_t * penc )
{
    ego_object_base_t save_base;

    if ( NULL == penc ) return penc;

    // save the base object data
    memcpy( &save_base, POBJ_GET_PBASE( penc ), sizeof( ego_object_base_t ) );

    memset( penc, 0, sizeof( *penc ) );

    // restore the base object data
    memcpy( POBJ_GET_PBASE( penc ), &save_base, sizeof( ego_object_base_t ) );

    penc->profile_ref      = MAX_PROFILE;
    penc->eve_ref          = MAX_EVE;

    penc->target_ref       = MAX_CHR;
    penc->owner_ref        = MAX_CHR;
    penc->spawner_ref      = MAX_CHR;
    penc->spawnermodel_ref = MAX_PROFILE;
    penc->overlay_ref      = MAX_CHR;

    penc->nextenchant_ref  = MAX_ENC;

    return penc;
}

//--------------------------------------------------------------------------------------------
Uint16 enc_get_free( Uint16 override )
{
    int    tnc;
    Uint16 ienc = MAX_ENC;

    ienc = MAX_ENC;
    if ( VALID_ENC_RANGE( override ) )
    {
        ienc = EncList_get_free();
        if ( ienc != override )
        {
            // Picked the wrong one, so put this one back and find the right one

            for ( tnc = 0; tnc < MAX_ENC; tnc++ )
            {
                if ( EncList.free_ref[tnc] == override )
                {
                    EncList.free_ref[tnc] = ienc;
                    break;
                }
            }

            ienc = override;
        }

        if ( MAX_ENC == ienc )
        {
            log_warning( "enc_get_free() - failed to override an enchant? enchant %d already spawned? \n", override );
        }
    }
    else
    {
        ienc = EncList_get_free();
        if ( MAX_ENC == ienc )
        {
            log_warning( "enc_get_free() - failed to allocate a new character\n" );
        }
    }

    if ( MAX_ENC != ienc )
    {
        EGO_OBJECT_ALLOCATE( EncList.lst + ienc, ienc );
    }

    return ienc;
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_enchant( Uint16 owner, Uint16 target, Uint16 spawner, Uint16 enc_override, Uint16 modeloptional )
{
    /// @details ZZ@> This function enchants a target, returning the enchantment index or MAX_ENC
    ///    if failed

    Uint16 ienc, ieve, iprofile, overlay;
    int add_type, set_type;
    eve_t * peve;
    enc_t * penc;
    chr_t * ptarget;

    // Target must both be alive and on and valid
    if ( !ACTIVE_CHR( target ) )
    {
        log_warning( "spawn_one_enchant() - failed because target does not exist.\n" );
        return MAX_ENC;
    }
    ptarget = ChrList.lst + target;

    // you should be able to enchant dead stuff to raise the dead...
    // if( !ptarget->alive ) return MAX_ENC;

    if ( LOADED_PRO( modeloptional ) )
    {
        // The enchantment type is given explicitly
        iprofile = modeloptional;

    }
    else
    {
        // The enchantment type is given by the spawner
        iprofile = chr_get_ipro( spawner );

        if ( !LOADED_PRO( iprofile ) )
        {
            log_warning( "spawn_one_enchant() - no valid profile for the spawning character \"%s\"(%d).\n", ChrList.lst[spawner].obj_base._name, spawner );
            return MAX_ENC;
        }
    }

    ieve = pro_get_ieve( iprofile );
    if ( !LOADED_EVE( ieve ) )
    {
        log_warning( "spawn_one_enchant() - the object \"%s\"(%d) does not have an enchant profile.\n", ProList.lst[iprofile].name, iprofile );

        return MAX_ENC;
    }
    peve = EveStack.lst + ieve;

    // Owner must both be alive and on and valid if it isn't a stayifnoowner enchant
    if ( !peve->stayifnoowner && ( !ACTIVE_CHR( owner ) || !ChrList.lst[owner].alive ) )
    {
        log_warning( "spawn_one_enchant() - failed because the required enchant owner cannot be found.\n" );
        return MAX_ENC;
    }

    // do retargeting, if necessary
    // Should it choose an inhand item?
    if ( peve->retarget )
    {
        // Left, right, or both are valid
        if ( ACTIVE_CHR( ptarget->holdingwhich[SLOT_LEFT] ) )
        {
            // Only right hand is valid
            target = ptarget->holdingwhich[SLOT_RIGHT];
        }
        else if ( ACTIVE_CHR( ptarget->holdingwhich[SLOT_LEFT] ) )
        {
            // Pick left hand
            target = ptarget->holdingwhich[SLOT_LEFT];
        }
        else
        {
            // No weapons to pick
            target = MAX_CHR;
        }
    }

    // make sure the target is valid
    if ( !ACTIVE_CHR( target ) || !ptarget->alive )
    {
        log_warning( "spawn_one_enchant() - failed because the target is not alive.\n" );
        return MAX_ENC;
    }
    ptarget = ChrList.lst + target;

    // Check peve->dontdamagetype
    if ( peve->dontdamagetype != DAMAGE_NONE )
    {
        if (( ptarget->damagemodifier[peve->dontdamagetype]&DAMAGESHIFT ) >= 3 ||
            ptarget->damagemodifier[peve->dontdamagetype]&DAMAGECHARGE )
        {
            log_warning( "spawn_one_enchant() - failed because the target is immune to the enchant.\n" );
            return MAX_ENC;
        }
    }

    // Check peve->onlydamagetype
    if ( peve->onlydamagetype != DAMAGE_NONE )
    {
        if ( ptarget->damagetargettype != peve->onlydamagetype )
        {
            log_warning( "spawn_one_enchant() - failed because the target not have the right damagetargettype.\n" );
            return MAX_ENC;
        }
    }

    // Find an enchant index to use
    ienc = enc_get_free( enc_override );

    if ( !ALLOCATED_ENC( ienc ) )
    {
        log_warning( "spawn_one_enchant() - could not allocate an enchant.\n" );
        return MAX_ENC;
    }
    penc = EncList.lst + ienc;

    // initialize the enchant
    enc_init( penc );

    // turn the enchant on here. you can't fail to spawn after this point.
    EGO_OBJECT_ACTIVATE( penc, peve->name );

    penc->target_ref       = ACTIVE_CHR( target )  ? target  : MAX_CHR;
    penc->owner_ref        = ACTIVE_CHR( owner )   ? owner   : MAX_CHR;
    penc->spawner_ref      = ACTIVE_CHR( spawner ) ? spawner : MAX_CHR;
    penc->spawnermodel_ref = chr_get_ipro( spawner );

    if ( ACTIVE_CHR( spawner ) )
    {
        ChrList.lst[spawner].undoenchant = ienc;
    }

    penc->eve_ref      = ieve;
    penc->profile_ref  = iprofile;
    penc->time         = peve->time;
    penc->spawntime    = 1;
    penc->ownermana    = peve->ownermana;
    penc->ownerlife    = peve->ownerlife;
    penc->targetmana   = peve->targetmana;
    penc->targetlife   = peve->targetlife;

    // Add it as first in the list
    penc->nextenchant_ref = ptarget->firstenchant;
    ptarget->firstenchant = ienc;

    // Now set all of the specific values, morph first
    for ( set_type = ENC_SET_FIRST; set_type <= ENC_SET_LAST; set_type++ )
    {
        enchant_apply_set( ienc, set_type, iprofile );
    }

    // Now do all of the stat adds
    for ( add_type = ENC_ADD_FIRST; add_type <= ENC_ADD_LAST; add_type++ )
    {
        enchant_apply_add( ienc, add_type, ieve );
    }

    // Create an overlay character?
    if ( peve->spawn_overlay )
    {
        overlay = spawn_one_character( ptarget->pos, iprofile, ptarget->team, 0, ptarget->turn_z, NULL, MAX_CHR );
        if ( ACTIVE_CHR( overlay ) )
        {
            chr_t * povl;
            mad_t * povl_mad;
            int action;

            povl     = ChrList.lst + overlay;
            povl_mad = chr_get_pmad( overlay );

            penc->overlay_ref = overlay;  // Kill this character on end...
            povl->ai.target   = target;
            povl->ai.state    = peve->spawn_overlay;    // ??? WHY DO THIS ???
            povl->is_overlay  = btrue;

            // Start out with ActionMJ...  Object activated
            action = mad_get_action( chr_get_imad( overlay ), ACTION_MJ );
            if ( !ACTION_IS_TYPE( action, D ) )
            {
                chr_start_anim( povl, action, bfalse, btrue );
            }

            // Assume it's transparent...
            chr_set_light( povl, 254 );
            chr_set_alpha( povl,   0 );
        }
    }

    // Allow them to see kurses?
    if ( peve->seekurse )
    {
        ptarget->canseekurse = btrue;
    }

    return ienc;
}

//--------------------------------------------------------------------------------------------
void disenchant_character( Uint16 cnt )
{
    /// @details ZZ@> This function removes all enchantments from a character
    while ( ChrList.lst[cnt].firstenchant != MAX_ENC )
    {
        remove_enchant( ChrList.lst[cnt].firstenchant );
    }
}

//--------------------------------------------------------------------------------------------
void EncList_free_all()
{
    /// @details ZZ@> This functions frees all of the enchantments

    int cnt;

    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        EncList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
Uint16 load_one_enchant_profile( const char* szLoadName, Uint16 ieve )
{
    /// @details ZZ@> This function loads an enchantment profile into the EveStack

    eve_t * peve;

    if ( ieve > MAX_EVE ) return MAX_EVE;
    peve = EveStack.lst + ieve;

    if ( NULL == load_one_enchant_file( szLoadName, peve ) )
    {
        ieve = MAX_EVE;
    }

    // limit the endsound_index
    peve->endsound_index = CLIP( peve->endsound_index, INVALID_SOUND, MAX_WAVE );

    return ieve;
}

//--------------------------------------------------------------------------------------------
Uint16 EncList_get_free()
{
    /// @details ZZ@> This function returns the next free enchantment or MAX_ENC if there are none

    Uint16 retval = MAX_ENC;

    if ( EncList.free_count > 0 )
    {
        EncList.free_count--;
        retval = EncList.free_ref[EncList.free_count];
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void enchant_remove_set( Uint16 ienc, Uint8 valueindex )
{
    /// @details ZZ@> This function unsets a set value
    Uint16 character;
    enc_t * penc;
    chr_t * ptarget;

    if ( !ALLOCATED_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    if( valueindex >= MAX_ENCHANT_SET || !penc->setyesno[valueindex] ) return;

    if ( !ACTIVE_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget   = ChrList.lst + penc->target_ref;

    switch ( valueindex )
    {
        case SETDAMAGETYPE:
            ptarget->damagetargettype = penc->setsave[valueindex];
            break;

        case SETNUMBEROFJUMPS:
            ptarget->jumpnumberreset = penc->setsave[valueindex];
            break;

        case SETLIFEBARCOLOR:
            ptarget->lifecolor = penc->setsave[valueindex];
            break;

        case SETMANABARCOLOR:
            ptarget->manacolor = penc->setsave[valueindex];
            break;

        case SETSLASHMODIFIER:
            ptarget->damagemodifier[DAMAGE_SLASH] = penc->setsave[valueindex];
            break;

        case SETCRUSHMODIFIER:
            ptarget->damagemodifier[DAMAGE_CRUSH] = penc->setsave[valueindex];
            break;

        case SETPOKEMODIFIER:
            ptarget->damagemodifier[DAMAGE_POKE] = penc->setsave[valueindex];
            break;

        case SETHOLYMODIFIER:
            ptarget->damagemodifier[DAMAGE_HOLY] = penc->setsave[valueindex];
            break;

        case SETEVILMODIFIER:
            ptarget->damagemodifier[DAMAGE_EVIL] = penc->setsave[valueindex];
            break;

        case SETFIREMODIFIER:
            ptarget->damagemodifier[DAMAGE_FIRE] = penc->setsave[valueindex];
            break;

        case SETICEMODIFIER:
            ptarget->damagemodifier[DAMAGE_ICE] = penc->setsave[valueindex];
            break;

        case SETZAPMODIFIER:
            ptarget->damagemodifier[DAMAGE_ZAP] = penc->setsave[valueindex];
            break;

        case SETFLASHINGAND:
            ptarget->flashand = penc->setsave[valueindex];
            break;

        case SETLIGHTBLEND:
            chr_set_light( ptarget, penc->setsave[valueindex] );
            break;

        case SETALPHABLEND:
            chr_set_alpha( ptarget, penc->setsave[valueindex] );
            break;

        case SETSHEEN:
            chr_set_sheen( ptarget, penc->setsave[valueindex] );
            break;

        case SETFLYTOHEIGHT:
            ptarget->flyheight = penc->setsave[valueindex];
            break;

        case SETWALKONWATER:
            ptarget->waterwalk = penc->setsave[valueindex];
            break;

        case SETCANSEEINVISIBLE:
            ptarget->see_invisible_level = penc->setsave[valueindex];
            break;

        case SETMISSILETREATMENT:
            ptarget->missiletreatment = penc->setsave[valueindex];
            break;

        case SETCOSTFOREACHMISSILE:
            ptarget->missilecost = penc->setsave[valueindex];
            ptarget->missilehandler = character;
            break;

        case SETMORPH:
            // Need special handler for when this is removed
            change_character( character, ptarget->basemodel, penc->setsave[valueindex], LEAVEALL );
            break;

        case SETCHANNEL:
            ptarget->canchannel = penc->setsave[valueindex];
            break;
    }

    penc->setyesno[valueindex] = bfalse;

}

//--------------------------------------------------------------------------------------------
void enchant_remove_add( Uint16 ienc, Uint8 valueindex )
{
    /// @details ZZ@> This function undoes cumulative modification to character stats

    float fvaluetoadd;
    int valuetoadd;
    Uint16 character;
    enc_t * penc;
    chr_t * ptarget;

    if ( !ALLOCATED_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    if ( !ACTIVE_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = ChrList.lst + penc->target_ref;

    if( penc->addyesno[valueindex] )
    {
        switch ( valueindex )
        {
            case ADDJUMPPOWER:
                fvaluetoadd = penc->addsave[valueindex] / 16.0f;
                ptarget->jump_power -= fvaluetoadd;
                break;

            case ADDBUMPDAMPEN:
                fvaluetoadd = penc->addsave[valueindex] / 128.0f;
                ptarget->phys.bumpdampen -= fvaluetoadd;
                break;

            case ADDBOUNCINESS:
                fvaluetoadd = penc->addsave[valueindex] / 128.0f;
                ptarget->phys.dampen -= fvaluetoadd;
                break;

            case ADDDAMAGE:
                valuetoadd = penc->addsave[valueindex];
                ptarget->damageboost -= valuetoadd;
                break;

            case ADDSIZE:
                fvaluetoadd = penc->addsave[valueindex] / 128.0f;
                ptarget->fat_goto -= fvaluetoadd;
                ptarget->fat_goto_time = SIZETIME;
                break;

            case ADDACCEL:
                fvaluetoadd = penc->addsave[valueindex] / 1000.0f;
                ptarget->maxaccel -= fvaluetoadd;
                break;

            case ADDRED:
                valuetoadd = penc->addsave[valueindex];
                chr_set_redshift( ptarget, ptarget->inst.redshift - valuetoadd );
                break;

            case ADDGRN:
                valuetoadd = penc->addsave[valueindex];
                chr_set_grnshift( ptarget, ptarget->inst.grnshift - valuetoadd );
                break;

            case ADDBLU:
                valuetoadd = penc->addsave[valueindex];
                chr_set_blushift( ptarget, ptarget->inst.blushift - valuetoadd );
                break;

            case ADDDEFENSE:
                valuetoadd = penc->addsave[valueindex];
                ptarget->defense -= valuetoadd;
                break;

            case ADDMANA:
                valuetoadd = penc->addsave[valueindex];
                ptarget->manamax -= valuetoadd;
                ptarget->mana -= valuetoadd;
                if ( ptarget->mana < 0 ) ptarget->mana = 0;
                break;

            case ADDLIFE:
                valuetoadd = penc->addsave[valueindex];
                ptarget->lifemax -= valuetoadd;
                ptarget->life -= valuetoadd;
                if ( ptarget->life < 1 ) ptarget->life = 1;
                break;

            case ADDSTRENGTH:
                valuetoadd = penc->addsave[valueindex];
                ptarget->strength -= valuetoadd;
                break;

            case ADDWISDOM:
                valuetoadd = penc->addsave[valueindex];
                ptarget->wisdom -= valuetoadd;
                break;

            case ADDINTELLIGENCE:
                valuetoadd = penc->addsave[valueindex];
                ptarget->intelligence -= valuetoadd;
                break;

            case ADDDEXTERITY:
                valuetoadd = penc->addsave[valueindex];
                ptarget->dexterity -= valuetoadd;
                break;
        }

        penc->addyesno[valueindex] = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
Uint16  enc_get_iowner( Uint16 ienc )
{
    enc_t * penc;

    if ( !ACTIVE_ENC( ienc ) && !WAITING_ENC( ienc ) ) return MAX_CHR;
    penc = EncList.lst + ienc;

    if ( !ACTIVE_CHR( penc->owner_ref ) ) return MAX_CHR;

    return penc->owner_ref;
}

//--------------------------------------------------------------------------------------------
chr_t * enc_get_powner( Uint16 ienc )
{
    enc_t * penc;

    if ( !ACTIVE_ENC( ienc ) && !WAITING_ENC( ienc ) ) return NULL;
    penc = EncList.lst + ienc;

    if ( !ACTIVE_CHR( penc->owner_ref ) ) return NULL;

    return ChrList.lst + penc->owner_ref;
}

//--------------------------------------------------------------------------------------------
Uint16  enc_get_ieve( Uint16 ienc )
{
    enc_t * penc;

    if ( !ACTIVE_ENC( ienc ) && !WAITING_ENC( ienc ) ) return MAX_EVE;
    penc = EncList.lst + ienc;

    if ( !LOADED_EVE( penc->eve_ref ) ) return MAX_EVE;

    return penc->eve_ref;
}

//--------------------------------------------------------------------------------------------
eve_t * enc_get_peve( Uint16 ienc )
{
    enc_t * penc;

    if ( !ACTIVE_ENC( ienc ) && !WAITING_ENC( ienc ) ) return NULL;
    penc = EncList.lst + ienc;

    if ( !LOADED_EVE( penc->eve_ref ) ) return NULL;

    return EveStack.lst + penc->eve_ref;
}

//--------------------------------------------------------------------------------------------
Uint16  enc_get_ipro( Uint16 ienc )
{
    enc_t * penc;

    if ( !ACTIVE_ENC( ienc ) && !WAITING_ENC( ienc ) ) return MAX_PROFILE;
    penc = EncList.lst + ienc;

    if ( !LOADED_PRO( penc->profile_ref ) ) return MAX_PROFILE;

    return penc->profile_ref;
}

//--------------------------------------------------------------------------------------------
pro_t * enc_get_ppro( Uint16 ienc )
{
    enc_t * penc;

    if ( !ACTIVE_ENC( ienc ) && !WAITING_ENC( ienc ) ) return NULL;
    penc = EncList.lst + ienc;

    if ( !LOADED_PRO( penc->profile_ref ) ) return NULL;

    return ProList.lst + penc->profile_ref;
}

//--------------------------------------------------------------------------------------------
IDSZ enc_get_idszremove( Uint16 ienc )
{
    eve_t * peve = enc_get_peve( ienc );
    if ( NULL == peve ) return IDSZ_NONE;

    return peve->removedbyidsz;
}

//--------------------------------------------------------------------------------------------
bool_t enc_is_removed( Uint16 ienc, Uint16 test_profile )
{
    IDSZ idsz_remove;

    if ( !ACTIVE_ENC( ienc ) ) return bfalse;
    idsz_remove = enc_get_idszremove( ienc );

    // if nothing can remove it, just go on with your business
    if ( IDSZ_NONE == idsz_remove ) return bfalse;

    // check vs. every IDSZ that could have something to do with cancelling the enchant
    if ( idsz_remove == pro_get_idsz( test_profile, IDSZ_TYPE ) ) return btrue;
    if ( idsz_remove == pro_get_idsz( test_profile, IDSZ_PARENT ) ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_all_eve()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        eve_init( EveStack.lst + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void release_all_eve()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        release_one_eve( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_eve( Uint16 ieve )
{
    eve_t * peve;

    if ( !VALID_EVE_RANGE( ieve ) ) return bfalse;
    peve = EveStack.lst + ieve;

    if ( !peve->loaded ) return btrue;

    eve_init( peve );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void update_all_enchants()
{
    /// @details ZZ@> This function lets enchantments spawn particles

    int cnt, tnc;
    Uint16 facing;
    Uint16 owner, target, eve;

    // the following functions should not be done the first time through the update loop
    if ( clock_wld == 0 ) return;

    // check to see whether the enchant needs to spawn some particles
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        enc_t * penc;
        eve_t * peve;
        chr_t * ptarget;

        if ( !ACTIVE_ENC( cnt ) ) continue;
        penc = EncList.lst + cnt;

        if ( penc->spawntime > 0 ) penc->spawntime--;
        if ( penc->spawntime > 0 ) continue;

        peve = enc_get_peve( cnt );
        if ( NULL == peve ) continue;

        penc->spawntime = peve->contspawn_time;

        if ( peve->contspawn_amount <= 0 ) continue;

        if ( !ACTIVE_CHR( penc->target_ref ) ) continue;
        ptarget = ChrList.lst + penc->target_ref;

        facing = ptarget->turn_z;
        for ( tnc = 0; tnc < peve->contspawn_amount; tnc++ )
        {
            spawn_one_particle( ptarget->pos, facing, penc->profile_ref, peve->contspawn_pip,
                                MAX_CHR, GRIP_LAST, chr_get_iteam( penc->owner_ref ), penc->owner_ref, TOTAL_MAX_PRT, tnc, MAX_CHR );

            facing += peve->contspawn_facingadd;
        }

    }

    // check to see if any enchant
    if ( clock_enc_stat >= ONESECOND )
    {
        // Reset the clock
        clock_enc_stat -= ONESECOND;

        // Run through all the enchants as well
        for ( cnt = 0; cnt < MAX_ENC; cnt++ )
        {
            enc_t * penc;

            if ( !ACTIVE_ENC( cnt ) ) continue;
            penc = EncList.lst + cnt;

            if ( 0 == penc->time )
            {
                enc_request_terminate( cnt );
            }
            else
            {
                // Do enchant timer
                if ( penc->time > 0 ) penc->time--;

                // To make life easier
                owner  = enc_get_iowner( cnt );
                target = penc->target_ref;
                eve    = enc_get_ieve( cnt );

                // Do drains
                if ( ChrList.lst[owner].alive )
                {
                    bool_t mana_paid;

                    // Change life
                    ChrList.lst[owner].life += penc->ownerlife;
                    if ( ChrList.lst[owner].life <= 0 )
                    {
                        kill_character( owner, target, bfalse );
                    }

                    if ( ChrList.lst[owner].life > ChrList.lst[owner].lifemax )
                    {
                        ChrList.lst[owner].life = ChrList.lst[owner].lifemax;
                    }

                    // Change mana
                    mana_paid = cost_mana( owner, -penc->ownermana, target );
                    if ( EveStack.lst[eve].endifcantpay && !mana_paid )
                    {
                        enc_request_terminate( cnt );
                    }
                }
                else if ( !EveStack.lst[eve].stayifnoowner )
                {
                    enc_request_terminate( cnt );
                }

                // the enchant could have been inactivated by the stuff above
                // check it again
                if ( ACTIVE_ENC( cnt ) )
                {
                    if ( ChrList.lst[target].alive )
                    {
                        bool_t mana_paid;

                        // Change life
                        ChrList.lst[target].life += penc->targetlife;
                        if ( ChrList.lst[target].life <= 0 )
                        {
                            kill_character( target, owner, bfalse );
                        }
                        if ( ChrList.lst[target].life > ChrList.lst[target].lifemax )
                        {
                            ChrList.lst[target].life = ChrList.lst[target].lifemax;
                        }

                        // Change mana
                        mana_paid = cost_mana( target, -penc->targetmana, owner );
                        if ( EveStack.lst[eve].endifcantpay && !mana_paid )
                        {
                            enc_request_terminate( cnt );
                        }
                    }
                    else if ( !EveStack.lst[eve].stayiftargetdead )
                    {
                        enc_request_terminate( cnt );
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
Uint16 cleanup_enchant_list( Uint16 ienc )
{
    /// @details BB@> remove all the dead enchants from the enchant list
    ///     and report back the first non-dead enchant in the list.

    Uint16 first_valid_enchant;
    Uint16 enc_now, enc_next;

    first_valid_enchant = MAX_ENC;
    enc_now = ienc;
    while ( MAX_ENC != enc_now )
    {
        enc_next = EncList.lst[enc_now].nextenchant_ref;

        if ( !ACTIVE_ENC( enc_now ) )
        {
            remove_enchant( enc_now );
        }
        else if ( MAX_ENC == first_valid_enchant )
        {
            first_valid_enchant = enc_now;
        }

        enc_now = enc_next;
    }

    return first_valid_enchant;
}

//--------------------------------------------------------------------------------------------
void cleanup_all_enchants()
{
    /// @details ZZ@> this function scans all the enchants and removes any dead ones.
    ///               this happens only once a loop

    int cnt;

    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        enc_t * penc;
        eve_t * peve;
        bool_t  do_remove;

        // allow inactive (but not terminated) enchants to be cleaned up
        if ( !ALLOCATED_ENC( cnt ) ) continue;
        penc = EncList.lst + cnt;

        if ( !LOADED_EVE( penc->eve_ref ) )
        {
            // this should never happen
            assert( bfalse );
            continue;
        }
        peve = EveStack.lst + penc->eve_ref;

        do_remove = bfalse;
        if ( WAITING_PBASE( POBJ_GET_PBASE( penc ) ) )
        {
            // the enchant has been marked for removal
            do_remove = btrue;
        }
        else if ( !ACTIVE_CHR( penc->owner_ref ) && !peve->stayifnoowner )
        {
            // the enchant's owner has died
            do_remove = btrue;
        }
        else if ( !ACTIVE_CHR( penc->target_ref ) && !peve->stayiftargetdead )
        {
            // the enchant's target has died
            do_remove = btrue;
        }
        else
        {
            // the enchant has timed out
            do_remove = ( 0 == penc->time );
        }

        if ( do_remove )
        {
            remove_enchant( cnt );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t enc_request_terminate( Uint16 ienc )
{
    if ( !ACTIVE_ENC( ienc ) ) return bfalse;

    EGO_OBJECT_REQUST_TERMINATE( EncList.lst + ienc );

    return btrue;
}


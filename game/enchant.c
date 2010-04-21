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

#include "enchant.inl"

#include "char.inl"
#include "mad.h"
#include "particle.inl"
#include "profile.inl"

#include "sound.h"
#include "camera.h"
#include "game.h"
#include "script_functions.h"
#include "log.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INSTANTIATE_STACK( ACCESS_TYPE_NONE, eve_t, EveStack, MAX_EVE );
INSTANTIATE_LIST( ACCESS_TYPE_NONE, enc_t, EncList, MAX_ENC );

int enc_loop_depth = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static enc_t * enc_ctor( enc_t * penc );
static enc_t * enc_dtor( enc_t * penc );
static bool_t  enc_free( enc_t * penc );
static enc_t * enc_config_init( enc_t * penc );
static enc_t * enc_config_deinit( enc_t * penc );
static enc_t * enc_config_active( enc_t * penc );

static void    EncList_init( void );
static void    EncList_dtor( void );
static size_t  EncList_get_free( void );
static ENC_REF EncList_allocate( const ENC_REF override );
static bool_t  EncList_free_one( const ENC_REF by_reference ienc );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void enchant_system_begin()
{
    EncList_init();
    init_all_eve();
}

//--------------------------------------------------------------------------------------------
void enchant_system_end()
{
    release_all_eve();
    EncList_dtor();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void EncList_init()
{
    ENC_REF cnt;

    EncList.free_count = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        enc_t * penc = EncList.lst + cnt;

        // blank out all the data, including the obj_base data
        memset( penc, 0, sizeof( *penc ) );

        // enchant "initializer"
        enc_ctor( penc );

        EncList.free_ref[EncList.free_count] = EncList.free_count;
        EncList.free_count++;
    }
}

//--------------------------------------------------------------------------------------------
void EncList_dtor()
{
    ENC_REF cnt;

    EncList.free_count = 0;
    EncList.used_count = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        enc_dtor( EncList.lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void EncList_free_all()
{
    /// @details ZZ@> This functions frees all of the enchantments

    ENC_REF cnt;

    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        EncList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
size_t EncList_get_free()
{
    /// @details ZZ@> This function returns the next free enchantment or MAX_ENC if there are none

    size_t retval = MAX_ENC;

    if ( EncList.free_count > 0 )
    {
        EncList.free_count--;
        retval = EncList.free_ref[EncList.free_count];
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void EncList_update_used()
{
    ENC_REF cnt;
    size_t  tnc;

    EncList.used_count = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        if ( !INGAME_ENC( cnt ) ) continue;

        EncList.used_ref[EncList.used_count] = REF_TO_INT( cnt );
        EncList.used_count++;
    }

    for ( tnc = EncList.used_count; tnc < MAX_ENC; tnc++ )
    {
        EncList.used_ref[tnc] = MAX_ENC;
    }
}

//--------------------------------------------------------------------------------------------
bool_t EncList_free_one( const ENC_REF by_reference ienc )
{
    /// @details ZZ@> This function sticks an enchant back on the free enchant stack
    ///
    /// @note Tying ALLOCATED_ENC() and EGO_OBJECT_TERMINATE() to EncList_free_one()
    /// should be enough to ensure that no enchant is freed more than once

    bool_t retval;
    enc_t * penc;

    if ( !ALLOCATED_ENC( ienc ) ) return bfalse;
    penc = EncList.lst + ienc;

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
        EncList.free_ref[EncList.free_count] = REF_TO_INT( ienc );
        EncList.free_count++;

        retval = btrue;
    }

    // enchant "initializer"
    enc_dtor( penc );

    return retval;
}

//--------------------------------------------------------------------------------------------
ENC_REF EncList_allocate( const ENC_REF override )
{
    int    tnc;
    ENC_REF ienc = ( ENC_REF )MAX_ENC;

    ienc = MAX_ENC;
    if ( VALID_ENC_RANGE( override ) )
    {
        ienc = EncList_get_free();
        if ( override != ienc )
        {
            // Picked the wrong one, so put this one back and find the right one

            for ( tnc = 0; tnc < MAX_ENC; tnc++ )
            {
                if ( override == EncList.free_ref[tnc] )
                {
                    EncList.free_ref[tnc] = REF_TO_INT( ienc );
                    break;
                }
            }

            ienc = override;
        }

        if ( MAX_ENC == ienc )
        {
            log_warning( "EncList_allocate() - failed to override an enchant? enchant %d already spawned? \n", REF_TO_INT( override ) );
        }
    }
    else
    {
        ienc = EncList_get_free();
        if ( MAX_ENC == ienc )
        {
            log_warning( "EncList_allocate() - failed to allocate a new character\n" );
        }
    }

    if ( MAX_ENC != ienc )
    {
        EGO_OBJECT_ALLOCATE( EncList.lst + ienc, REF_TO_INT( ienc ) );
    }

    if ( ALLOCATED_ENC( ienc ) )
    {
        enc_ctor( EncList.lst + ienc );
    }

    return ienc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t remove_enchant( const ENC_REF by_reference ienc )
{
    /// @details ZZ@> This function removes a specific enchantment and adds it to the unused list

    int     iwave;
    CHR_REF overlay_ref;
    ENC_REF ienc_last, ienc_now;
    int add_type, set_type;

    enc_t * penc;
    eve_t * peve;
    CHR_REF itarget, ispawner;

    if ( !ALLOCATED_ENC( ienc ) ) return bfalse;
    penc = EncList.lst + ienc;

    itarget  = penc->target_ref;
    ispawner = penc->spawner_ref;
    peve     = enc_get_peve( ienc );

    // Unsparkle the spellbook
    if ( INGAME_CHR( ispawner ) )
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
    if ( INGAME_CHR( itarget ) )
    {
        chr_t * ptarget = ChrList.lst + itarget;
        reset_character_alpha( ptarget->holdingwhich[SLOT_LEFT] );
        reset_character_alpha( ptarget->holdingwhich[SLOT_RIGHT] );
    }

    // Unlink it from the spawner (if possible)
    if ( ALLOCATED_CHR( penc->spawner_ref ) )
    {
        chr_t * pspawner = ChrList.lst + penc->spawner_ref;

        if ( ienc == pspawner->undoenchant )
        {
            pspawner->undoenchant = MAX_ENC;
        }
    }

    // Unlink it from the target
    if ( ALLOCATED_CHR( penc->target_ref ) )
    {
        chr_t * ptarget =  ChrList.lst + penc->target_ref;

        if ( ptarget->firstenchant == ienc )
        {
            // It was the first in the list
            ptarget->firstenchant = EncList.lst[ienc].nextenchant_ref;
        }
        else
        {
            // Search until we find it
            ienc_last = ienc_now = ptarget->firstenchant;
            while ( MAX_ENC != ienc_now && ienc_now != ienc )
            {
                ienc_last    = ienc_now;
                ienc_now = EncList.lst[ienc_now].nextenchant_ref;
            }

            // Relink the last enchantment
            if ( ienc_now == ienc )
            {
                EncList.lst[ienc_last].nextenchant_ref = EncList.lst[ienc].nextenchant_ref;
            }
        }
    }

    // Kill overlay too...
    overlay_ref = penc->overlay_ref;
    if ( INGAME_CHR( overlay_ref ) )
    {
        chr_t * povl = ChrList.lst + overlay_ref;

        if ( povl->invictus )
        {
            chr_get_pteam_base( overlay_ref )->morale++;
        }

        kill_character( overlay_ref, ( CHR_REF )MAX_CHR, btrue );
    }

    // nothing above this demends on having a valid enchant profile
    if ( NULL != peve )
    {
        // Play the end sound
        iwave = peve->endsound_index;
        if ( VALID_SND( iwave ) )
        {
            PRO_REF imodel = penc->spawnermodel_ref;
            if ( LOADED_PRO( imodel ) )
            {
                if ( INGAME_CHR( itarget ) )
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

        // Remove see kurse enchant
        if ( INGAME_CHR( itarget ) )
        {
            chr_t * ptarget = ChrList.lst + penc->target_ref;

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
    if ( INGAME_CHR( itarget ) && NULL != peve && peve->killtargetonend )
    {
        chr_t * ptarget = ChrList.lst + itarget;
        if ( ptarget->invictus )  chr_get_pteam_base( itarget )->morale++;

        //ptarget->invictus = bfalse;   /// @note ZF@> no longer needed because ignoreinvictus is added in kill_character()?
        kill_character( itarget, ( CHR_REF )MAX_CHR, btrue );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ENC_REF enchant_value_filled( const ENC_REF by_reference  ienc, int value_idx )
{
    /// @details ZZ@> This function returns MAX_ENC if the enchantment's target has no conflicting
    ///    set values in its other enchantments.  Otherwise it returns the ienc
    ///    of the conflicting enchantment

    CHR_REF character;
    ENC_REF currenchant;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return ( ENC_REF )MAX_ENC;

    if ( !INGAME_ENC( ienc ) ) return ( ENC_REF )MAX_ENC;

    character = EncList.lst[ienc].target_ref;

    currenchant = ChrList.lst[character].firstenchant;
    while ( currenchant != MAX_ENC )
    {
        if ( INGAME_ENC( currenchant ) && EncList.lst[currenchant].setyesno[value_idx] )
        {
            break;
        }
        currenchant = EncList.lst[currenchant].nextenchant_ref;
    }

    return currenchant;
}

//--------------------------------------------------------------------------------------------
void enchant_apply_set( const ENC_REF by_reference  ienc, int value_idx, const PRO_REF by_reference profile )
{
    /// @details ZZ@> This function sets and saves one of the character's stats

    ENC_REF conflict;
    CHR_REF character;
    enc_t * penc;
    eve_t * peve;
    chr_t * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return;

    if ( !INGAME_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    peve = pro_get_peve( profile );
    if ( NULL == peve ) return;

    penc->setyesno[value_idx] = bfalse;
    if ( peve->setyesno[value_idx] )
    {
        conflict = enchant_value_filled( ienc, value_idx );
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
                    enchant_remove_set( conflict, value_idx );
                }
            }

            // Set the value, and save the character's real stat
            if ( INGAME_CHR( penc->target_ref ) )
            {
                character = penc->target_ref;
                ptarget = ChrList.lst + character;

                penc->setyesno[value_idx] = btrue;

                switch ( value_idx )
                {
                    case SETDAMAGETYPE:
                        penc->setsave[value_idx]  = ptarget->damagetargettype;
                        ptarget->damagetargettype = peve->setvalue[value_idx];
                        break;

                    case SETNUMBEROFJUMPS:
                        penc->setsave[value_idx] = ptarget->jumpnumberreset;
                        ptarget->jumpnumberreset = peve->setvalue[value_idx];
                        break;

                    case SETLIFEBARCOLOR:
                        penc->setsave[value_idx] = ptarget->lifecolor;
                        ptarget->lifecolor       = peve->setvalue[value_idx];
                        break;

                    case SETMANABARCOLOR:
                        penc->setsave[value_idx] = ptarget->manacolor;
                        ptarget->manacolor       = peve->setvalue[value_idx];
                        break;

                    case SETSLASHMODIFIER:
                        penc->setsave[value_idx]              = ptarget->damagemodifier[DAMAGE_SLASH];
                        ptarget->damagemodifier[DAMAGE_SLASH] = peve->setvalue[value_idx];
                        break;

                    case SETCRUSHMODIFIER:
                        penc->setsave[value_idx]              = ptarget->damagemodifier[DAMAGE_CRUSH];
                        ptarget->damagemodifier[DAMAGE_CRUSH] = peve->setvalue[value_idx];
                        break;

                    case SETPOKEMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damagemodifier[DAMAGE_POKE];
                        ptarget->damagemodifier[DAMAGE_POKE] = peve->setvalue[value_idx];
                        break;

                    case SETHOLYMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damagemodifier[DAMAGE_HOLY];
                        ptarget->damagemodifier[DAMAGE_HOLY] = peve->setvalue[value_idx];
                        break;

                    case SETEVILMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damagemodifier[DAMAGE_EVIL];
                        ptarget->damagemodifier[DAMAGE_EVIL] = peve->setvalue[value_idx];
                        break;

                    case SETFIREMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damagemodifier[DAMAGE_FIRE];
                        ptarget->damagemodifier[DAMAGE_FIRE] = peve->setvalue[value_idx];
                        break;

                    case SETICEMODIFIER:
                        penc->setsave[value_idx]            = ptarget->damagemodifier[DAMAGE_ICE];
                        ptarget->damagemodifier[DAMAGE_ICE] = peve->setvalue[value_idx];
                        break;

                    case SETZAPMODIFIER:
                        penc->setsave[value_idx]            = ptarget->damagemodifier[DAMAGE_ZAP];
                        ptarget->damagemodifier[DAMAGE_ZAP] = peve->setvalue[value_idx];
                        break;

                    case SETFLASHINGAND:
                        penc->setsave[value_idx] = ptarget->flashand;
                        ptarget->flashand        = peve->setvalue[value_idx];
                        break;

                    case SETLIGHTBLEND:
                        penc->setsave[value_idx] = ptarget->inst.light;
                        chr_set_light( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETALPHABLEND:
                        penc->setsave[value_idx] = ptarget->inst.alpha;
                        chr_set_alpha( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETSHEEN:
                        penc->setsave[value_idx] = ptarget->inst.sheen;
                        chr_set_sheen( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETFLYTOHEIGHT:
                        penc->setsave[value_idx] = ptarget->flyheight;
                        if ( ptarget->flyheight == 0 && ptarget->pos.z > -2 )
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
void enchant_apply_add( const ENC_REF by_reference ienc, int value_idx, const EVE_REF by_reference ieve )
{
    /// @details ZZ@> This function does cumulative modification to character stats

    int valuetoadd, newvalue;
    float fvaluetoadd, fnewvalue;
    CHR_REF character;
    enc_t * penc;
    eve_t * peve;
    chr_t * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_ADD ) return;

    if ( !INGAME_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    if ( ieve >= MAX_EVE || !EveStack.lst[ieve].loaded ) return;
    peve = EveStack.lst + ieve;

    if ( !peve->addyesno[value_idx] )
    {
        penc->addyesno[value_idx] = bfalse;
        penc->addsave[value_idx]  = 0.0f;
        return;
    }

    if ( !INGAME_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = ChrList.lst + character;

    valuetoadd  = 0;
    fvaluetoadd = 0.0f;
    switch ( value_idx )
    {
        case ADDJUMPPOWER:
            fnewvalue = ptarget->jump_power;
            fvaluetoadd = peve->addvalue[value_idx];
            fgetadd( 0.0f, fnewvalue, 30.0f, &fvaluetoadd );
            ptarget->jump_power += fvaluetoadd;
            break;

        case ADDBUMPDAMPEN:
            fnewvalue = ptarget->phys.bumpdampen;
            fvaluetoadd = peve->addvalue[value_idx];
            fgetadd( 0.0f, fnewvalue, 1.0f, &fvaluetoadd );
            ptarget->phys.bumpdampen += fvaluetoadd;
            break;

        case ADDBOUNCINESS:
            fnewvalue = ptarget->phys.dampen;
            fvaluetoadd = peve->addvalue[value_idx];
            fgetadd( 0.0f, fnewvalue, 0.95f, &fvaluetoadd );
            ptarget->phys.dampen += fvaluetoadd;
            break;

        case ADDDAMAGE:
            newvalue = ptarget->damageboost;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 4096, &valuetoadd );
            ptarget->damageboost += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDSIZE:
            fnewvalue = ptarget->fat_goto;
            fvaluetoadd = peve->addvalue[value_idx];
            fgetadd( 0.5f, fnewvalue, 2.0f, &fvaluetoadd );
            ptarget->fat_goto += fvaluetoadd;
            ptarget->fat_goto_time = SIZETIME;
            break;

        case ADDACCEL:
            fnewvalue = ptarget->maxaccel;
            fvaluetoadd = peve->addvalue[value_idx];
            fgetadd( 0.0f, fnewvalue, 1.50f, &fvaluetoadd );
            ptarget->maxaccel += fvaluetoadd;
            break;

        case ADDRED:
            newvalue = ptarget->inst.redshift;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr_set_redshift( ptarget, ptarget->inst.redshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDGRN:
            newvalue = ptarget->inst.grnshift;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr_set_grnshift( ptarget, ptarget->inst.grnshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDBLU:
            newvalue = ptarget->inst.blushift;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr_set_blushift( ptarget, ptarget->inst.blushift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDDEFENSE:
            newvalue = ptarget->defense;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 55, newvalue, 255, &valuetoadd );  // Don't fix again!  //ZF> why limit min to 55?
            ptarget->defense += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDMANA:
            newvalue = ptarget->manamax;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->manamax += valuetoadd;
            //ptarget->mana    += valuetoadd;
            //if ( ptarget->mana < 0 )  ptarget->mana = 0;
            if ( ptarget->mana < ptarget->manamax )  ptarget->mana = ptarget->manamax;
            fvaluetoadd = valuetoadd;
            break;

        case ADDLIFE:
            newvalue = ptarget->lifemax;
            valuetoadd = peve->addvalue[value_idx];
            getadd( LOWSTAT, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->lifemax += valuetoadd;
            //ptarget->life += valuetoadd;
            //if ( ptarget->life < 1 )  ptarget->life = 1;
            if ( ptarget->life < ptarget->lifemax )  ptarget->life = ptarget->lifemax;
            fvaluetoadd = valuetoadd;
            break;

        case ADDSTRENGTH:
            newvalue = ptarget->strength;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->strength += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDWISDOM:
            newvalue = ptarget->wisdom;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->wisdom += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDINTELLIGENCE:
            newvalue = ptarget->intelligence;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->intelligence += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDDEXTERITY:
            newvalue = ptarget->dexterity;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->dexterity += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;
    }

    // save whether there was any change in the value
    penc->addyesno[value_idx] = ( 0.0f != fvaluetoadd );

    // Save the value for undo
    penc->addsave[value_idx]  = valuetoadd;
}

//--------------------------------------------------------------------------------------------
bool_t  enc_free( enc_t * penc )
{
    if ( !ALLOCATED_PENC( penc ) ) return bfalse;

    // nothing to do yet

    return btrue;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_ctor( enc_t * penc )
{
    ego_object_base_t save_base;
    ego_object_base_t * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_CONSTRUCTING_PBASE( pbase ) ) return penc;

    memcpy( &save_base, pbase, sizeof( ego_object_base_t ) );

    memset( penc, 0, sizeof( *penc ) );

    // restore the base object data
    memcpy( pbase, &save_base, sizeof( ego_object_base_t ) );

    penc->profile_ref      = ( PRO_REF )MAX_PROFILE;
    penc->eve_ref          = ( EVE_REF )MAX_EVE;

    penc->target_ref       = ( CHR_REF )MAX_CHR;
    penc->owner_ref        = ( CHR_REF )MAX_CHR;
    penc->spawner_ref      = ( CHR_REF )MAX_CHR;
    penc->spawnermodel_ref = ( PRO_REF )MAX_PROFILE;
    penc->overlay_ref      = ( CHR_REF )MAX_CHR;

    penc->nextenchant_ref  = ( ENC_REF )MAX_ENC;

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_init( enc_t * penc )
{
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase ) return NULL;

    if ( !STATE_INITIALIZING_PBASE( pbase ) ) return penc;

    // do nothing but set the state to the next value
    pbase->state = ego_object_active;

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_deinit( enc_t * penc )
{
    /// @details BB@> deinitialize the character data

    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase ) return NULL;

    if ( !STATE_DEINITIALIZING_PBASE( pbase ) ) return penc;

    enc_free( penc );

    pbase->state = ego_object_destructing;

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_dtor( enc_t * penc )
{
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase ) return NULL;

    if ( !STATE_DEINITIALIZING_PBASE( pbase ) ) return penc;

    // Destroy the base object.
    // Sets the state to ego_object_terminated automatically.
    EGO_OBJECT_TERMINATE( penc );

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_active( enc_t * penc )
{
    // there's nothing to configure if the object is active...

    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    if ( !STATE_ACTIVE_PBASE( pbase ) ) return penc;

    // if anything was needed, it would go here

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_run_config( enc_t * penc )
{
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // set the object to deinitialize if it is not "dangerous" and if was requested
    if ( pbase->kill_me )
    {
        if ( pbase->state > ego_object_constructing && pbase->state < ego_object_deinitializing )
        {
            pbase->state = ego_object_deinitializing;
        }
    }

    switch ( pbase->state )
    {
        default:
        case ego_object_invalid:
            penc = NULL;
            break;

        case ego_object_constructing:
            penc = enc_ctor( penc );
            break;

        case ego_object_initializing:
            penc = enc_config_init( penc );
            break;

        case ego_object_active:
            penc = enc_config_active( penc );
            break;

        case ego_object_deinitializing:
            penc = enc_config_deinit( penc );
            break;

        case ego_object_destructing:
            penc = enc_dtor( penc );
            break;

        case ego_object_waiting:
        case ego_object_terminated:
            /* do nothing */
            break;
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
ENC_REF spawn_one_enchant( const CHR_REF by_reference owner, const CHR_REF by_reference target, const CHR_REF by_reference spawner, const ENC_REF by_reference enc_override, const PRO_REF by_reference modeloptional )
{
    /// @details ZZ@> This function enchants a target, returning the enchantment index or MAX_ENC
    ///    if failed

    ENC_REF ienc;
    EVE_REF ieve;
    CHR_REF overlay;
    PRO_REF iprofile;
    int add_type, set_type;
    eve_t * peve;
    enc_t * penc;
    chr_t * ptarget;

    CHR_REF loc_target;

    // Target must both be alive and on and valid
    loc_target = target;
    if ( !INGAME_CHR( loc_target ) )
    {
        log_warning( "spawn_one_enchant() - failed because loc_target does not exist.\n" );
        return ( ENC_REF )MAX_ENC;
    }
    ptarget = ChrList.lst + loc_target;

    // you should be able to enchant dead stuff to raise the dead...
    // if( !ptarget->alive ) return (ENC_REF)MAX_ENC;

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
            log_warning( "spawn_one_enchant() - no valid profile for the spawning character \"%s\"(%d).\n", ChrList.lst[spawner].obj_base._name, REF_TO_INT( spawner ) );
            return ( ENC_REF )MAX_ENC;
        }
    }

    ieve = pro_get_ieve( iprofile );
    if ( !LOADED_EVE( ieve ) )
    {
        log_warning( "spawn_one_enchant() - the object \"%s\"(%d) does not have an enchant profile.\n", ProList.lst[iprofile].name, REF_TO_INT( iprofile ) );

        return ( ENC_REF )MAX_ENC;
    }
    peve = EveStack.lst + ieve;

    // Owner must both be alive and on and valid if it isn't a stayifnoowner enchant
    if ( !peve->stayifnoowner && ( !INGAME_CHR( owner ) || !ChrList.lst[owner].alive ) )
    {
        log_warning( "spawn_one_enchant() - failed because the required enchant owner cannot be found.\n" );
        return ( ENC_REF )MAX_ENC;
    }

    // do retargeting, if necessary
    // Should it choose an inhand item?
    if ( peve->retarget )
    {
        // Left, right, or both are valid
        if ( INGAME_CHR( ptarget->holdingwhich[SLOT_LEFT] ) )
        {
            // Only right hand is valid
            loc_target = ptarget->holdingwhich[SLOT_RIGHT];
        }
        else if ( INGAME_CHR( ptarget->holdingwhich[SLOT_LEFT] ) )
        {
            // Pick left hand
            loc_target = ptarget->holdingwhich[SLOT_LEFT];
        }
        else
        {
            // No weapons to pick
            loc_target = ( CHR_REF )MAX_CHR;
        }
    }

    // make sure the loc_target is valid
    if ( !INGAME_CHR( loc_target ) || !ptarget->alive )
    {
        log_warning( "spawn_one_enchant() - failed because the loc_target is not alive.\n" );
        return ( ENC_REF )MAX_ENC;
    }
    ptarget = ChrList.lst + loc_target;

    // Check peve->dontdamagetype
    if ( peve->dontdamagetype != DAMAGE_NONE )
    {
        if (( ptarget->damagemodifier[peve->dontdamagetype]&DAMAGESHIFT ) >= 3 ||
            ptarget->damagemodifier[peve->dontdamagetype]&DAMAGECHARGE )
        {
            log_warning( "spawn_one_enchant() - failed because the loc_target is immune to the enchant.\n" );
            return ( ENC_REF )MAX_ENC;
        }
    }

    // Check peve->onlydamagetype
    if ( peve->onlydamagetype != DAMAGE_NONE )
    {
        if ( ptarget->damagetargettype != peve->onlydamagetype )
        {
            log_warning( "spawn_one_enchant() - failed because the loc_target not have the right damagetargettype.\n" );
            return ( ENC_REF )MAX_ENC;
        }
    }

    // Find an enchant index to use
    ienc = EncList_allocate( enc_override );

    if ( !ALLOCATED_ENC( ienc ) )
    {
        log_warning( "spawn_one_enchant() - could not allocate an enchant.\n" );
        return ( ENC_REF )MAX_ENC;
    }
    penc = EncList.lst + ienc;

    // turn the enchant on here. you can't fail to spawn after this point.
    EGO_OBJECT_ACTIVATE( penc, peve->name );

    penc->target_ref       = INGAME_CHR( loc_target ) ? loc_target  : ( CHR_REF )MAX_CHR;
    penc->owner_ref        = INGAME_CHR( owner ) ? owner       : ( CHR_REF )MAX_CHR;
    penc->spawner_ref      = INGAME_CHR( spawner ) ? spawner     : ( CHR_REF )MAX_CHR;
    penc->spawnermodel_ref = chr_get_ipro( spawner );

    if ( INGAME_CHR( spawner ) )
    {
        ChrList.lst[spawner].undoenchant = ienc;
    }

    penc->eve_ref      = ieve;
    penc->profile_ref  = iprofile;
    penc->time         = peve->time;
    penc->spawntime    = 1;
    penc->owner_mana    = peve->owner_mana;
    penc->owner_life    = peve->owner_life;
    penc->target_mana   = peve->target_mana;
    penc->target_life   = peve->target_life;

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
        overlay = spawn_one_character( ptarget->pos, iprofile, ptarget->team, 0, ptarget->facing_z, NULL, ( CHR_REF )MAX_CHR );
        if ( INGAME_CHR( overlay ) )
        {
            chr_t * povl;
            mad_t * povl_mad;
            int action;

            povl     = ChrList.lst + overlay;
            povl_mad = chr_get_pmad( overlay );

            penc->overlay_ref = overlay;  // Kill this character on end...
            povl->ai.target   = loc_target;
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
EVE_REF load_one_enchant_profile( const char* szLoadName, const EVE_REF by_reference ieve )
{
    /// @details ZZ@> This function loads an enchantment profile into the EveStack

    EVE_REF retval = ( EVE_REF )MAX_EVE;

    if ( VALID_EVE_RANGE( ieve ) )
    {
        eve_t * peve = EveStack.lst + ieve;

        if ( NULL != load_one_enchant_file( szLoadName, peve ) )
        {
            retval = ieve;

            // limit the endsound_index
            peve->endsound_index = CLIP( peve->endsound_index, INVALID_SOUND, MAX_WAVE );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void enchant_remove_set( const ENC_REF by_reference ienc, int value_idx )
{
    /// @details ZZ@> This function unsets a set value
    CHR_REF character;
    enc_t * penc;
    chr_t * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return;

    if ( !ALLOCATED_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    if ( value_idx >= MAX_ENCHANT_SET || !penc->setyesno[value_idx] ) return;

    if ( !INGAME_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget   = ChrList.lst + penc->target_ref;

    switch ( value_idx )
    {
        case SETDAMAGETYPE:
            ptarget->damagetargettype = penc->setsave[value_idx];
            break;

        case SETNUMBEROFJUMPS:
            ptarget->jumpnumberreset = penc->setsave[value_idx];
            break;

        case SETLIFEBARCOLOR:
            ptarget->lifecolor = penc->setsave[value_idx];
            break;

        case SETMANABARCOLOR:
            ptarget->manacolor = penc->setsave[value_idx];
            break;

        case SETSLASHMODIFIER:
            ptarget->damagemodifier[DAMAGE_SLASH] = penc->setsave[value_idx];
            break;

        case SETCRUSHMODIFIER:
            ptarget->damagemodifier[DAMAGE_CRUSH] = penc->setsave[value_idx];
            break;

        case SETPOKEMODIFIER:
            ptarget->damagemodifier[DAMAGE_POKE] = penc->setsave[value_idx];
            break;

        case SETHOLYMODIFIER:
            ptarget->damagemodifier[DAMAGE_HOLY] = penc->setsave[value_idx];
            break;

        case SETEVILMODIFIER:
            ptarget->damagemodifier[DAMAGE_EVIL] = penc->setsave[value_idx];
            break;

        case SETFIREMODIFIER:
            ptarget->damagemodifier[DAMAGE_FIRE] = penc->setsave[value_idx];
            break;

        case SETICEMODIFIER:
            ptarget->damagemodifier[DAMAGE_ICE] = penc->setsave[value_idx];
            break;

        case SETZAPMODIFIER:
            ptarget->damagemodifier[DAMAGE_ZAP] = penc->setsave[value_idx];
            break;

        case SETFLASHINGAND:
            ptarget->flashand = penc->setsave[value_idx];
            break;

        case SETLIGHTBLEND:
            chr_set_light( ptarget, penc->setsave[value_idx] );
            break;

        case SETALPHABLEND:
            chr_set_alpha( ptarget, penc->setsave[value_idx] );
            break;

        case SETSHEEN:
            chr_set_sheen( ptarget, penc->setsave[value_idx] );
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
            change_character( character, ptarget->basemodel, penc->setsave[value_idx], ENC_LEAVE_ALL );
            break;

        case SETCHANNEL:
            ptarget->canchannel = ( 0 != penc->setsave[value_idx] );
            break;
    }

    penc->setyesno[value_idx] = bfalse;
}

//--------------------------------------------------------------------------------------------
void enchant_remove_add( const ENC_REF by_reference ienc, int value_idx )
{
    /// @details ZZ@> This function undoes cumulative modification to character stats

    float fvaluetoadd;
    int valuetoadd;
    CHR_REF character;
    enc_t * penc;
    chr_t * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_ADD ) return;

    if ( !ALLOCATED_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    if ( !INGAME_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = ChrList.lst + penc->target_ref;

    if ( penc->addyesno[value_idx] )
    {
        switch ( value_idx )
        {
            case ADDJUMPPOWER:
                fvaluetoadd = penc->addsave[value_idx] / 16.0f;
                ptarget->jump_power -= fvaluetoadd;
                break;

            case ADDBUMPDAMPEN:
                fvaluetoadd = penc->addsave[value_idx] / 128.0f;
                ptarget->phys.bumpdampen -= fvaluetoadd;
                break;

            case ADDBOUNCINESS:
                fvaluetoadd = penc->addsave[value_idx] / 128.0f;
                ptarget->phys.dampen -= fvaluetoadd;
                break;

            case ADDDAMAGE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->damageboost -= valuetoadd;
                break;

            case ADDSIZE:
                fvaluetoadd = penc->addsave[value_idx] / 128.0f;
                ptarget->fat_goto -= fvaluetoadd;
                ptarget->fat_goto_time = SIZETIME;
                break;

            case ADDACCEL:
                fvaluetoadd = penc->addsave[value_idx] / 1000.0f;
                ptarget->maxaccel -= fvaluetoadd;
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
                valuetoadd = penc->addsave[value_idx];
                ptarget->defense -= valuetoadd;
                break;

            case ADDMANA:
                valuetoadd = penc->addsave[value_idx];
                ptarget->manamax -= valuetoadd;
                ptarget->mana -= valuetoadd;
                if ( ptarget->mana < 0 ) ptarget->mana = 0;
                break;

            case ADDLIFE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->lifemax -= valuetoadd;
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
        }

        penc->addyesno[value_idx] = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_all_eve()
{
    EVE_REF cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        eve_init( EveStack.lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_eve()
{
    EVE_REF cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        release_one_eve( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_eve( const EVE_REF by_reference ieve )
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

    ENC_REF  cnt;
    int      tnc;
    FACING_T facing;
    CHR_REF  owner, target;
    EVE_REF  eve;

    // the following functions should not be done the first time through the update loop
    if ( clock_wld == 0 ) return;

    // check to see whether the enchant needs to spawn some particles
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        enc_t * penc;
        eve_t * peve;
        chr_t * ptarget;

        if ( !INGAME_ENC( cnt ) ) continue;
        penc = EncList.lst + cnt;

        if ( penc->spawntime > 0 ) penc->spawntime--;
        if ( penc->spawntime > 0 ) continue;

        peve = enc_get_peve( cnt );
        if ( NULL == peve ) continue;

        penc->spawntime = peve->contspawn_delay;

        if ( peve->contspawn_amount <= 0 ) continue;

        if ( !INGAME_CHR( penc->target_ref ) ) continue;
        ptarget = ChrList.lst + penc->target_ref;

        facing = ptarget->facing_z;
        for ( tnc = 0; tnc < peve->contspawn_amount; tnc++ )
        {
            spawn_one_particle( ptarget->pos, facing, penc->profile_ref, peve->contspawn_pip,
                                ( CHR_REF )MAX_CHR, GRIP_LAST, chr_get_iteam( penc->owner_ref ), penc->owner_ref, ( PRT_REF )TOTAL_MAX_PRT, tnc, ( CHR_REF )MAX_CHR, EGO_OBJECT_DO_ALLOCATE );

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

            if ( !INGAME_ENC( cnt ) ) continue;
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
                    ChrList.lst[owner].life += penc->owner_life;
                    if ( ChrList.lst[owner].life <= 0 )
                    {
                        kill_character( owner, target, bfalse );
                    }

                    if ( ChrList.lst[owner].life > ChrList.lst[owner].lifemax )
                    {
                        ChrList.lst[owner].life = ChrList.lst[owner].lifemax;
                    }

                    // Change mana
                    mana_paid = cost_mana( owner, -penc->owner_mana, target );
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
                if ( INGAME_ENC( cnt ) )
                {
                    if ( ChrList.lst[target].alive )
                    {
                        bool_t mana_paid;

                        // Change life
                        ChrList.lst[target].life += penc->target_life;
                        if ( ChrList.lst[target].life <= 0 )
                        {
                            kill_character( target, owner, bfalse );
                        }
                        if ( ChrList.lst[target].life > ChrList.lst[target].lifemax )
                        {
                            ChrList.lst[target].life = ChrList.lst[target].lifemax;
                        }

                        // Change mana
                        mana_paid = cost_mana( target, -penc->target_mana, owner );
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
ENC_REF cleanup_enchant_list( const ENC_REF by_reference ienc )
{
    /// @details BB@> remove all the dead enchants from the enchant list
    ///     and report back the first non-dead enchant in the list.

    ENC_REF first_valid_enchant;
    ENC_REF enc_now, enc_next;

    first_valid_enchant = MAX_ENC;
    enc_now             = ienc;
    while ( MAX_ENC != enc_now )
    {
        enc_next = EncList.lst[enc_now].nextenchant_ref;

        if ( !INGAME_ENC( enc_now ) )
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

    ENC_REF cnt;

    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        ENC_REF * enc_lst;
        enc_t * penc;
        eve_t * peve;
        bool_t  do_remove;

        // allow inactive (but not terminated) enchants to be cleaned up
        if ( !ALLOCATED_ENC( cnt ) ) continue;
        penc = EncList.lst + cnt;

        // try to determine something about the parent
        enc_lst = NULL;
        if ( INGAME_CHR( penc->target_ref ) )
        {
            // this is linked to a known character
            enc_lst = &( ChrList.lst[penc->target_ref].firstenchant );
        }

        if ( !LOADED_EVE( penc->eve_ref ) )
        {
            // this should never happen
            EGOBOO_ASSERT( bfalse );
            continue;
        }
        peve = EveStack.lst + penc->eve_ref;

        do_remove = bfalse;
        if ( WAITING_PBASE( POBJ_GET_PBASE( penc ) ) )
        {
            // the enchant has been marked for removal
            do_remove = btrue;
        }
        else if ( !INGAME_CHR( penc->owner_ref ) && !peve->stayifnoowner )
        {
            // the enchant's owner has died
            do_remove = btrue;
        }
        else if ( !INGAME_CHR( penc->target_ref ) && !peve->stayiftargetdead )
        {
            // the enchant's target has died
            do_remove = btrue;
        }
        else if ( INGAME_CHR( penc->owner_ref ) && peve->endifcantpay )
        {
            // Undo enchants that cannot be sustained anymore
            if ( ChrList.lst[penc->owner_ref].mana == 0 ) do_remove = btrue;
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
size_t spawn_all_delayed_enchants()
{
    return 0;
}

//--------------------------------------------------------------------------------------------
void bump_all_enchants_update_counters()
{
    ENC_REF cnt;

    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        ego_object_base_t * pbase;

        pbase = POBJ_GET_PBASE( EncList.lst + cnt );
        if ( !ACTIVE_PBASE( pbase ) ) continue;

        pbase->update_count++;
    }
}

//--------------------------------------------------------------------------------------------
bool_t enc_request_terminate( const ENC_REF by_reference ienc )
{
    if ( !ALLOCATED_ENC( ienc ) || TERMINATED_ENC( ienc ) ) return bfalse;

    EGO_OBJECT_REQUST_TERMINATE( EncList.lst + ienc );

    return btrue;
}


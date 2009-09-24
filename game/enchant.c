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

/* Egoboo - enchant.c handles enchantments attached to objects
 */
#include "enchant.h"
#include "char.h"
#include "mad.h"
#include "profile.h"

#include "sound.h"
#include "camera.h"
#include "game.h"
#include "script_functions.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
DECLARE_STACK( ACCESS_TYPE_NONE, eve_t, EveStack );
DECLARE_LIST ( ACCESS_TYPE_NONE, enc_t, EncList );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool_t EncList_free_one( Uint16 ienc )
{
    // ZZ> This function sticks an enchant back on the free enchant stack

    bool_t retval;

    if ( !VALID_ENC_RANGE(ienc) ) return bfalse;

    // enchant "destructor"
    // sets all boolean values to false, incluting the "on" flag
    memset( EncList.lst + ienc, 0, sizeof(enc_t) );

#if defined(DEBUG)
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

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t remove_enchant( Uint16 ienc )
{
    // ZZ> This function removes a specific enchantment and adds it to the unused list

    Sint16 iwave;
    Uint16 itarget, ispawner;
    Uint16 overlay_ref;
    Uint16 lastenchant, currentenchant;
    int add;

    enc_t * penc;
    eve_t * peve;

    if ( INVALID_ENC(ienc) ) return bfalse;
    penc = EncList.lst + ienc;

    // Unsparkle the spellbook
    ispawner = penc->spawner_ref;
    if ( VALID_CHR(ispawner) )
    {
        ChrList.lst[ispawner].sparkle = NOSPARKLE;

        // Make the spawner unable to undo the enchantment
        if ( ChrList.lst[ispawner].undoenchant == ienc )
        {
            ChrList.lst[ispawner].undoenchant = MAX_ENC;
        }
    }

    // Unset enchant values, doing morph last
    unset_enchant_value( ienc, SETDAMAGETYPE );
    unset_enchant_value( ienc, SETNUMBEROFJUMPS );
    unset_enchant_value( ienc, SETLIFEBARCOLOR );
    unset_enchant_value( ienc, SETMANABARCOLOR );
    unset_enchant_value( ienc, SETSLASHMODIFIER );
    unset_enchant_value( ienc, SETCRUSHMODIFIER );
    unset_enchant_value( ienc, SETPOKEMODIFIER );
    unset_enchant_value( ienc, SETHOLYMODIFIER );
    unset_enchant_value( ienc, SETEVILMODIFIER );
    unset_enchant_value( ienc, SETFIREMODIFIER );
    unset_enchant_value( ienc, SETICEMODIFIER );
    unset_enchant_value( ienc, SETZAPMODIFIER );
    unset_enchant_value( ienc, SETFLASHINGAND );
    unset_enchant_value( ienc, SETLIGHTBLEND );
    unset_enchant_value( ienc, SETALPHABLEND );
    unset_enchant_value( ienc, SETSHEEN );
    unset_enchant_value( ienc, SETFLYTOHEIGHT );
    unset_enchant_value( ienc, SETWALKONWATER );
    unset_enchant_value( ienc, SETCANSEEINVISIBLE );
    unset_enchant_value( ienc, SETMISSILETREATMENT );
    unset_enchant_value( ienc, SETCOSTFOREACHMISSILE );
    unset_enchant_value( ienc, SETCHANNEL );
    unset_enchant_value( ienc, SETMORPH );

    // Remove all of the cumulative values
    for ( add = 0; add < MAX_ENCHANT_ADD; add++ )
    {
        remove_enchant_value( ienc, add );
    }

    // Now fix dem weapons
    if ( VALID_CHR( penc->target_ref ) )
    {
        reset_character_alpha( ChrList.lst[penc->target_ref].holdingwhich[SLOT_LEFT] );
        reset_character_alpha( ChrList.lst[penc->target_ref].holdingwhich[SLOT_RIGHT] );
    }

    // Unlink it
    if ( VALID_CHR(penc->target_ref) )
    {
        if ( ChrList.lst[penc->target_ref].firstenchant == ienc )
        {
            // It was the first in the list
            ChrList.lst[penc->target_ref].firstenchant = penc->nextenchant_ref;
        }
        else
        {
            // Search until we find it
            lastenchant = currentenchant = ChrList.lst[penc->target_ref].firstenchant;
            while ( currentenchant != ienc )
            {
                lastenchant = currentenchant;
                currentenchant = EncList.lst[penc->target_ref].nextenchant_ref;
            }

            // Relink the last enchantment
            EncList.lst[lastenchant].nextenchant_ref = penc->nextenchant_ref;
        }
    }

    // Kill overlay too...
    overlay_ref = penc->overlay_ref;
    if ( VALID_CHR(overlay_ref) )
    {
        if ( ChrList.lst[overlay_ref].invictus )  chr_get_pteam_base(overlay_ref)->morale++;

        ChrList.lst[overlay_ref].invictus = bfalse;
        kill_character( overlay_ref, MAX_CHR );
    }

    // nothing above this demends on having a valid enchant profile
    peve = enc_get_peve( ienc );
    if( NULL != peve )
    {

        // who is the target?
        itarget = penc->target_ref;

        // Play the end sound
        iwave = peve->endsoundindex;
        if ( VALID_SND( iwave ) )
        {
            Uint16 imodel = penc->spawnermodel_ref;
            if ( VALID_PRO( imodel ) )
            {
                if ( VALID_CHR(itarget) )
                {
                    sound_play_chunk(ChrList.lst[itarget].pos_old, pro_get_chunk(imodel,iwave));
                }
                else
                {
                    sound_play_chunk( PCamera->track_pos, pro_get_chunk(imodel,iwave));
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

        // Check to see if the character dies
        if ( peve->killonend )
        {
            if ( VALID_CHR(itarget) )
            {
                if ( ChrList.lst[itarget].invictus )  chr_get_pteam_base(itarget)->morale++;

                ChrList.lst[itarget].invictus = bfalse;
                kill_character( itarget, MAX_CHR );
            }
        }

        // Remove see kurse enchant
        if ( VALID_CHR( itarget ) )
        {
            if ( peve->seekurse && !chr_get_pcap(itarget)->canseekurse )
            {
                ChrList.lst[itarget].canseekurse = bfalse;
            }
        }
    }

    EncList_free_one( ienc );

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 enchant_value_filled( Uint16 ienc, Uint8 valueindex )
{
    // ZZ> This function returns MAX_ENC if the enchantment's target has no conflicting
    //    set values in its other enchantments.  Otherwise it returns the ienc
    //    of the conflicting enchantment
    Uint16 character, currenchant;

    if ( ienc > MAX_ENC || !EncList.lst[ienc].on ) return MAX_ENC;

    character = EncList.lst[ienc].target_ref;

    currenchant = ChrList.lst[character].firstenchant;
    while ( currenchant != MAX_ENC )
    {
        if ( EncList.lst[currenchant].setyesno[valueindex] )
        {
            break;
        }

        currenchant = EncList.lst[currenchant].nextenchant_ref;
    }

    return currenchant;
}

//--------------------------------------------------------------------------------------------
void set_enchant_value( Uint16 ienc, Uint8 valueindex, Uint16 ieve )
{
    // ZZ> This function sets and saves one of the character's stats
    Uint16 conflict, character;
    enc_t * penc;
    eve_t * peve;
    chr_t * ptarget;

    if ( ienc >= MAX_ENC || !EncList.lst[ienc].on) return;
    penc = EncList.lst + ienc;

    if ( ieve >= MAX_EVE || !EveStack.lst[ieve].loaded ) return;
    peve = EveStack.lst + ieve;

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
                    unset_enchant_value( conflict, valueindex );
                }
            }

            // Set the value, and save the character's real stat
            if ( VALID_CHR(penc->target_ref) )
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
                        ptarget->inst.light = peve->setvalue[valueindex];
                        break;

                    case SETALPHABLEND:
                        penc->setsave[valueindex] = ptarget->inst.alpha;
                        ptarget->inst.alpha = peve->setvalue[valueindex];
                        break;

                    case SETSHEEN:
                        penc->setsave[valueindex] = ptarget->inst.sheen;
                        ptarget->inst.sheen = peve->setvalue[valueindex];
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
                        penc->setsave[valueindex] = ptarget->canseeinvisible;
                        ptarget->canseeinvisible = peve->setvalue[valueindex];
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
                        penc->setsave[valueindex] = ptarget->skin;
                        // Special handler for morph
                        change_character( character, ieve, 0, LEAVEALL ); // LEAVEFIRST);
                        ptarget->ai.changed = btrue;
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
void add_enchant_value( Uint16 ienc, Uint8 valueindex, Uint16 ieve )
{
    // ZZ> This function does cumulative modification to character stats
    int valuetoadd, newvalue;
    float fvaluetoadd, fnewvalue;
    Uint16 character;
    enc_t * penc;
    eve_t * peve;
    chr_t * ptarget;

    if ( ienc >= MAX_ENC || !EncList.lst[ienc].on) return;
    penc = EncList.lst + ienc;

    if ( ieve >= MAX_EVE || !EveStack.lst[ieve].loaded ) return;
    peve = EveStack.lst + ieve;

    if ( INVALID_CHR(penc->target_ref) ) return;
    character = penc->target_ref;
    ptarget = ChrList.lst + character;

    valuetoadd = 0;
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
            fnewvalue = ptarget->bumpdampen;
            fvaluetoadd = peve->addvalue[valueindex] / 128.0f;
            fgetadd( 0, fnewvalue, 1.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            ptarget->bumpdampen += fvaluetoadd;
            break;

        case ADDBOUNCINESS:
            fnewvalue = ptarget->dampen;
            fvaluetoadd = peve->addvalue[valueindex] / 128.0f;
            fgetadd( 0, fnewvalue, 0.95f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            ptarget->dampen += fvaluetoadd;
            break;

        case ADDDAMAGE:
            newvalue = ptarget->damageboost;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, 4096, &valuetoadd );
            ptarget->damageboost += valuetoadd;
            break;

        case ADDSIZE:
            fnewvalue = ptarget->fat_goto;
            fvaluetoadd = peve->addvalue[valueindex] / 128.0f;
            fgetadd( 0.5f, fnewvalue, 2.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
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
            ptarget->inst.redshift += valuetoadd;
            break;

        case ADDGRN:
            newvalue = ptarget->inst.grnshift;
            valuetoadd = peve->addvalue[valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            ptarget->inst.grnshift += valuetoadd;
            break;

        case ADDBLU:
            newvalue = ptarget->inst.blushift;
            valuetoadd = peve->addvalue[valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            ptarget->inst.blushift += valuetoadd;
            break;

        case ADDDEFENSE:
            newvalue = ptarget->defense;
            valuetoadd = peve->addvalue[valueindex];
            getadd( 55, newvalue, 255, &valuetoadd );  // Don't fix again!
            ptarget->defense += valuetoadd;
            break;

        case ADDMANA:
            newvalue = ptarget->manamax;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->manamax += valuetoadd;
            ptarget->mana += valuetoadd;
            if ( ptarget->mana < 0 )  ptarget->mana = 0;
            break;

        case ADDLIFE:
            newvalue = ptarget->lifemax;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( LOWSTAT, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->lifemax += valuetoadd;
            ptarget->life += valuetoadd;
            if ( ptarget->life < 1 )  ptarget->life = 1;
            break;

        case ADDSTRENGTH:
            newvalue = ptarget->strength;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->strength += valuetoadd;
            break;

        case ADDWISDOM:
            newvalue = ptarget->wisdom;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->wisdom += valuetoadd;
            break;

        case ADDINTELLIGENCE:
            newvalue = ptarget->intelligence;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->intelligence += valuetoadd;
            break;

        case ADDDEXTERITY:
            newvalue = ptarget->dexterity;
            valuetoadd = peve->addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->dexterity += valuetoadd;
            break;
    }

    penc->addsave[valueindex] = valuetoadd;  // Save the value for undo
}

//--------------------------------------------------------------------------------------------
void enc_init( enc_t * penc )
{
    if( NULL == penc ) return;

    memset( penc, 0, sizeof(enc_t) );

    penc->profile_ref      = MAX_PROFILE;
    penc->eve_ref          = MAX_EVE;

    penc->target_ref       = MAX_CHR;
    penc->owner_ref        = MAX_CHR;
    penc->spawner_ref      = MAX_CHR;
    penc->spawnermodel_ref = MAX_PROFILE;
    penc->overlay_ref      = MAX_CHR;

    penc->nextenchant_ref      = MAX_ENC;
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_enchant( Uint16 owner, Uint16 target, Uint16 spawner, Uint16 enc_override, Uint16 modeloptional )
{
    // ZZ> This function enchants a target, returning the enchantment index or MAX_ENC
    //    if failed
    Uint16 ienc, ieve, iprofile, overlay;
    int add;
    eve_t * peve;
    enc_t * penc;
    chr_t * ptarget;

    // Target must both be alive and on and valid
    if ( INVALID_CHR(target) ) return MAX_ENC;
    ptarget = ChrList.lst + target;

    // you should be able to enchant dead stuff to raise the dead...
    // if( !ptarget->alive ) return MAX_ENC;

    if ( VALID_PRO(modeloptional) )
    {
        // The enchantment type is given explicitly
        iprofile = modeloptional;
        ieve     = pro_get_ieve(modeloptional);
    }
    else
    {
        // The enchantment type is given by the spawner
        iprofile = chr_get_iobj(spawner);
        ieve     = chr_get_ieve(spawner);
    }

    if ( INVALID_EVE(ieve) ) return MAX_ENC;
    peve = EveStack.lst + ieve;

    // Owner must both be alive and on and valid if it isn't a stayifnoowner enchant
    if ( !peve->stayifnoowner && ( INVALID_CHR(owner) || !ChrList.lst[owner].alive ) )
        return MAX_ENC;

    if ( VALID_ENC_RANGE(enc_override) )
    {
        // are we re-enchanting something? enc_override always == MEX_ENC, so there is no knowning
        // what this was really intended to do...

        if( INVALID_ENC(enc_override) )
        {
            ienc = EncList_get_free(enc_override);
        }
        else
        {
            ienc = enc_override;
        }
    }
    else
    {
        // Should it choose an inhand item?
        if ( peve->retarget )
        {
            // Left, right, or both are valid
            if ( VALID_CHR(ptarget->holdingwhich[SLOT_LEFT]) )
            {
                // Only right hand is valid
                target = ptarget->holdingwhich[SLOT_RIGHT];
            }
            else if( VALID_CHR(ptarget->holdingwhich[SLOT_LEFT]) )
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

        if ( INVALID_CHR(target) || !ptarget->alive ) return MAX_ENC;
        ptarget = ChrList.lst + target;

        // Make sure it's valid
        if ( peve->dontdamagetype != DAMAGE_NONE )
        {
            if ( ( ptarget->damagemodifier[peve->dontdamagetype]&DAMAGESHIFT ) >= 3 ||
                   ptarget->damagemodifier[peve->dontdamagetype]&DAMAGECHARGE )
            {
                return MAX_ENC;
            }
        }

        if ( peve->onlydamagetype != DAMAGE_NONE )
        {
            if ( ptarget->damagetargettype != peve->onlydamagetype )
            {
                return MAX_ENC;
            }
        }

        // Find one to use
        ienc = EncList_get_free(enc_override);
    }

    if ( !VALID_ENC_RANGE(ienc) ) return MAX_ENC;
    penc = EncList.lst + ienc;
    ptarget = ChrList.lst + target;

    // initialize the enchant
    enc_init( penc );

    // Make a new one
    penc->on               = btrue;
    penc->target_ref       = VALID_CHR(target)  ? target  : MAX_CHR;
    penc->owner_ref        = VALID_CHR(owner)   ? owner   : MAX_CHR;
    penc->spawner_ref      = VALID_CHR(spawner) ? spawner : MAX_CHR;
    penc->spawnermodel_ref = chr_get_iobj(spawner);

    if ( VALID_CHR(spawner) )
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
    penc->nextenchant_ref     = ptarget->firstenchant;
    ptarget->firstenchant = ienc;

    // Now set all of the specific values, morph first
    set_enchant_value( ienc, SETMORPH, ieve );
    set_enchant_value( ienc, SETDAMAGETYPE, ieve );
    set_enchant_value( ienc, SETNUMBEROFJUMPS, ieve );
    set_enchant_value( ienc, SETLIFEBARCOLOR, ieve );
    set_enchant_value( ienc, SETMANABARCOLOR, ieve );
    set_enchant_value( ienc, SETSLASHMODIFIER, ieve );
    set_enchant_value( ienc, SETCRUSHMODIFIER, ieve );
    set_enchant_value( ienc, SETPOKEMODIFIER, ieve );
    set_enchant_value( ienc, SETHOLYMODIFIER, ieve );
    set_enchant_value( ienc, SETEVILMODIFIER, ieve );
    set_enchant_value( ienc, SETFIREMODIFIER, ieve );
    set_enchant_value( ienc, SETICEMODIFIER, ieve );
    set_enchant_value( ienc, SETZAPMODIFIER, ieve );
    set_enchant_value( ienc, SETFLASHINGAND, ieve );
    set_enchant_value( ienc, SETLIGHTBLEND, ieve );
    set_enchant_value( ienc, SETALPHABLEND, ieve );
    set_enchant_value( ienc, SETSHEEN, ieve );
    set_enchant_value( ienc, SETFLYTOHEIGHT, ieve );
    set_enchant_value( ienc, SETWALKONWATER, ieve );
    set_enchant_value( ienc, SETCANSEEINVISIBLE, ieve );
    set_enchant_value( ienc, SETMISSILETREATMENT, ieve );
    set_enchant_value( ienc, SETCOSTFOREACHMISSILE, ieve );
    set_enchant_value( ienc, SETCHANNEL, ieve );

    // Now do all of the stat adds
    for ( add = 0; add < MAX_ENCHANT_ADD; add++ )
    {
        add_enchant_value( ienc, add, ieve );
    }

    // Create an overlay character?
    if ( peve->spawn_overlay )
    {
        overlay = spawn_one_character( ptarget->pos, iprofile, ptarget->team, 0, ptarget->turn_z, NULL, MAX_CHR );
        if ( VALID_CHR(overlay) )
        {
            chr_t * povl;
            mad_t * povl_mad;

            povl     = ChrList.lst + overlay;
            povl_mad = chr_get_pmad(overlay);

            penc->overlay_ref = overlay;  // Kill this character on end...
            povl->ai.target   = target;
            povl->ai.state    = peve->spawn_overlay;    // ??? WHY DO THIS ???
            povl->is_overlay  = btrue;

            // Start out with ActionMJ...  Object activated
            if ( povl_mad->actionvalid[ACTION_MJ] )
            {
                povl->action = ACTION_MJ;
                povl->inst.lip = 0;
                povl->inst.frame_nxt = povl_mad->actionstart[ACTION_MJ];
                povl->inst.frame_lst = povl->inst.frame_nxt;
                povl->actionready = bfalse;
            }

            povl->inst.light = 254;  // Assume it's transparent...
        }
    }

    // Allow them to see kurses?
    if (peve->seekurse)
    {
        ptarget->canseekurse = btrue;
    }

    return ienc;
}

//--------------------------------------------------------------------------------------------
void disenchant_character( Uint16 cnt )
{
    // ZZ> This function removes all enchantments from a character
    while ( ChrList.lst[cnt].firstenchant != MAX_ENC )
    {
        remove_enchant( ChrList.lst[cnt].firstenchant );
    }
}

//--------------------------------------------------------------------------------------------
void EncList_free_all()
{
    // ZZ> This functions frees all of the enchantments

    int cnt;

    EncList.free_count = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++)
    {
        // reuse this code
        EncList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
Uint16 load_one_enchant_profile( const char* szLoadName, Uint16 ieve )
{
    // ZZ> This function loads an enchantment profile into the EveList

    eve_t * peve;

    if ( ieve > MAX_EVE ) return MAX_EVE;
    peve = EveStack.lst + ieve;

    if( NULL == load_one_enchant_file( szLoadName, peve ) )
    {
        ieve = MAX_EVE;
    }

    return ieve;
}

//--------------------------------------------------------------------------------------------
Uint16 EncList_get_free()
{
    // ZZ> This function returns the next free enchantment or MAX_ENC if there are none

    Uint16 retval = MAX_ENC;

    if ( EncList.free_count > 0 )
    {
        EncList.free_count--;
        retval = EncList.free_ref[EncList.free_count];
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void unset_enchant_value( Uint16 ienc, Uint8 valueindex )
{
    // ZZ> This function unsets a set value
    Uint16 character;
    enc_t * penc;
    chr_t * ptarget;

    if ( INVALID_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    if ( INVALID_CHR(penc->target_ref) ) return;
    character = penc->target_ref;
    ptarget = ChrList.lst + penc->target_ref;

    if ( penc->setyesno[valueindex] )
    {
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
                ptarget->inst.light = penc->setsave[valueindex];
                break;

            case SETALPHABLEND:
                ptarget->inst.alpha = penc->setsave[valueindex];
                break;

            case SETSHEEN:
                ptarget->inst.sheen = penc->setsave[valueindex];
                break;

            case SETFLYTOHEIGHT:
                ptarget->flyheight = penc->setsave[valueindex];
                break;

            case SETWALKONWATER:
                ptarget->waterwalk = penc->setsave[valueindex];
                break;

            case SETCANSEEINVISIBLE:
                ptarget->canseeinvisible = penc->setsave[valueindex];
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
}

//--------------------------------------------------------------------------------------------
void remove_enchant_value( Uint16 ienc, Uint8 valueindex )
{
    // ZZ> This function undoes cumulative modification to character stats
    float fvaluetoadd;
    int valuetoadd;
    Uint16 character;
    enc_t * penc;
    chr_t * ptarget;

    if ( ienc >= MAX_ENC || !EncList.lst[ienc].on) return;
    penc = EncList.lst + ienc;

    if ( INVALID_CHR(penc->target_ref) ) return;
    character = penc->target_ref;
    ptarget = ChrList.lst + penc->target_ref;

    switch ( valueindex )
    {
        case ADDJUMPPOWER:
            fvaluetoadd = penc->addsave[valueindex] / 16.0f;
            ptarget->jump_power -= fvaluetoadd;
            break;

        case ADDBUMPDAMPEN:
            fvaluetoadd = penc->addsave[valueindex] / 128.0f;
            ptarget->bumpdampen -= fvaluetoadd;
            break;

        case ADDBOUNCINESS:
            fvaluetoadd = penc->addsave[valueindex] / 128.0f;
            ptarget->dampen -= fvaluetoadd;
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
            ptarget->inst.redshift -= valuetoadd;
            break;

        case ADDGRN:
            valuetoadd = penc->addsave[valueindex];
            ptarget->inst.grnshift -= valuetoadd;
            break;

        case ADDBLU:
            valuetoadd = penc->addsave[valueindex];
            ptarget->inst.blushift -= valuetoadd;
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
}

//--------------------------------------------------------------------------------------------
Uint16  enc_get_iowner( Uint16 ienc )
{
    enc_t * penc;

    if( INVALID_ENC(ienc) ) return MAX_CHR;
    penc = EncList.lst + ienc;

    if( INVALID_CHR(penc->owner_ref) ) return MAX_CHR;

    return penc->owner_ref;
}

//--------------------------------------------------------------------------------------------
chr_t * enc_get_powner( Uint16 ienc )
{
    enc_t * penc;

    if( INVALID_ENC(ienc) ) return NULL;
    penc = EncList.lst + ienc;

    if( INVALID_CHR(penc->owner_ref) ) return NULL;

    return ChrList.lst + penc->owner_ref;
}

//--------------------------------------------------------------------------------------------
Uint16  enc_get_ieve( Uint16 ienc )
{
    enc_t * penc;

    if( INVALID_ENC(ienc) ) return MAX_EVE;
    penc = EncList.lst + ienc;

    if( INVALID_EVE(penc->eve_ref) ) return MAX_EVE;

    return penc->eve_ref;
}

//--------------------------------------------------------------------------------------------
eve_t * enc_get_peve( Uint16 ienc )
{
    enc_t * penc;

    if( INVALID_ENC(ienc) ) return NULL;
    penc = EncList.lst + ienc;

    if( INVALID_EVE(penc->eve_ref) ) return NULL;

    return EveStack.lst + penc->eve_ref;
}

//--------------------------------------------------------------------------------------------
IDSZ enc_get_idszremove( Uint16 ienc )
{
    eve_t * peve = enc_get_peve( ienc );
    if( NULL == peve ) return IDSZ_NONE;

    return peve->removedbyidsz;
}

//--------------------------------------------------------------------------------------------
bool_t enc_is_removed( Uint16 ienc, Uint16 test_profile )
{
    IDSZ idsz_remove;

    if( INVALID_ENC(ienc) ) return bfalse;
    idsz_remove = enc_get_idszremove( ienc );

    // if nothing can remove it, just go on with your business
    if( IDSZ_NONE == idsz_remove ) return bfalse;

    // check vs. every IDSZ that could have something to do with cancelling the enchant
    if( idsz_remove == pro_get_idsz(test_profile, IDSZ_TYPE  ) ) return btrue;
    if( idsz_remove == pro_get_idsz(test_profile, IDSZ_PARENT) ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_all_eve()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        memset( EveStack.lst + cnt, 0, sizeof(eve_t) );
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

    if( !VALID_EVE_RANGE( ieve) ) return bfalse;
    peve = EveStack.lst + ieve;

    if(!peve->loaded) return btrue;

    memset( peve, 0, sizeof(eve_t) );

    return btrue;
}

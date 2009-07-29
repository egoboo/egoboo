// ********************************************************************************************
// *
// *    This file is part of Egoboo.
// *
// *    Egoboo is free software: you can redistribute it and/or modify it
// *    under the terms of the GNU General Public License as published by
// *    the Free Software Foundation, either version 3 of the License, or
// *    (at your option) any later version.
// *
// *    Egoboo is distributed in the hope that it will be useful, but
// *    WITHOUT ANY WARRANTY; without even the implied warranty of
// *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// *    General Public License for more details.
// *
// *    You should have received a copy of the GNU General Public License
// *    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
// *
// ********************************************************************************************

/* Egoboo - enchant.c handles enchantments attached to objects
 */
#include "enchant.h"

#include "char.h"
#include "sound.h"
#include "camera.h"
#include "mad.h"
#include "game.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static Uint16       numfreeenchant = 0;         // For allocating new ones
static Uint16       freeenchant[MAX_ENC];

eve_t EveList[MAXEVE];
enc_t EncList[MAX_ENC];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void free_one_enchant( Uint16 ienc )
{
    // Now get rid of it

    if ( VALID_ENC_RANGE(ienc) )
    {
        // enchant "destructor"
        // sets all boolean values to false, incluting the "on" flag
        memset( EncList + ienc, 0, sizeof(enc_t) );

        if ( numfreeenchant < MAX_ENC )
        {
            freeenchant[numfreeenchant] = ienc;
            numfreeenchant++;
        }
    }
};

//--------------------------------------------------------------------------------------------
bool_t remove_enchant( Uint16 ienc )
{
    // ZZ> This function removes a specific enchantment and adds it to the unused list

    Sint16 iwave;
    Uint16 itarget, ispawner, ieve;
    Uint16 overlay;
    Uint16 lastenchant, currentenchant;
    int add;
    enc_t * penc;

    if ( INVALID_ENC(ienc) ) return bfalse;
    penc = EncList + ienc;

    // Unsparkle the spellbook
    ispawner = penc->spawner;
    if ( VALID_CHR(ispawner) )
    {
        ChrList[ispawner].sparkle = NOSPARKLE;

        // Make the spawner unable to undo the enchantment
        if ( ChrList[ispawner].undoenchant == ienc )
        {
            ChrList[ispawner].undoenchant = MAX_ENC;
        }
    }

    // who is the target?
    itarget = penc->target;

    // Play the end sound
    ieve = penc->eve;
    if ( VALID_EVE( ieve ) )
    {
        iwave = EveList[ieve].endsoundindex;
        if ( iwave >= 0 && iwave < MAX_WAVE )
        {
            Uint16 ispawner = penc->spawner;
            if ( VALID_CHR(ispawner) )
            {
                Uint16 imodel = ChrList[ispawner].model;
                if ( VALID_CAP(imodel) )
                {
                    if ( VALID_CHR(itarget) )
                    {
                        sound_play_chunk(ChrList[itarget].pos_old, CapList[imodel].wavelist[iwave]);
                    }
                    else
                    {
                        sound_play_chunk( PCamera->track_pos, CapList[imodel].wavelist[iwave]);
                    }
                }
            }
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
    for ( add = 0; add < MAXEVEADDVALUE; add++ )
    {
        remove_enchant_value( ienc, add );
    }

    // Unlink it
    if ( VALID_CHR(itarget) )
    {
        if ( ChrList[itarget].firstenchant == ienc )
        {
            // It was the first in the list
            ChrList[itarget].firstenchant = penc->nextenchant;
        }
        else
        {
            // Search until we find it
            lastenchant = currentenchant = ChrList[itarget].firstenchant;

            while ( currentenchant != ienc )
            {
                lastenchant = currentenchant;
                currentenchant = EncList[currentenchant].nextenchant;
            }

            // Relink the last enchantment
            EncList[lastenchant].nextenchant = penc->nextenchant;
        }
    }

    // See if we spit out an end message
    if ( EveList[penc->eve].endmessage >= 0 )
    {
        display_message( NULL, MadList[penc->eve].msgstart + EveList[penc->eve].endmessage, penc->target );
    }

    // Check to see if we spawn a poof
    if ( EveList[penc->eve].poofonend )
    {
        spawn_poof( penc->target, penc->eve );
    }

    // Check to see if the character dies
    if ( EveList[penc->eve].killonend )
    {
        if ( VALID_CHR(itarget) )
        {
            if ( ChrList[itarget].invictus )  TeamList[ChrList[itarget].baseteam].morale++;

            ChrList[itarget].invictus = bfalse;
            kill_character( itarget, MAX_CHR );
        }
    }

    // Kill overlay too...
    overlay = penc->overlay;
    if ( VALID_CHR(overlay) )
    {
        if ( ChrList[overlay].invictus )  TeamList[ChrList[overlay].baseteam].morale++;

        ChrList[overlay].invictus = bfalse;
        kill_character( overlay, MAX_CHR );
    }

    // Remove see kurse enchant
    if ( VALID_CHR( itarget ) )
    {
        if ( EveList[penc->eve].seekurse && !CapList[ChrList[itarget].model].canseekurse )
        {
            ChrList[itarget].canseekurse = bfalse;
        }
    }

    // Now fix dem weapons
    if ( VALID_CHR( itarget ) )
    {
        reset_character_alpha( ChrList[itarget].holdingwhich[SLOT_LEFT] );
        reset_character_alpha( ChrList[itarget].holdingwhich[SLOT_RIGHT] );
    }

    free_one_enchant( ienc );

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 enchant_value_filled( Uint16 ienc, Uint8 valueindex )
{
    // ZZ> This function returns MAX_ENC if the enchantment's target has no conflicting
    //     set values in its other enchantments.  Otherwise it returns the ienc
    //     of the conflicting enchantment
    Uint16 character, currenchant;

    if ( ienc > MAX_ENC || !EncList[ienc].on ) return MAX_ENC;

    character = EncList[ienc].target;
    currenchant = ChrList[character].firstenchant;

    while ( currenchant != MAX_ENC )
    {
        if ( EncList[currenchant].setyesno[valueindex] )
        {
            break;
        }

        currenchant = EncList[currenchant].nextenchant;
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

    if ( ienc >= MAX_ENC || !EncList[ienc].on) return;
    penc = EncList + ienc;

    if ( ieve >= MAXEVE || !EveList[ieve].loaded ) return;
    peve = EveList + ieve;

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
            character = penc->target;
            if ( VALID_CHR(penc->target) )
            {
                ptarget = ChrList + character;
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
                        ptarget->missilehandler = penc->owner;
                        break;

                    case SETMORPH:
                        penc->setsave[valueindex] = ptarget->skin;
                        // Special handler for morph
                        change_character( character, ieve, 0, LEAVEALL ); // LEAVEFIRST);
                        ptarget->ai.alert |= ALERTIF_CHANGED;
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

    if ( ienc >= MAX_ENC || !EncList[ienc].on) return;
    penc = EncList + ienc;

    if ( ieve >= MAXEVE || !EveList[ieve].loaded ) return;
    peve = EveList + ieve;

    character = penc->target;
    if ( INVALID_CHR(penc->target) ) return;
    ptarget = ChrList + character;

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
            fnewvalue = ptarget->sizegoto;
            fvaluetoadd = peve->addvalue[valueindex] / 128.0f;
            fgetadd( 0.5f, fnewvalue, 2.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            ptarget->sizegoto += fvaluetoadd;
            ptarget->sizegototime = SIZETIME;
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
Uint16 spawn_enchant( Uint16 owner, Uint16 target, Uint16 spawner, Uint16 ienc, Uint16 modeloptional )
{
    // ZZ> This function enchants a target, returning the enchantment index or MAX_ENC
    //     if failed
    Uint16 ieve, overlay;
    int add;
    eve_t * peve;

    // Target must both be alive and on and valid
    if ( INVALID_CHR(target) || !ChrList[target].alive )
        return MAX_ENC;

    if ( modeloptional < MAX_PROFILE )
    {
        // The enchantment type is given explicitly
        ieve = modeloptional;
    }
    else
    {
        // The enchantment type is given by the spawner
        ieve = ChrList[spawner].model;
    }

    // Owner must both be alive and on and valid if it isn't a stayifnoowner enchant
    if ( !EveList[ieve].stayifnoowner && ( INVALID_CHR(owner) || !ChrList[target].alive) )
        return MAX_ENC;

    if ( ieve >= MAXEVE || !EveList[ieve].loaded ) return MAX_PROFILE;
    peve = EveList + ieve;

    if ( ienc == MAX_ENC )
    {
        // Should it choose an inhand item?
        if ( peve->retarget )
        {
            // Is at least one valid?
            if ( ChrList[target].holdingwhich[SLOT_LEFT] == MAX_CHR && ChrList[target].holdingwhich[SLOT_RIGHT] == MAX_CHR )
            {
                // No weapons to pick
                return MAX_ENC;
            }

            // Left, right, or both are valid
            if ( ChrList[target].holdingwhich[SLOT_LEFT] == MAX_CHR )
            {
                // Only right hand is valid
                target = ChrList[target].holdingwhich[SLOT_RIGHT];
            }
            else
            {
                // Pick left hand
                target = ChrList[target].holdingwhich[SLOT_LEFT];
            }
        }

        if ( INVALID_CHR(target) || !ChrList[target].alive ) return MAX_ENC;

        // Make sure it's valid
        if ( peve->dontdamagetype != DAMAGE_NONE )
        {
            if ( ( ChrList[target].damagemodifier[peve->dontdamagetype]&DAMAGESHIFT ) >= 3
                    || ChrList[target].damagemodifier[peve->dontdamagetype]&DAMAGECHARGE )
            {
                return MAX_ENC;
            }
        }
        if ( peve->onlydamagetype != DAMAGE_NONE )
        {
            if ( ChrList[target].damagetargettype != peve->onlydamagetype )
            {
                return MAX_ENC;
            }
        }

        // Find one to use
        ienc = get_free_enchant();
    }
    else
    {
        numfreeenchant--;  // To keep it in order
    }

    if ( ienc < MAX_ENC )
    {
        enc_t * penc = EncList + ienc;
        chr_t * ptarget = ChrList + target;

        memset( penc, 0, sizeof(enc_t) );

        // Make a new one
        penc->on      = btrue;
        penc->target  = VALID_CHR(target)  ? target  : MAX_CHR;
        penc->owner   = VALID_CHR(owner)   ? owner   : MAX_CHR;
        penc->spawner = VALID_CHR(spawner) ? spawner : MAX_CHR;

        if ( VALID_CHR(spawner) )
        {
            ChrList[spawner].undoenchant = ienc;
        }

        penc->eve = ieve;
        penc->time = peve->time;
        penc->spawntime = 1;
        penc->ownermana = peve->ownermana;
        penc->ownerlife = peve->ownerlife;
        penc->targetmana = peve->targetmana;
        penc->targetlife = peve->targetlife;

        // Add it as first in the list
        penc->nextenchant = ptarget->firstenchant;
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
        add = 0;
        while ( add < MAXEVEADDVALUE )
        {
            add_enchant_value( ienc, add, ieve );
            add++;
        }

        // Create an overlay character?
        penc->overlay = MAX_CHR;
        if ( peve->overlay )
        {
            overlay = spawn_one_character( ptarget->pos, ieve, ptarget->team, 0, ptarget->turn_z, NULL, MAX_CHR );

            if ( VALID_CHR(overlay) )
            {
                chr_t * povl = ChrList + overlay;

                penc->overlay = overlay;  // Kill this character on end...
                povl->ai.target = target;
                povl->ai.state = peve->overlay;
                povl->overlay = btrue;

                // Start out with ActionMJ...  Object activated
                if ( MadList[povl->inst.imad].actionvalid[ACTION_MJ] )
                {
                    povl->action = ACTION_MJ;
                    povl->inst.lip = 0;
                    povl->inst.frame_nxt = MadList[povl->inst.imad].actionstart[ACTION_MJ];
                    povl->inst.frame_lst = povl->inst.frame_nxt;
                    povl->actionready = bfalse;
                }

                povl->inst.light = 254;  // Assume it's transparent...
            }
        }

        // Allow them to see kurses?
        if (peve->seekurse) ChrList[target].canseekurse = btrue;
    }

    return ienc;
}

//--------------------------------------------------------------------------------------------
void disenchant_character( Uint16 cnt )
{
    // ZZ> This function removes all enchantments from a character
    while ( ChrList[cnt].firstenchant != MAX_ENC )
    {
        remove_enchant( ChrList[cnt].firstenchant );
    }
}

//--------------------------------------------------------------------------------------------
void free_all_enchants()
{
    // ZZ> This functions frees all of the enchantments

    int cnt;

    numfreeenchant = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++)
    {
        // reuse this code
        free_one_enchant( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t load_one_enchant_profile( const char* szLoadName, Uint16 profile )
{
    // ZZ> This function loads the enchantment associated with an object
    FILE* fileread;
    char cTmp;
    int iTmp, tTmp;
    IDSZ idsz;
    float fTmp;
    int num;
    eve_t * peve;

    if ( profile > MAXEVE ) return bfalse;
    peve = EveList + profile;

    memset( peve, 0, sizeof(eve_t) );

    parse_filename = "";
    fileread = fopen( szLoadName, "r" );
    if ( NULL == fileread )
    {
        return bfalse;
    }
    parse_filename = szLoadName;

    // btrue/bfalse values
    cTmp = fget_next_char( fileread );
    peve->retarget = bfalse;
    if ( 'T' == toupper(cTmp) )  peve->retarget = btrue;

    cTmp = fget_next_char( fileread );
    peve->override = bfalse;
    if ( 'T' == toupper(cTmp) )  peve->override = btrue;

    cTmp = fget_next_char( fileread );
    peve->removeoverridden = bfalse;
    if ( 'T' == toupper(cTmp) )  peve->removeoverridden = btrue;

    cTmp = fget_next_char( fileread );
    peve->killonend = bfalse;
    if ( 'T' == toupper(cTmp) )  peve->killonend = btrue;

    cTmp = fget_next_char( fileread );
    peve->poofonend = bfalse;
    if ( 'T' == toupper(cTmp) )  peve->poofonend = btrue;

    // More stuff
    iTmp = fget_next_int( fileread );  peve->time = iTmp;
    iTmp = fget_next_int( fileread );  peve->endmessage = iTmp;

    // Drain stuff
    fTmp = fget_next_float( fileread );  peve->ownermana = fTmp * 256;
    fTmp = fget_next_float( fileread );  peve->targetmana = fTmp * 256;
    cTmp = fget_next_char( fileread );
    peve->endifcantpay = bfalse;
    if ( 'T' == toupper(cTmp) )  peve->endifcantpay = btrue;

    fTmp = fget_next_float( fileread );  peve->ownerlife = fTmp * 256;
    fTmp = fget_next_float( fileread );  peve->targetlife = fTmp * 256;

    // Specifics
    cTmp = fget_next_char( fileread );
    peve->dontdamagetype = DAMAGE_NONE;
    if ( 'S' == toupper(cTmp) )  peve->dontdamagetype = DAMAGE_SLASH;
    if ( 'C' == toupper(cTmp) )  peve->dontdamagetype = DAMAGE_CRUSH;
    if ( 'P' == toupper(cTmp) )  peve->dontdamagetype = DAMAGE_POKE;
    if ( 'H' == toupper(cTmp) )  peve->dontdamagetype = DAMAGE_HOLY;
    if ( 'E' == toupper(cTmp) )  peve->dontdamagetype = DAMAGE_EVIL;
    if ( 'F' == toupper(cTmp) )  peve->dontdamagetype = DAMAGE_FIRE;
    if ( 'I' == toupper(cTmp) )  peve->dontdamagetype = DAMAGE_ICE;
    if ( 'Z' == toupper(cTmp) )  peve->dontdamagetype = DAMAGE_ZAP;

    cTmp = fget_next_char( fileread );
    peve->onlydamagetype = DAMAGE_NONE;
    if ( 'S' == toupper(cTmp) )  peve->onlydamagetype = DAMAGE_SLASH;
    if ( 'C' == toupper(cTmp) )  peve->onlydamagetype = DAMAGE_CRUSH;
    if ( 'P' == toupper(cTmp) )  peve->onlydamagetype = DAMAGE_POKE;
    if ( 'H' == toupper(cTmp) )  peve->onlydamagetype = DAMAGE_HOLY;
    if ( 'E' == toupper(cTmp) )  peve->onlydamagetype = DAMAGE_EVIL;
    if ( 'F' == toupper(cTmp) )  peve->onlydamagetype = DAMAGE_FIRE;
    if ( 'I' == toupper(cTmp) )  peve->onlydamagetype = DAMAGE_ICE;
    if ( 'Z' == toupper(cTmp) )  peve->onlydamagetype = DAMAGE_ZAP;

    peve->removedbyidsz = fget_next_idsz( fileread );

    // Now the set values
    num = 0;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );
    peve->setvalue[num] = DAMAGE_SLASH;
    if ( 'C' == toupper(cTmp) )  peve->setvalue[num] = DAMAGE_CRUSH;
    if ( 'P' == toupper(cTmp) )  peve->setvalue[num] = DAMAGE_POKE;
    if ( 'H' == toupper(cTmp) )  peve->setvalue[num] = DAMAGE_HOLY;
    if ( 'E' == toupper(cTmp) )  peve->setvalue[num] = DAMAGE_EVIL;
    if ( 'F' == toupper(cTmp) )  peve->setvalue[num] = DAMAGE_FIRE;
    if ( 'I' == toupper(cTmp) )  peve->setvalue[num] = DAMAGE_ICE;
    if ( 'Z' == toupper(cTmp) )  peve->setvalue[num] = DAMAGE_ZAP;

    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    iTmp = fget_int( fileread );  peve->setvalue[num] = iTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    iTmp = fget_int( fileread );  peve->setvalue[num] = iTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    iTmp = fget_int( fileread );  peve->setvalue[num] = iTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( 'T' == toupper(cTmp) ) iTmp = DAMAGEINVERT;
    if ( 'C' == toupper(cTmp) ) iTmp = DAMAGECHARGE;
    if ( 'M' == toupper(cTmp) ) iTmp = DAMAGEMANA;

    tTmp = fget_int( fileread );  peve->setvalue[num] = iTmp | tTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( 'T' == toupper(cTmp) ) iTmp = DAMAGEINVERT;
    if ( 'C' == toupper(cTmp) ) iTmp = DAMAGECHARGE;
    if ( 'M' == toupper(cTmp) ) iTmp = DAMAGEMANA;

    tTmp = fget_int( fileread );  peve->setvalue[num] = iTmp | tTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( 'T' == toupper(cTmp) ) iTmp = DAMAGEINVERT;
    if ( 'C' == toupper(cTmp) ) iTmp = DAMAGECHARGE;
    if ( 'M' == toupper(cTmp) ) iTmp = DAMAGEMANA;

    tTmp = fget_int( fileread );  peve->setvalue[num] = iTmp | tTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( 'T' == toupper(cTmp) ) iTmp = DAMAGEINVERT;
    if ( 'C' == toupper(cTmp) ) iTmp = DAMAGECHARGE;
    if ( 'M' == toupper(cTmp) ) iTmp = DAMAGEMANA;

    tTmp = fget_int( fileread );  peve->setvalue[num] = iTmp | tTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( 'T' == toupper(cTmp) ) iTmp = DAMAGEINVERT;
    if ( 'C' == toupper(cTmp) ) iTmp = DAMAGECHARGE;
    if ( 'M' == toupper(cTmp) ) iTmp = DAMAGEMANA;

    tTmp = fget_int( fileread );  peve->setvalue[num] = iTmp | tTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( 'T' == toupper(cTmp) ) iTmp = DAMAGEINVERT;
    if ( 'C' == toupper(cTmp) ) iTmp = DAMAGECHARGE;
    if ( 'M' == toupper(cTmp) ) iTmp = DAMAGEMANA;

    tTmp = fget_int( fileread );  peve->setvalue[num] = iTmp | tTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( 'T' == toupper(cTmp) ) iTmp = DAMAGEINVERT;
    if ( 'C' == toupper(cTmp) ) iTmp = DAMAGECHARGE;
    if ( 'M' == toupper(cTmp) ) iTmp = DAMAGEMANA;

    tTmp = fget_int( fileread );  peve->setvalue[num] = iTmp | tTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( 'T' == toupper(cTmp) ) iTmp = DAMAGEINVERT;
    if ( 'C' == toupper(cTmp) ) iTmp = DAMAGECHARGE;
    if ( 'M' == toupper(cTmp) ) iTmp = DAMAGEMANA;

    tTmp = fget_int( fileread );  peve->setvalue[num] = iTmp | tTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    iTmp = fget_int( fileread );  peve->setvalue[num] = iTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    iTmp = fget_int( fileread );  peve->setvalue[num] = iTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    iTmp = fget_int( fileread );  peve->setvalue[num] = iTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    iTmp = fget_int( fileread );  peve->setvalue[num] = iTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    iTmp = fget_int( fileread );  peve->setvalue[num] = iTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );
    peve->setvalue[num] = ( 'T' == toupper(cTmp) );
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );
    peve->setvalue[num] = ( 'T' == toupper(cTmp) );
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    cTmp = fget_first_letter( fileread );
    peve->setvalue[num] = MISNORMAL;
    if ( 'R' == toupper(cTmp) )  peve->setvalue[num] = MISREFLECT;
    if ( 'D' == toupper(cTmp) )  peve->setvalue[num] = MISDEFLECT;

    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    fTmp = fget_float( fileread );  fTmp = fTmp * 16;
    peve->setvalue[num] = (Uint8) fTmp;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    peve->setvalue[num] = btrue;
    num++;
    cTmp = fget_next_char( fileread );
    peve->setyesno[num] = ( 'T' == toupper(cTmp) );
    peve->setvalue[num] = btrue;
    num++;

    // Now read in the add values
    num = 0;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 16;
    num++;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 127;
    num++;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 127;
    num++;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 4;
    num++;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 127;
    num++;
    iTmp = fget_next_int( fileread );
    peve->addvalue[num] = iTmp;
    num++;
    iTmp = fget_next_int( fileread );
    peve->addvalue[num] = iTmp;
    num++;
    iTmp = fget_next_int( fileread );
    peve->addvalue[num] = iTmp;
    num++;
    iTmp = fget_next_int( fileread );
    peve->addvalue[num] = iTmp;
    num++;
    iTmp = fget_next_int( fileread );  // Defense is backwards
    peve->addvalue[num] = -iTmp;
    num++;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 4;
    num++;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 4;
    num++;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 4;
    num++;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 4;
    num++;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 4;
    num++;
    fTmp = fget_next_float( fileread );
    peve->addvalue[num] = (Sint32) fTmp * 4;
    num++;

    // Clear expansions...
    peve->contspawntime = 0;
    peve->contspawnamount = 0;
    peve->contspawnfacingadd = 0;
    peve->contspawnpip = 0;
    peve->endsoundindex = INVALID_SOUND;
    peve->stayifnoowner = 0;
    peve->overlay = 0;
    peve->seekurse = bfalse;

    // Read expansions
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        idsz = fget_idsz( fileread );

        if ( idsz == MAKE_IDSZ( 'A', 'M', 'O', 'U' ) )  peve->contspawnamount = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'T', 'Y', 'P', 'E' ) )  peve->contspawnpip = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'T', 'I', 'M', 'E' ) )  peve->contspawntime = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'F', 'A', 'C', 'E' ) )  peve->contspawnfacingadd = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'S', 'E', 'N', 'D' ) )
        {
            // This is wrong, it gets stored or loaded incorrectly (Loaded in game.c)
            int itmp = fget_int( fileread );
            peve->endsoundindex = CLIP(itmp, INVALID_SOUND, MAX_WAVE);
        }
        else if ( idsz == MAKE_IDSZ( 'S', 'T', 'A', 'Y' ) ) peve->stayifnoowner = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'O', 'V', 'E', 'R' ) ) peve->overlay = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'C', 'K', 'U', 'R' ) ) peve->seekurse = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'D', 'E', 'A', 'D' ) ) peve->stayifdead = fget_int( fileread );
    }

    // All done ( finally )
    fclose( fileread );
    parse_filename = "";

    peve->loaded = btrue;

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 get_free_enchant()
{
    // ZZ> This function returns the next free enchantment or MAX_ENC if there are none
    if ( numfreeenchant > 0 )
    {
        numfreeenchant--;
        return freeenchant[numfreeenchant];
    }

    return MAX_ENC;
}

//--------------------------------------------------------------------------------------------
void unset_enchant_value( Uint16 ienc, Uint8 valueindex )
{
    // ZZ> This function unsets a set value
    Uint16 character;
    enc_t * penc;
    chr_t * ptarget;

    if ( ienc >= MAX_ENC || !EncList[ienc].on) return;
    penc = EncList + ienc;

    character = penc->target;
    if ( INVALID_CHR(penc->target) ) return;
    ptarget = ChrList + character;

    if ( penc->setyesno[valueindex] )
    {
        character = penc->target;

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

    if ( ienc >= MAX_ENC || !EncList[ienc].on) return;
    penc = EncList + ienc;

    character = penc->target;
    if ( INVALID_CHR(penc->target) ) return;
    ptarget = ChrList + character;

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
            ptarget->sizegoto -= fvaluetoadd;
            ptarget->sizegototime = SIZETIME;
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

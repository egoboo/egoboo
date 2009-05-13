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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - enchant.c handles enchantments attached to objects
 */
#include "enchant.h"

#include "char.h"
#include "sound.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static Uint16       numfreeenchant = 0;         // For allocating new ones
static Uint16       freeenchant[MAXENCHANT];

eve_t EveList[MAXEVE];
enc_t EncList[MAXENCHANT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t remove_enchant( Uint16 enchantindex )
{
    // ZZ> This function removes a specific enchantment and adds it to the unused list
    Uint16 character, overlay;
    Uint16 lastenchant, currentenchant;
    int add;

    if ( enchantindex >= MAXENCHANT ) return bfalse;
    if ( !EncList[enchantindex].on ) return bfalse;

    // Unsparkle the spellbook
    character = EncList[enchantindex].spawner;
    if ( character < MAXCHR )
    {
        ChrList[character].sparkle = NOSPARKLE;

        // Make the spawner unable to undo the enchantment
        if ( ChrList[character].undoenchant == enchantindex )
        {
            ChrList[character].undoenchant = MAXENCHANT;
        }
    }

    // Play the end sound
    character = EncList[enchantindex].target;
    if ( EveList[enchantindex].waveindex >= 0 && EveList[enchantindex].waveindex < MAXWAVE )
    {
        Sint16 iwave    = EveList[enchantindex].waveindex;
        Uint16 ispawner = EncList[enchantindex].spawner;
        if ( MAXCHR != ispawner && iwave >= 0 && iwave < MAXWAVE )
        {
            sound_play_chunk(ChrList[character].oldx, ChrList[character].oldy, CapList[ChrList[ispawner].model].wavelist[iwave]);
        }
    }

    // Unset enchant values, doing morph last
    unset_enchant_value( enchantindex, SETDAMAGETYPE );
    unset_enchant_value( enchantindex, SETNUMBEROFJUMPS );
    unset_enchant_value( enchantindex, SETLIFEBARCOLOR );
    unset_enchant_value( enchantindex, SETMANABARCOLOR );
    unset_enchant_value( enchantindex, SETSLASHMODIFIER );
    unset_enchant_value( enchantindex, SETCRUSHMODIFIER );
    unset_enchant_value( enchantindex, SETPOKEMODIFIER );
    unset_enchant_value( enchantindex, SETHOLYMODIFIER );
    unset_enchant_value( enchantindex, SETEVILMODIFIER );
    unset_enchant_value( enchantindex, SETFIREMODIFIER );
    unset_enchant_value( enchantindex, SETICEMODIFIER );
    unset_enchant_value( enchantindex, SETZAPMODIFIER );
    unset_enchant_value( enchantindex, SETFLASHINGAND );
    unset_enchant_value( enchantindex, SETLIGHTBLEND );
    unset_enchant_value( enchantindex, SETALPHABLEND );
    unset_enchant_value( enchantindex, SETSHEEN );
    unset_enchant_value( enchantindex, SETFLYTOHEIGHT );
    unset_enchant_value( enchantindex, SETWALKONWATER );
    unset_enchant_value( enchantindex, SETCANSEEINVISIBLE );
    unset_enchant_value( enchantindex, SETMISSILETREATMENT );
    unset_enchant_value( enchantindex, SETCOSTFOREACHMISSILE );
    unset_enchant_value( enchantindex, SETCHANNEL );
    unset_enchant_value( enchantindex, SETMORPH );

    // Remove all of the cumulative values
    add = 0;

    while ( add < MAXEVEADDVALUE )
    {
        remove_enchant_value( enchantindex, add );
        add++;
    }

    // Unlink it
    if ( ChrList[character].firstenchant == enchantindex )
    {
        // It was the first in the list
        ChrList[character].firstenchant = EncList[enchantindex].nextenchant;
    }
    else
    {
        // Search until we find it
        lastenchant = currentenchant = ChrList[character].firstenchant;

        while ( currentenchant != enchantindex )
        {
            lastenchant = currentenchant;
            currentenchant = EncList[currentenchant].nextenchant;
        }

        // Relink the last enchantment
        EncList[lastenchant].nextenchant = EncList[enchantindex].nextenchant;
    }

    // See if we spit out an end message
    if ( EveList[EncList[enchantindex].eve].endmessage >= 0 )
    {
        display_message( NULL, MadList[EncList[enchantindex].eve].msgstart + EveList[EncList[enchantindex].eve].endmessage, EncList[enchantindex].target );
    }

    // Check to see if we spawn a poof
    if ( EveList[EncList[enchantindex].eve].poofonend )
    {
        spawn_poof( EncList[enchantindex].target, EncList[enchantindex].eve );
    }

    // Check to see if the character dies
    if ( EveList[EncList[enchantindex].eve].killonend )
    {
        if ( ChrList[character].invictus )  TeamList[ChrList[character].baseteam].morale++;

        ChrList[character].invictus = bfalse;
        kill_character( character, MAXCHR );
    }

    // Kill overlay too...
    overlay = EncList[enchantindex].overlay;
    if ( overlay < MAXCHR )
    {
        if ( ChrList[overlay].invictus )  TeamList[ChrList[overlay].baseteam].morale++;

        ChrList[overlay].invictus = bfalse;
        kill_character( overlay, MAXCHR );
    }

    // Remove see kurse enchant
    if ( EveList[EncList[enchantindex].eve].seekurse && !CapList[ChrList[character].model].canseekurse )
    {
        ChrList[character].canseekurse = bfalse;
    }

    // Now get rid of it
    EncList[enchantindex].on = bfalse;
    freeenchant[numfreeenchant] = enchantindex;
    numfreeenchant++;

    // Now fix dem weapons
    reset_character_alpha( ChrList[character].holdingwhich[SLOT_LEFT] );
    reset_character_alpha( ChrList[character].holdingwhich[SLOT_RIGHT] );

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 enchant_value_filled( Uint16 enchantindex, Uint8 valueindex )
{
    // ZZ> This function returns MAXENCHANT if the enchantment's target has no conflicting
    //     set values in its other enchantments.  Otherwise it returns the enchantindex
    //     of the conflicting enchantment
    Uint16 character, currenchant;

    character = EncList[enchantindex].target;
    currenchant = ChrList[character].firstenchant;

    while ( currenchant != MAXENCHANT )
    {
        if ( EncList[currenchant].setyesno[valueindex] )
        {
            return currenchant;
        }

        currenchant = EncList[currenchant].nextenchant;
    }

    return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void set_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype )
{
    // ZZ> This function sets and saves one of the character's stats
    Uint16 conflict, character;

    EncList[enchantindex].setyesno[valueindex] = bfalse;
    if ( EveList[enchanttype].setyesno[valueindex] )
    {
        conflict = enchant_value_filled( enchantindex, valueindex );
        if ( conflict == MAXENCHANT || EveList[enchanttype].override )
        {
            // Check for multiple enchantments
            if ( conflict < MAXENCHANT )
            {
                // Multiple enchantments aren't allowed for sets
                if ( EveList[enchanttype].removeoverridden )
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
            character = EncList[enchantindex].target;
            EncList[enchantindex].setyesno[valueindex] = btrue;

            switch ( valueindex )
            {
                case SETDAMAGETYPE:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].damagetargettype;
                    ChrList[character].damagetargettype = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETNUMBEROFJUMPS:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].jumpnumberreset;
                    ChrList[character].jumpnumberreset = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETLIFEBARCOLOR:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].lifecolor;
                    ChrList[character].lifecolor = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETMANABARCOLOR:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].manacolor;
                    ChrList[character].manacolor = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETSLASHMODIFIER:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGE_SLASH];
                    ChrList[character].damagemodifier[DAMAGE_SLASH] = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETCRUSHMODIFIER:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGE_CRUSH];
                    ChrList[character].damagemodifier[DAMAGE_CRUSH] = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETPOKEMODIFIER:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGE_POKE];
                    ChrList[character].damagemodifier[DAMAGE_POKE] = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETHOLYMODIFIER:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGE_HOLY];
                    ChrList[character].damagemodifier[DAMAGE_HOLY] = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETEVILMODIFIER:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGE_EVIL];
                    ChrList[character].damagemodifier[DAMAGE_EVIL] = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETFIREMODIFIER:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGE_FIRE];
                    ChrList[character].damagemodifier[DAMAGE_FIRE] = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETICEMODIFIER:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGE_ICE];
                    ChrList[character].damagemodifier[DAMAGE_ICE] = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETZAPMODIFIER:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGE_ZAP];
                    ChrList[character].damagemodifier[DAMAGE_ZAP] = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETFLASHINGAND:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].flashand;
                    ChrList[character].flashand = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETLIGHTBLEND:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].inst.light;
                    ChrList[character].inst.light = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETALPHABLEND:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].inst.alpha;
                    ChrList[character].inst.alpha = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETSHEEN:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].inst.sheen;
                    ChrList[character].inst.sheen = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETFLYTOHEIGHT:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].flyheight;
                    if ( ChrList[character].flyheight == 0 && ChrList[character].zpos > -2 )
                    {
                        ChrList[character].flyheight = EveList[enchanttype].setvalue[valueindex];
                    }
                    break;
                case SETWALKONWATER:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].waterwalk;
                    if ( !ChrList[character].waterwalk )
                    {
                        ChrList[character].waterwalk = EveList[enchanttype].setvalue[valueindex];
                    }
                    break;
                case SETCANSEEINVISIBLE:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].canseeinvisible;
                    ChrList[character].canseeinvisible = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETMISSILETREATMENT:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].missiletreatment;
                    ChrList[character].missiletreatment = EveList[enchanttype].setvalue[valueindex];
                    break;
                case SETCOSTFOREACHMISSILE:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].missilecost;
                    ChrList[character].missilecost = EveList[enchanttype].setvalue[valueindex];
                    ChrList[character].missilehandler = EncList[enchantindex].owner;
                    break;
                case SETMORPH:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].skin;
                    // Special handler for morph
                    change_character( character, enchanttype, 0, LEAVEALL ); // LEAVEFIRST);
                    ChrList[character].ai.alert |= ALERTIF_CHANGED;
                    break;
                case SETCHANNEL:
                    EncList[enchantindex].setsave[valueindex] = ChrList[character].canchannel;
                    ChrList[character].canchannel = EveList[enchanttype].setvalue[valueindex];
                    break;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void add_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype )
{
    // ZZ> This function does cumulative modification to character stats
    int valuetoadd, newvalue;
    float fvaluetoadd, fnewvalue;
    Uint16 character;

    character = EncList[enchantindex].target;

    valuetoadd = 0;

    switch ( valueindex )
    {
        case ADDJUMPPOWER:
            fnewvalue = ChrList[character].jump;
            fvaluetoadd = EveList[enchanttype].addvalue[valueindex] / 16.0f;
            fgetadd( 0, fnewvalue, 30.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 16.0f; // Get save value
            fvaluetoadd = valuetoadd / 16.0f;
            ChrList[character].jump += fvaluetoadd;
            break;
        case ADDBUMPDAMPEN:
            fnewvalue = ChrList[character].bumpdampen;
            fvaluetoadd = EveList[enchanttype].addvalue[valueindex] / 128.0f;
            fgetadd( 0, fnewvalue, 1.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            ChrList[character].bumpdampen += fvaluetoadd;
            break;
        case ADDBOUNCINESS:
            fnewvalue = ChrList[character].dampen;
            fvaluetoadd = EveList[enchanttype].addvalue[valueindex] / 128.0f;
            fgetadd( 0, fnewvalue, 0.95f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            ChrList[character].dampen += fvaluetoadd;
            break;
        case ADDDAMAGE:
            newvalue = ChrList[character].damageboost;
            valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
            getadd( 0, newvalue, 4096, &valuetoadd );
            ChrList[character].damageboost += valuetoadd;
            break;
        case ADDSIZE:
            fnewvalue = ChrList[character].sizegoto;
            fvaluetoadd = EveList[enchanttype].addvalue[valueindex] / 128.0f;
            fgetadd( 0.5f, fnewvalue, 2.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            ChrList[character].sizegoto += fvaluetoadd;
            ChrList[character].sizegototime = SIZETIME;
            break;
        case ADDACCEL:
            fnewvalue = ChrList[character].maxaccel;
            fvaluetoadd = EveList[enchanttype].addvalue[valueindex] / 80.0f;
            fgetadd( 0, fnewvalue, 1.5f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 1000.0f; // Get save value
            fvaluetoadd = valuetoadd / 1000.0f;
            ChrList[character].maxaccel += fvaluetoadd;
            break;
        case ADDRED:
            newvalue = ChrList[character].inst.redshift;
            valuetoadd = EveList[enchanttype].addvalue[valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            ChrList[character].inst.redshift += valuetoadd;
            break;
        case ADDGRN:
            newvalue = ChrList[character].inst.grnshift;
            valuetoadd = EveList[enchanttype].addvalue[valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            ChrList[character].inst.grnshift += valuetoadd;
            break;
        case ADDBLU:
            newvalue = ChrList[character].inst.blushift;
            valuetoadd = EveList[enchanttype].addvalue[valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            ChrList[character].inst.blushift += valuetoadd;
            break;
        case ADDDEFENSE:
            newvalue = ChrList[character].defense;
            valuetoadd = EveList[enchanttype].addvalue[valueindex];
            getadd( 55, newvalue, 255, &valuetoadd );  // Don't fix again!
            ChrList[character].defense += valuetoadd;
            break;
        case ADDMANA:
            newvalue = ChrList[character].manamax;
            valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
            getadd( 0, newvalue, PERFECTBIG, &valuetoadd );
            ChrList[character].manamax += valuetoadd;
            ChrList[character].mana += valuetoadd;
            if ( ChrList[character].mana < 0 )  ChrList[character].mana = 0;

            break;
        case ADDLIFE:
            newvalue = ChrList[character].lifemax;
            valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
            getadd( LOWSTAT, newvalue, PERFECTBIG, &valuetoadd );
            ChrList[character].lifemax += valuetoadd;
            ChrList[character].life += valuetoadd;
            if ( ChrList[character].life < 1 )  ChrList[character].life = 1;

            break;
        case ADDSTRENGTH:
            newvalue = ChrList[character].strength;
            valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ChrList[character].strength += valuetoadd;
            break;
        case ADDWISDOM:
            newvalue = ChrList[character].wisdom;
            valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ChrList[character].wisdom += valuetoadd;
            break;
        case ADDINTELLIGENCE:
            newvalue = ChrList[character].intelligence;
            valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ChrList[character].intelligence += valuetoadd;
            break;
        case ADDDEXTERITY:
            newvalue = ChrList[character].dexterity;
            valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ChrList[character].dexterity += valuetoadd;
            break;
    }

    EncList[enchantindex].addsave[valueindex] = valuetoadd;  // Save the value for undo
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_enchant( Uint16 owner, Uint16 target, Uint16 spawner, Uint16 enchantindex, Uint16 modeloptional )
{
    // ZZ> This function enchants a target, returning the enchantment index or MAXENCHANT
    //     if failed
    Uint16 enchanttype, overlay;
    int add;
    if ( modeloptional < MAXMODEL )
    {
        // The enchantment type is given explicitly
        enchanttype = modeloptional;
    }
    else
    {
        // The enchantment type is given by the spawner
        enchanttype = ChrList[spawner].model;
    }

    // Target and owner must both be alive and on and valid
    if ( !ChrList[target].on || !ChrList[target].alive || target >= MAXCHR )
        return MAXENCHANT;
    if ( !ChrList[owner].on || !ChrList[owner].alive || owner >= MAXCHR )
        return MAXENCHANT;


    if ( EveList[enchanttype].valid )
    {
        if ( enchantindex == MAXENCHANT )
        {
            // Should it choose an inhand item?
            if ( EveList[enchanttype].retarget )
            {
                // Is at least one valid?
                if ( ChrList[target].holdingwhich[SLOT_LEFT] == MAXCHR && ChrList[target].holdingwhich[SLOT_RIGHT] == MAXCHR )
                {
                    // No weapons to pick
                    return MAXENCHANT;
                }

                // Left, right, or both are valid
                if ( ChrList[target].holdingwhich[SLOT_LEFT] == MAXCHR )
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

            // Make sure it's valid
            if ( EveList[enchanttype].dontdamagetype != DAMAGENULL )
            {
                if ( ( ChrList[target].damagemodifier[EveList[enchanttype].dontdamagetype]&7 ) >= 3 )  // Invert | Shift = 7
                {
                    return MAXENCHANT;
                }
            }
            if ( EveList[enchanttype].onlydamagetype != DAMAGENULL )
            {
                if ( ChrList[target].damagetargettype != EveList[enchanttype].onlydamagetype )
                {
                    return MAXENCHANT;
                }
            }

            // Find one to use
            enchantindex = get_free_enchant();
        }
        else
        {
            numfreeenchant--;  // To keep it in order
        }
        if ( enchantindex < MAXENCHANT )
        {
            // Make a new one
            EncList[enchantindex].on      = btrue;
            EncList[enchantindex].target  = target;
            EncList[enchantindex].owner   = owner;
            EncList[enchantindex].spawner = spawner;
            if ( spawner < MAXCHR )
            {
                ChrList[spawner].undoenchant = enchantindex;
            }

            EncList[enchantindex].eve = enchanttype;
            EncList[enchantindex].time = EveList[enchanttype].time;
            EncList[enchantindex].spawntime = 1;
            EncList[enchantindex].ownermana = EveList[enchanttype].ownermana;
            EncList[enchantindex].ownerlife = EveList[enchanttype].ownerlife;
            EncList[enchantindex].targetmana = EveList[enchanttype].targetmana;
            EncList[enchantindex].targetlife = EveList[enchanttype].targetlife;

            // Add it as first in the list
            EncList[enchantindex].nextenchant = ChrList[target].firstenchant;
            ChrList[target].firstenchant = enchantindex;

            // Now set all of the specific values, morph first
            set_enchant_value( enchantindex, SETMORPH, enchanttype );
            set_enchant_value( enchantindex, SETDAMAGETYPE, enchanttype );
            set_enchant_value( enchantindex, SETNUMBEROFJUMPS, enchanttype );
            set_enchant_value( enchantindex, SETLIFEBARCOLOR, enchanttype );
            set_enchant_value( enchantindex, SETMANABARCOLOR, enchanttype );
            set_enchant_value( enchantindex, SETSLASHMODIFIER, enchanttype );
            set_enchant_value( enchantindex, SETCRUSHMODIFIER, enchanttype );
            set_enchant_value( enchantindex, SETPOKEMODIFIER, enchanttype );
            set_enchant_value( enchantindex, SETHOLYMODIFIER, enchanttype );
            set_enchant_value( enchantindex, SETEVILMODIFIER, enchanttype );
            set_enchant_value( enchantindex, SETFIREMODIFIER, enchanttype );
            set_enchant_value( enchantindex, SETICEMODIFIER, enchanttype );
            set_enchant_value( enchantindex, SETZAPMODIFIER, enchanttype );
            set_enchant_value( enchantindex, SETFLASHINGAND, enchanttype );
            set_enchant_value( enchantindex, SETLIGHTBLEND, enchanttype );
            set_enchant_value( enchantindex, SETALPHABLEND, enchanttype );
            set_enchant_value( enchantindex, SETSHEEN, enchanttype );
            set_enchant_value( enchantindex, SETFLYTOHEIGHT, enchanttype );
            set_enchant_value( enchantindex, SETWALKONWATER, enchanttype );
            set_enchant_value( enchantindex, SETCANSEEINVISIBLE, enchanttype );
            set_enchant_value( enchantindex, SETMISSILETREATMENT, enchanttype );
            set_enchant_value( enchantindex, SETCOSTFOREACHMISSILE, enchanttype );
            set_enchant_value( enchantindex, SETCHANNEL, enchanttype );

            // Now do all of the stat adds
            add = 0;

            while ( add < MAXEVEADDVALUE )
            {
                add_enchant_value( enchantindex, add, enchanttype );
                add++;
            }

            // Create an overlay character?
            EncList[enchantindex].overlay = MAXCHR;
            if ( EveList[enchanttype].overlay )
            {
                overlay = spawn_one_character( ChrList[target].xpos, ChrList[target].ypos, ChrList[target].zpos,
                                               enchanttype, ChrList[target].team, 0, ChrList[target].turnleftright,
                                               NULL, MAXCHR );
                if ( overlay < MAXCHR )
                {
                    EncList[enchantindex].overlay = overlay;  // Kill this character on end...
                    ChrList[overlay].ai.target = target;
                    ChrList[overlay].ai.state = EveList[enchanttype].overlay;
                    ChrList[overlay].overlay = btrue;

                    // Start out with ActionMJ...  Object activated
                    if ( MadList[ChrList[overlay].inst.imad].actionvalid[ACTIONMJ] )
                    {
                        ChrList[overlay].action = ACTIONMJ;
                        ChrList[overlay].inst.lip = 0;
                        ChrList[overlay].inst.frame = MadList[ChrList[overlay].inst.imad].actionstart[ACTIONMJ];
                        ChrList[overlay].inst.lastframe = ChrList[overlay].inst.frame;
                        ChrList[overlay].actionready = bfalse;
                    }

                    ChrList[overlay].inst.light = 254;  // Assume it's transparent...
                }
            }
        }

        return enchantindex;
    }

    return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void disenchant_character( Uint16 cnt )
{
    // ZZ> This function removes all enchantments from a character
    while ( ChrList[cnt].firstenchant != MAXENCHANT )
    {
        remove_enchant( ChrList[cnt].firstenchant );
    }
}

//--------------------------------------------------------------------------------------------
void free_all_enchants()
{
    // ZZ> This functions frees all of the enchantments
    numfreeenchant = 0;

    while ( numfreeenchant < MAXENCHANT )
    {
        freeenchant[numfreeenchant] = numfreeenchant;
        EncList[numfreeenchant].on = bfalse;
        numfreeenchant++;
    }
}

//--------------------------------------------------------------------------------------------
bool_t load_one_enchant_type( const char* szLoadName, Uint16 profile )
{
    // ZZ> This function loads the enchantment associated with an object
    FILE* fileread;
    char cTmp;
    int iTmp, tTmp, idsz, test;
    float fTmp;
    int num;

    parse_filename = szLoadName;
    EveList[profile].valid = bfalse;
    fileread = fopen( szLoadName, "r" );
    if ( NULL == fileread )
    {
        return bfalse;
    }

    // btrue/bfalse values
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].retarget = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  EveList[profile].retarget = btrue;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].override = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  EveList[profile].override = btrue;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].removeoverridden = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  EveList[profile].removeoverridden = btrue;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].killonend = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  EveList[profile].killonend = btrue;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].poofonend = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  EveList[profile].poofonend = btrue;

    // More stuff
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  EveList[profile].time = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  EveList[profile].endmessage = iTmp;

    // Drain stuff
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  EveList[profile].ownermana = fTmp * 256;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  EveList[profile].targetmana = fTmp * 256;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].endifcantpay = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  EveList[profile].endifcantpay = btrue;

    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  EveList[profile].ownerlife = fTmp * 256;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  EveList[profile].targetlife = fTmp * 256;

    // Specifics
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].dontdamagetype = DAMAGENULL;
    if ( cTmp == 'S' || cTmp == 's' )  EveList[profile].dontdamagetype = DAMAGE_SLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  EveList[profile].dontdamagetype = DAMAGE_CRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  EveList[profile].dontdamagetype = DAMAGE_POKE;
    if ( cTmp == 'H' || cTmp == 'h' )  EveList[profile].dontdamagetype = DAMAGE_HOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  EveList[profile].dontdamagetype = DAMAGE_EVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  EveList[profile].dontdamagetype = DAMAGE_FIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  EveList[profile].dontdamagetype = DAMAGE_ICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  EveList[profile].dontdamagetype = DAMAGE_ZAP;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].onlydamagetype = DAMAGENULL;
    if ( cTmp == 'S' || cTmp == 's' )  EveList[profile].onlydamagetype = DAMAGE_SLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  EveList[profile].onlydamagetype = DAMAGE_CRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  EveList[profile].onlydamagetype = DAMAGE_POKE;
    if ( cTmp == 'H' || cTmp == 'h' )  EveList[profile].onlydamagetype = DAMAGE_HOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  EveList[profile].onlydamagetype = DAMAGE_EVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  EveList[profile].onlydamagetype = DAMAGE_FIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  EveList[profile].onlydamagetype = DAMAGE_ICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  EveList[profile].onlydamagetype = DAMAGE_ZAP;

    goto_colon( fileread );  EveList[profile].removedbyidsz = fget_idsz( fileread );

    // Now the set values
    num = 0;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );
    EveList[profile].setvalue[num] = DAMAGE_SLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  EveList[profile].setvalue[num] = DAMAGE_CRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  EveList[profile].setvalue[num] = DAMAGE_POKE;
    if ( cTmp == 'H' || cTmp == 'h' )  EveList[profile].setvalue[num] = DAMAGE_HOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  EveList[profile].setvalue[num] = DAMAGE_EVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  EveList[profile].setvalue[num] = DAMAGE_FIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  EveList[profile].setvalue[num] = DAMAGE_ICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  EveList[profile].setvalue[num] = DAMAGE_ZAP;

    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( toupper(cTmp) == 'T' ) iTmp = DAMAGEINVERT;
    if ( toupper(cTmp) == 'C' ) iTmp = DAMAGECHARGE;
    if ( toupper(cTmp) == 'M' ) iTmp = DAMAGEMANA;

    fscanf( fileread, "%d", &tTmp );  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( toupper(cTmp) == 'T' ) iTmp = DAMAGEINVERT;
    if ( toupper(cTmp) == 'C' ) iTmp = DAMAGECHARGE;
    if ( toupper(cTmp) == 'M' ) iTmp = DAMAGEMANA;

    fscanf( fileread, "%d", &tTmp );  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( toupper(cTmp) == 'T' ) iTmp = DAMAGEINVERT;
    if ( toupper(cTmp) == 'C' ) iTmp = DAMAGECHARGE;
    if ( toupper(cTmp) == 'M' ) iTmp = DAMAGEMANA;

    fscanf( fileread, "%d", &tTmp );  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( toupper(cTmp) == 'T' ) iTmp = DAMAGEINVERT;
    if ( toupper(cTmp) == 'C' ) iTmp = DAMAGECHARGE;
    if ( toupper(cTmp) == 'M' ) iTmp = DAMAGEMANA;

    fscanf( fileread, "%d", &tTmp );  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( toupper(cTmp) == 'T' ) iTmp = DAMAGEINVERT;
    if ( toupper(cTmp) == 'C' ) iTmp = DAMAGECHARGE;
    if ( toupper(cTmp) == 'M' ) iTmp = DAMAGEMANA;

    fscanf( fileread, "%d", &tTmp );  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( toupper(cTmp) == 'T' ) iTmp = DAMAGEINVERT;
    if ( toupper(cTmp) == 'C' ) iTmp = DAMAGECHARGE;
    if ( toupper(cTmp) == 'M' ) iTmp = DAMAGEMANA;

    fscanf( fileread, "%d", &tTmp );  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( toupper(cTmp) == 'T' ) iTmp = DAMAGEINVERT;
    if ( toupper(cTmp) == 'C' ) iTmp = DAMAGECHARGE;
    if ( toupper(cTmp) == 'M' ) iTmp = DAMAGEMANA;

    fscanf( fileread, "%d", &tTmp );  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );  iTmp = 0;
    if ( toupper(cTmp) == 'T' ) iTmp = DAMAGEINVERT;
    if ( toupper(cTmp) == 'C' ) iTmp = DAMAGECHARGE;
    if ( toupper(cTmp) == 'M' ) iTmp = DAMAGEMANA;

    fscanf( fileread, "%d", &tTmp );  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );
    EveList[profile].setvalue[num] = ( cTmp == 'T' || cTmp == 't' );
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );
    EveList[profile].setvalue[num] = ( cTmp == 'T' || cTmp == 't' );
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = fget_first_letter( fileread );
    EveList[profile].setvalue[num] = MISNORMAL;
    if ( cTmp == 'R' || cTmp == 'r' )  EveList[profile].setvalue[num] = MISREFLECT;
    if ( cTmp == 'D' || cTmp == 'd' )  EveList[profile].setvalue[num] = MISDEFLECT;

    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%f", &fTmp );  fTmp = fTmp * 16;
    EveList[profile].setvalue[num] = (Uint8) fTmp;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    EveList[profile].setvalue[num] = btrue;
    num++;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    EveList[profile].setyesno[num] = ( cTmp == 'T' || cTmp == 't' );
    EveList[profile].setvalue[num] = btrue;
    num++;

    // Now read in the add values
    num = 0;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 16;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 127;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 127;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 127;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    EveList[profile].addvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    EveList[profile].addvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    EveList[profile].addvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    EveList[profile].addvalue[num] = iTmp;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Defense is backwards
    EveList[profile].addvalue[num] = -iTmp;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    EveList[profile].addvalue[num] = (Sint32) fTmp * 4;
    num++;

    // Clear expansions...
    EveList[profile].contspawntime = 0;
    EveList[profile].contspawnamount = 0;
    EveList[profile].contspawnfacingadd = 0;
    EveList[profile].contspawnpip = 0;
    EveList[profile].waveindex = -1;
    EveList[profile].frequency = 11025;
    EveList[profile].stayifnoowner = 0;
    EveList[profile].overlay = 0;

    // Read expansions
    while ( goto_colon_yesno( fileread ) )
    {
        idsz = fget_idsz( fileread );
        fscanf( fileread, "%c%d", &cTmp, &iTmp );
        test = Make_IDSZ( "AMOU" );  // [AMOU]
        if ( idsz == test )  EveList[profile].contspawnamount = iTmp;

        test = Make_IDSZ( "TYPE" );  // [TYPE]
        if ( idsz == test )  EveList[profile].contspawnpip = iTmp;

        test = Make_IDSZ( "TIME" );  // [TIME]
        if ( idsz == test )  EveList[profile].contspawntime = iTmp;

        test = Make_IDSZ( "FACE" );  // [FACE]
        if ( idsz == test )  EveList[profile].contspawnfacingadd = iTmp;

        test = Make_IDSZ( "SEND" );  // [SEND]
        if ( idsz == test )
        {
            // This is wrong, it gets stored or loaded incorrectly (Loaded in game.c)
            EveList[profile].waveindex = CLIP(iTmp, -1, MAXWAVE);
        }

        test = Make_IDSZ( "SFQR" );  // [SFRQ]
        if ( idsz == test )  EveList[profile].frequency = iTmp;  // OUTDATED??

        test = Make_IDSZ( "STAY" );  // [STAY]
        if ( idsz == test )
            EveList[profile].stayifnoowner = iTmp;

        test = Make_IDSZ( "OVER" );  // [OVER]
        if ( idsz == test )  EveList[profile].overlay = iTmp;

        test = Make_IDSZ( "CKUR" );  // [CKUR]
        if ( idsz == test )  EveList[profile].seekurse = iTmp;
    }

    // All done ( finally )
    fclose( fileread );

    EveList[profile].valid = btrue;

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 get_free_enchant()
{
    // ZZ> This function returns the next free enchantment or MAXENCHANT if there are none
    if ( numfreeenchant > 0 )
    {
        numfreeenchant--;
        return freeenchant[numfreeenchant];
    }

    return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void unset_enchant_value( Uint16 enchantindex, Uint8 valueindex )
{
    // ZZ> This function unsets a set value
    Uint16 character;
    if ( EncList[enchantindex].setyesno[valueindex] )
    {
        character = EncList[enchantindex].target;

        switch ( valueindex )
        {
            case SETDAMAGETYPE:
                ChrList[character].damagetargettype = EncList[enchantindex].setsave[valueindex];
                break;
            case SETNUMBEROFJUMPS:
                ChrList[character].jumpnumberreset = EncList[enchantindex].setsave[valueindex];
                break;
            case SETLIFEBARCOLOR:
                ChrList[character].lifecolor = EncList[enchantindex].setsave[valueindex];
                break;
            case SETMANABARCOLOR:
                ChrList[character].manacolor = EncList[enchantindex].setsave[valueindex];
                break;
            case SETSLASHMODIFIER:
                ChrList[character].damagemodifier[DAMAGE_SLASH] = EncList[enchantindex].setsave[valueindex];
                break;
            case SETCRUSHMODIFIER:
                ChrList[character].damagemodifier[DAMAGE_CRUSH] = EncList[enchantindex].setsave[valueindex];
                break;
            case SETPOKEMODIFIER:
                ChrList[character].damagemodifier[DAMAGE_POKE] = EncList[enchantindex].setsave[valueindex];
                break;
            case SETHOLYMODIFIER:
                ChrList[character].damagemodifier[DAMAGE_HOLY] = EncList[enchantindex].setsave[valueindex];
                break;
            case SETEVILMODIFIER:
                ChrList[character].damagemodifier[DAMAGE_EVIL] = EncList[enchantindex].setsave[valueindex];
                break;
            case SETFIREMODIFIER:
                ChrList[character].damagemodifier[DAMAGE_FIRE] = EncList[enchantindex].setsave[valueindex];
                break;
            case SETICEMODIFIER:
                ChrList[character].damagemodifier[DAMAGE_ICE] = EncList[enchantindex].setsave[valueindex];
                break;
            case SETZAPMODIFIER:
                ChrList[character].damagemodifier[DAMAGE_ZAP] = EncList[enchantindex].setsave[valueindex];
                break;
            case SETFLASHINGAND:
                ChrList[character].flashand = EncList[enchantindex].setsave[valueindex];
                break;
            case SETLIGHTBLEND:
                ChrList[character].inst.light = EncList[enchantindex].setsave[valueindex];
                break;
            case SETALPHABLEND:
                ChrList[character].inst.alpha = EncList[enchantindex].setsave[valueindex];
                break;
            case SETSHEEN:
                ChrList[character].inst.sheen = EncList[enchantindex].setsave[valueindex];
                break;
            case SETFLYTOHEIGHT:
                ChrList[character].flyheight = EncList[enchantindex].setsave[valueindex];
                break;
            case SETWALKONWATER:
                ChrList[character].waterwalk = EncList[enchantindex].setsave[valueindex];
                break;
            case SETCANSEEINVISIBLE:
                ChrList[character].canseeinvisible = EncList[enchantindex].setsave[valueindex];
                break;
            case SETMISSILETREATMENT:
                ChrList[character].missiletreatment = EncList[enchantindex].setsave[valueindex];
                break;
            case SETCOSTFOREACHMISSILE:
                ChrList[character].missilecost = EncList[enchantindex].setsave[valueindex];
                ChrList[character].missilehandler = character;
                break;
            case SETMORPH:
                // Need special handler for when this is removed
                change_character( character, ChrList[character].basemodel, EncList[enchantindex].setsave[valueindex], LEAVEALL );
                break;
            case SETCHANNEL:
                ChrList[character].canchannel = EncList[enchantindex].setsave[valueindex];
                break;
        }

        EncList[enchantindex].setyesno[valueindex] = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
void remove_enchant_value( Uint16 enchantindex, Uint8 valueindex )
{
    // ZZ> This function undoes cumulative modification to character stats
    float fvaluetoadd;
    int valuetoadd;

    Uint16 character = EncList[enchantindex].target;

    switch ( valueindex )
    {
        case ADDJUMPPOWER:
            fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 16.0f;
            ChrList[character].jump -= fvaluetoadd;
            break;
        case ADDBUMPDAMPEN:
            fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 128.0f;
            ChrList[character].bumpdampen -= fvaluetoadd;
            break;
        case ADDBOUNCINESS:
            fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 128.0f;
            ChrList[character].dampen -= fvaluetoadd;
            break;
        case ADDDAMAGE:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].damageboost -= valuetoadd;
            break;
        case ADDSIZE:
            fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 128.0f;
            ChrList[character].sizegoto -= fvaluetoadd;
            ChrList[character].sizegototime = SIZETIME;
            break;
        case ADDACCEL:
            fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 1000.0f;
            ChrList[character].maxaccel -= fvaluetoadd;
            break;
        case ADDRED:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].inst.redshift -= valuetoadd;
            break;
        case ADDGRN:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].inst.grnshift -= valuetoadd;
            break;
        case ADDBLU:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].inst.blushift -= valuetoadd;
            break;
        case ADDDEFENSE:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].defense -= valuetoadd;
            break;
        case ADDMANA:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].manamax -= valuetoadd;
            ChrList[character].mana -= valuetoadd;
            if ( ChrList[character].mana < 0 ) ChrList[character].mana = 0;

            break;
        case ADDLIFE:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].lifemax -= valuetoadd;
            ChrList[character].life -= valuetoadd;
            if ( ChrList[character].life < 1 ) ChrList[character].life = 1;

            break;
        case ADDSTRENGTH:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].strength -= valuetoadd;
            break;
        case ADDWISDOM:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].wisdom -= valuetoadd;
            break;
        case ADDINTELLIGENCE:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].intelligence -= valuetoadd;
            break;
        case ADDDEXTERITY:
            valuetoadd = EncList[enchantindex].addsave[valueindex];
            ChrList[character].dexterity -= valuetoadd;
            break;
    }
}

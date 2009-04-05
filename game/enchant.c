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

/* Egoboo - enchant.c
 */

#include "egoboo.h"
#include "char.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void remove_enchant( Uint16 enchantindex )
{
    // ZZ> This function removes a specific enchantment and adds it to the unused list
    Uint16 character, overlay;
    Uint16 lastenchant, currentenchant;
    int add;

    if ( enchantindex < MAXENCHANT )
    {
        if ( encon[enchantindex] )
        {
            // Unsparkle the spellbook
            character = encspawner[enchantindex];

            if ( character < MAXCHR )
            {
                chr[character].sparkle = NOSPARKLE;

                // Make the spawner unable to undo the enchantment
                if ( chr[character].undoenchant == enchantindex )
                {
                    chr[character].undoenchant = MAXENCHANT;
                }
            }

            // Play the end sound
            character = enctarget[enchantindex];
            if ( evewaveindex[enchantindex] >= 0 && evewaveindex[enchantindex] < MAXWAVE )
            {
                Sint16 iwave    = evewaveindex[enchantindex];
                Uint16 ispawner = encspawner[enchantindex];
                if ( MAXCHR != ispawner && iwave >= 0 && iwave < MAXWAVE )
                {
                    play_mix(chr[character].oldx, chr[character].oldy, capwaveindex[chr[ispawner].model] + iwave);
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
            if ( chr[character].firstenchant == enchantindex )
            {
                // It was the first in the list
                chr[character].firstenchant = encnextenchant[enchantindex];
            }
            else
            {
                // Search until we find it
                lastenchant = currentenchant = chr[character].firstenchant;

                while ( currentenchant != enchantindex )
                {
                    lastenchant = currentenchant;
                    currentenchant = encnextenchant[currentenchant];
                }

                // Relink the last enchantment
                encnextenchant[lastenchant] = encnextenchant[enchantindex];
            }

            // See if we spit out an end message
            if ( eveendmessage[enceve[enchantindex]] >= 0 )
            {
                display_message( madmsgstart[enceve[enchantindex]] + eveendmessage[enceve[enchantindex]], enctarget[enchantindex] );
            }

            // Check to see if we spawn a poof
            if ( evepoofonend[enceve[enchantindex]] )
            {
                spawn_poof( enctarget[enchantindex], enceve[enchantindex] );
            }

            // Check to see if the character dies
            if ( evekillonend[enceve[enchantindex]] )
            {
                if ( chr[character].invictus )  teammorale[chr[character].baseteam]++;

                chr[character].invictus = bfalse;
                kill_character( character, MAXCHR );
            }

            // Kill overlay too...
            overlay = encoverlay[enchantindex];

            if ( overlay < MAXCHR )
            {
                if ( chr[overlay].invictus )  teammorale[chr[overlay].baseteam]++;

                chr[overlay].invictus = bfalse;
                kill_character( overlay, MAXCHR );
            }

            // Remove see kurse enchant
            if ( eveseekurse[enceve[enchantindex]] && !capcanseekurse[chr[character].model] )
            {
                chr[character].canseekurse = bfalse;
            }

            // Now get rid of it
            encon[enchantindex] = bfalse;
            freeenchant[numfreeenchant] = enchantindex;
            numfreeenchant++;

            // Now fix dem weapons
            reset_character_alpha( chr[character].holdingwhich[0] );
            reset_character_alpha( chr[character].holdingwhich[1] );
        }
    }
}

//--------------------------------------------------------------------------------------------
Uint16 enchant_value_filled( Uint16 enchantindex, Uint8 valueindex )
{
    // ZZ> This function returns MAXENCHANT if the enchantment's target has no conflicting
    //     set values in its other enchantments.  Otherwise it returns the enchantindex
    //     of the conflicting enchantment
    Uint16 character, currenchant;

    character = enctarget[enchantindex];
    currenchant = chr[character].firstenchant;

    while ( currenchant != MAXENCHANT )
    {
        if ( encsetyesno[currenchant][valueindex] )
        {
            return currenchant;
        }

        currenchant = encnextenchant[currenchant];
    }

    return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void set_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype )
{
    // ZZ> This function sets and saves one of the character's stats
    Uint16 conflict, character;

    encsetyesno[enchantindex][valueindex] = bfalse;

    if ( evesetyesno[enchanttype][valueindex] )
    {
        conflict = enchant_value_filled( enchantindex, valueindex );

        if ( conflict == MAXENCHANT || eveoverride[enchanttype] )
        {
            // Check for multiple enchantments
            if ( conflict < MAXENCHANT )
            {
                // Multiple enchantments aren't allowed for sets
                if ( everemoveoverridden[enchanttype] )
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
            character = enctarget[enchantindex];
            encsetyesno[enchantindex][valueindex] = btrue;

            switch ( valueindex )
            {
                case SETDAMAGETYPE:
                    encsetsave[enchantindex][valueindex] = chr[character].damagetargettype;
                    chr[character].damagetargettype = evesetvalue[enchanttype][valueindex];
                    break;
                case SETNUMBEROFJUMPS:
                    encsetsave[enchantindex][valueindex] = chr[character].jumpnumberreset;
                    chr[character].jumpnumberreset = evesetvalue[enchanttype][valueindex];
                    break;
                case SETLIFEBARCOLOR:
                    encsetsave[enchantindex][valueindex] = chr[character].lifecolor;
                    chr[character].lifecolor = evesetvalue[enchanttype][valueindex];
                    break;
                case SETMANABARCOLOR:
                    encsetsave[enchantindex][valueindex] = chr[character].manacolor;
                    chr[character].manacolor = evesetvalue[enchanttype][valueindex];
                    break;
                case SETSLASHMODIFIER:
                    encsetsave[enchantindex][valueindex] = chr[character].damagemodifier[DAMAGE_SLASH];
                    chr[character].damagemodifier[DAMAGE_SLASH] = evesetvalue[enchanttype][valueindex];
                    break;
                case SETCRUSHMODIFIER:
                    encsetsave[enchantindex][valueindex] = chr[character].damagemodifier[DAMAGE_CRUSH];
                    chr[character].damagemodifier[DAMAGE_CRUSH] = evesetvalue[enchanttype][valueindex];
                    break;
                case SETPOKEMODIFIER:
                    encsetsave[enchantindex][valueindex] = chr[character].damagemodifier[DAMAGE_POKE];
                    chr[character].damagemodifier[DAMAGE_POKE] = evesetvalue[enchanttype][valueindex];
                    break;
                case SETHOLYMODIFIER:
                    encsetsave[enchantindex][valueindex] = chr[character].damagemodifier[DAMAGE_HOLY];
                    chr[character].damagemodifier[DAMAGE_HOLY] = evesetvalue[enchanttype][valueindex];
                    break;
                case SETEVILMODIFIER:
                    encsetsave[enchantindex][valueindex] = chr[character].damagemodifier[DAMAGE_EVIL];
                    chr[character].damagemodifier[DAMAGE_EVIL] = evesetvalue[enchanttype][valueindex];
                    break;
                case SETFIREMODIFIER:
                    encsetsave[enchantindex][valueindex] = chr[character].damagemodifier[DAMAGE_FIRE];
                    chr[character].damagemodifier[DAMAGE_FIRE] = evesetvalue[enchanttype][valueindex];
                    break;
                case SETICEMODIFIER:
                    encsetsave[enchantindex][valueindex] = chr[character].damagemodifier[DAMAGE_ICE];
                    chr[character].damagemodifier[DAMAGE_ICE] = evesetvalue[enchanttype][valueindex];
                    break;
                case SETZAPMODIFIER:
                    encsetsave[enchantindex][valueindex] = chr[character].damagemodifier[DAMAGE_ZAP];
                    chr[character].damagemodifier[DAMAGE_ZAP] = evesetvalue[enchanttype][valueindex];
                    break;
                case SETFLASHINGAND:
                    encsetsave[enchantindex][valueindex] = chr[character].flashand;
                    chr[character].flashand = evesetvalue[enchanttype][valueindex];
                    break;
                case SETLIGHTBLEND:
                    encsetsave[enchantindex][valueindex] = chr[character].light;
                    chr[character].light = evesetvalue[enchanttype][valueindex];
                    break;
                case SETALPHABLEND:
                    encsetsave[enchantindex][valueindex] = chr[character].alpha;
                    chr[character].alpha = evesetvalue[enchanttype][valueindex];
                    break;
                case SETSHEEN:
                    encsetsave[enchantindex][valueindex] = chr[character].sheen;
                    chr[character].sheen = evesetvalue[enchanttype][valueindex];
                    break;
                case SETFLYTOHEIGHT:
                    encsetsave[enchantindex][valueindex] = chr[character].flyheight;

                    if ( chr[character].flyheight == 0 && chr[character].zpos > -2 )
                    {
                        chr[character].flyheight = evesetvalue[enchanttype][valueindex];
                    }
                    break;
                case SETWALKONWATER:
                    encsetsave[enchantindex][valueindex] = chr[character].waterwalk;

                    if ( !chr[character].waterwalk )
                    {
                        chr[character].waterwalk = evesetvalue[enchanttype][valueindex];
                    }
                    break;
                case SETCANSEEINVISIBLE:
                    encsetsave[enchantindex][valueindex] = chr[character].canseeinvisible;
                    chr[character].canseeinvisible = evesetvalue[enchanttype][valueindex];
                    break;
                case SETMISSILETREATMENT:
                    encsetsave[enchantindex][valueindex] = chr[character].missiletreatment;
                    chr[character].missiletreatment = evesetvalue[enchanttype][valueindex];
                    break;
                case SETCOSTFOREACHMISSILE:
                    encsetsave[enchantindex][valueindex] = chr[character].missilecost;
                    chr[character].missilecost = evesetvalue[enchanttype][valueindex];
                    chr[character].missilehandler = encowner[enchantindex];
                    break;
                case SETMORPH:
                    encsetsave[enchantindex][valueindex] = chr[character].texture - madskinstart[chr[character].model];
                    // Special handler for morph
                    change_character( character, enchanttype, 0, LEAVEALL ); // LEAVEFIRST);
                    chr[character].alert |= ALERTIFCHANGED;
                    break;
                case SETCHANNEL:
                    encsetsave[enchantindex][valueindex] = chr[character].canchannel;
                    chr[character].canchannel = evesetvalue[enchanttype][valueindex];
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

    character = enctarget[enchantindex];

    valuetoadd = 0;

    switch ( valueindex )
    {
        case ADDJUMPPOWER:
            fnewvalue = chr[character].jump;
            fvaluetoadd = eveaddvalue[enchanttype][valueindex] / 16.0f;
            fgetadd( 0, fnewvalue, 30.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 16.0f; // Get save value
            fvaluetoadd = valuetoadd / 16.0f;
            chr[character].jump += fvaluetoadd;
            break;
        case ADDBUMPDAMPEN:
            fnewvalue = chr[character].bumpdampen;
            fvaluetoadd = eveaddvalue[enchanttype][valueindex] / 128.0f;
            fgetadd( 0, fnewvalue, 1.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            chr[character].bumpdampen += fvaluetoadd;
            break;
        case ADDBOUNCINESS:
            fnewvalue = chr[character].dampen;
            fvaluetoadd = eveaddvalue[enchanttype][valueindex] / 128.0f;
            fgetadd( 0, fnewvalue, 0.95f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            chr[character].dampen += fvaluetoadd;
            break;
        case ADDDAMAGE:
            newvalue = chr[character].damageboost;
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd( 0, newvalue, 4096, &valuetoadd );
            chr[character].damageboost += valuetoadd;
            break;
        case ADDSIZE:
            fnewvalue = chr[character].sizegoto;
            fvaluetoadd = eveaddvalue[enchanttype][valueindex] / 128.0f;
            fgetadd( 0.5f, fnewvalue, 2.0f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 128.0f; // Get save value
            fvaluetoadd = valuetoadd / 128.0f;
            chr[character].sizegoto += fvaluetoadd;
            chr[character].sizegototime = SIZETIME;
            break;
        case ADDACCEL:
            fnewvalue = chr[character].maxaccel;
            fvaluetoadd = eveaddvalue[enchanttype][valueindex] / 80.0f;
            fgetadd( 0, fnewvalue, 1.5f, &fvaluetoadd );
            valuetoadd = fvaluetoadd * 1000.0f; // Get save value
            fvaluetoadd = valuetoadd / 1000.0f;
            chr[character].maxaccel += fvaluetoadd;
            break;
        case ADDRED:
            newvalue = chr[character].redshift;
            valuetoadd = eveaddvalue[enchanttype][valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr[character].redshift += valuetoadd;
            break;
        case ADDGRN:
            newvalue = chr[character].grnshift;
            valuetoadd = eveaddvalue[enchanttype][valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr[character].grnshift += valuetoadd;
            break;
        case ADDBLU:
            newvalue = chr[character].blushift;
            valuetoadd = eveaddvalue[enchanttype][valueindex];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr[character].blushift += valuetoadd;
            break;
        case ADDDEFENSE:
            newvalue = chr[character].defense;
            valuetoadd = eveaddvalue[enchanttype][valueindex];
            getadd( 55, newvalue, 255, &valuetoadd );  // Don't fix again!
            chr[character].defense += valuetoadd;
            break;
        case ADDMANA:
            newvalue = chr[character].manamax;
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd( 0, newvalue, PERFECTBIG, &valuetoadd );
            chr[character].manamax += valuetoadd;
            chr[character].mana += valuetoadd;

            if ( chr[character].mana < 0 )  chr[character].mana = 0;

            break;
        case ADDLIFE:
            newvalue = chr[character].lifemax;
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd( LOWSTAT, newvalue, PERFECTBIG, &valuetoadd );
            chr[character].lifemax += valuetoadd;
            chr[character].life += valuetoadd;

            if ( chr[character].life < 1 )  chr[character].life = 1;

            break;
        case ADDSTRENGTH:
            newvalue = chr[character].strength;
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            chr[character].strength += valuetoadd;
            break;
        case ADDWISDOM:
            newvalue = chr[character].wisdom;
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            chr[character].wisdom += valuetoadd;
            break;
        case ADDINTELLIGENCE:
            newvalue = chr[character].intelligence;
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            chr[character].intelligence += valuetoadd;
            break;
        case ADDDEXTERITY:
            newvalue = chr[character].dexterity;
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            chr[character].dexterity += valuetoadd;
            break;
    }

    encaddsave[enchantindex][valueindex] = valuetoadd;  // Save the value for undo
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_enchant( Uint16 owner, Uint16 target,
                      Uint16 spawner, Uint16 enchantindex, Uint16 modeloptional )
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
        enchanttype = chr[spawner].model;
    }

    // Target and owner must both be alive and on and valid
    if ( target < MAXCHR )
    {
        if ( !chr[target].on || !chr[target].alive )
            return MAXENCHANT;
    }
    else
    {
        // Invalid target
        return MAXENCHANT;
    }

    if ( owner < MAXCHR )
    {
        if ( !chr[owner].on || !chr[owner].alive )
            return MAXENCHANT;
    }
    else
    {
        // Invalid target
        return MAXENCHANT;
    }

    if ( evevalid[enchanttype] )
    {
        if ( enchantindex == MAXENCHANT )
        {
            // Should it choose an inhand item?
            if ( everetarget[enchanttype] )
            {
                // Is at least one valid?
                if ( chr[target].holdingwhich[0] == MAXCHR && chr[target].holdingwhich[1] == MAXCHR )
                {
                    // No weapons to pick
                    return MAXENCHANT;
                }

                // Left, right, or both are valid
                if ( chr[target].holdingwhich[0] == MAXCHR )
                {
                    // Only right hand is valid
                    target = chr[target].holdingwhich[1];
                }
                else
                {
                    // Pick left hand
                    target = chr[target].holdingwhich[0];
                }
            }

            // Make sure it's valid
            if ( evedontdamagetype[enchanttype] != DAMAGENULL )
            {
                if ( ( chr[target].damagemodifier[evedontdamagetype[enchanttype]]&7 ) >= 3 )  // Invert | Shift = 7
                {
                    return MAXENCHANT;
                }
            }

            if ( eveonlydamagetype[enchanttype] != DAMAGENULL )
            {
                if ( chr[target].damagetargettype != eveonlydamagetype[enchanttype] )
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
            encon[enchantindex]      = btrue;
            enctarget[enchantindex]  = target;
            encowner[enchantindex]   = owner;
            encspawner[enchantindex] = spawner;

            if ( spawner < MAXCHR )
            {
                chr[spawner].undoenchant = enchantindex;
            }

            enceve[enchantindex] = enchanttype;
            enctime[enchantindex] = evetime[enchanttype];
            encspawntime[enchantindex] = 1;
            encownermana[enchantindex] = eveownermana[enchanttype];
            encownerlife[enchantindex] = eveownerlife[enchanttype];
            enctargetmana[enchantindex] = evetargetmana[enchanttype];
            enctargetlife[enchantindex] = evetargetlife[enchanttype];

            // Add it as first in the list
            encnextenchant[enchantindex] = chr[target].firstenchant;
            chr[target].firstenchant = enchantindex;

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
            encoverlay[enchantindex] = MAXCHR;

            if ( eveoverlay[enchanttype] )
            {
                overlay = spawn_one_character( chr[target].xpos, chr[target].ypos, chr[target].zpos,
                                               enchanttype, chr[target].team, 0, chr[target].turnleftright,
                                               NULL, MAXCHR );

                if ( overlay < MAXCHR )
                {
                    encoverlay[enchantindex] = overlay;  // Kill this character on end...
                    chr[overlay].aitarget = target;
                    chr[overlay].aistate = eveoverlay[enchanttype];
                    chr[overlay].overlay = btrue;

                    // Start out with ActionMJ...  Object activated
                    if ( madactionvalid[chr[overlay].model][ACTIONMJ] )
                    {
                        chr[overlay].action = ACTIONMJ;
                        chr[overlay].lip = 0;
                        chr[overlay].frame = madactionstart[chr[overlay].model][ACTIONMJ];
                        chr[overlay].lastframe = chr[overlay].frame;
                        chr[overlay].actionready = bfalse;
                    }

                    chr[overlay].light = 254;  // Assume it's transparent...
                }
            }
        }

        return enchantindex;
    }

    return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void do_enchant_spawn()
{
    // ZZ> This function lets enchantments spawn particles
    int cnt, tnc;
    Uint16 facing, particle, eve, character;

    cnt = 0;

    while ( cnt < MAXENCHANT )
    {
        if ( encon[cnt] )
        {
            eve = enceve[cnt];

            if ( evecontspawnamount[eve] > 0 )
            {
                encspawntime[cnt]--;

                if ( encspawntime[cnt] == 0 )
                {
                    character = enctarget[cnt];
                    encspawntime[cnt] = evecontspawntime[eve];
                    facing = chr[character].turnleftright;
                    tnc = 0;

                    while ( tnc < evecontspawnamount[eve] )
                    {
                        particle = spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos,
                                                       facing, eve, evecontspawnpip[eve],
                                                       MAXCHR, SPAWNLAST, chr[encowner[cnt]].team, encowner[cnt], tnc, MAXCHR );
                        facing += evecontspawnfacingadd[eve];
                        tnc++;
                    }
                }
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void disenchant_character( Uint16 cnt )
{
    // ZZ> This function removes all enchantments from a character
    while ( chr[cnt].firstenchant != MAXENCHANT )
    {
        remove_enchant( chr[cnt].firstenchant );
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
        encon[numfreeenchant] = bfalse;
        numfreeenchant++;
    }
}

//--------------------------------------------------------------------------------------------
void load_one_enchant_type( char* szLoadName, Uint16 profile )
{
    // ZZ> This function loads the enchantment associated with an object
    FILE* fileread;
    char cTmp;
    int iTmp, tTmp, idsz, test;
    float fTmp;
    int num;

    parse_filename = szLoadName;
    evevalid[profile] = bfalse;
    fileread = fopen( szLoadName, "r" );

    if ( fileread )
    {
        evevalid[profile] = btrue;

        // btrue/bfalse values
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        everetarget[profile] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  everetarget[profile] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        eveoverride[profile] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  eveoverride[profile] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        everemoveoverridden[profile] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  everemoveoverridden[profile] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evekillonend[profile] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  evekillonend[profile] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evepoofonend[profile] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  evepoofonend[profile] = btrue;

        // More stuff
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  evetime[profile] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  eveendmessage[profile] = iTmp;

        // Drain stuff
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  eveownermana[profile] = fTmp * 256;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  evetargetmana[profile] = fTmp * 256;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        eveendifcantpay[profile] = bfalse;

        if ( cTmp == 'T' || cTmp == 't' )  eveendifcantpay[profile] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  eveownerlife[profile] = fTmp * 256;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  evetargetlife[profile] = fTmp * 256;

        // Specifics
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evedontdamagetype[profile] = DAMAGENULL;

        if ( cTmp == 'S' || cTmp == 's' )  evedontdamagetype[profile] = DAMAGE_SLASH;

        if ( cTmp == 'C' || cTmp == 'c' )  evedontdamagetype[profile] = DAMAGE_CRUSH;

        if ( cTmp == 'P' || cTmp == 'p' )  evedontdamagetype[profile] = DAMAGE_POKE;

        if ( cTmp == 'H' || cTmp == 'h' )  evedontdamagetype[profile] = DAMAGE_HOLY;

        if ( cTmp == 'E' || cTmp == 'e' )  evedontdamagetype[profile] = DAMAGE_EVIL;

        if ( cTmp == 'F' || cTmp == 'f' )  evedontdamagetype[profile] = DAMAGE_FIRE;

        if ( cTmp == 'I' || cTmp == 'i' )  evedontdamagetype[profile] = DAMAGE_ICE;

        if ( cTmp == 'Z' || cTmp == 'z' )  evedontdamagetype[profile] = DAMAGE_ZAP;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        eveonlydamagetype[profile] = DAMAGENULL;

        if ( cTmp == 'S' || cTmp == 's' )  eveonlydamagetype[profile] = DAMAGE_SLASH;

        if ( cTmp == 'C' || cTmp == 'c' )  eveonlydamagetype[profile] = DAMAGE_CRUSH;

        if ( cTmp == 'P' || cTmp == 'p' )  eveonlydamagetype[profile] = DAMAGE_POKE;

        if ( cTmp == 'H' || cTmp == 'h' )  eveonlydamagetype[profile] = DAMAGE_HOLY;

        if ( cTmp == 'E' || cTmp == 'e' )  eveonlydamagetype[profile] = DAMAGE_EVIL;

        if ( cTmp == 'F' || cTmp == 'f' )  eveonlydamagetype[profile] = DAMAGE_FIRE;

        if ( cTmp == 'I' || cTmp == 'i' )  eveonlydamagetype[profile] = DAMAGE_ICE;

        if ( cTmp == 'Z' || cTmp == 'z' )  eveonlydamagetype[profile] = DAMAGE_ZAP;

        goto_colon( fileread );  everemovedbyidsz[profile] = get_idsz( fileread );

        // Now the set values
        num = 0;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );
        evesetvalue[profile][num] = DAMAGE_SLASH;

        if ( cTmp == 'C' || cTmp == 'c' )  evesetvalue[profile][num] = DAMAGE_CRUSH;

        if ( cTmp == 'P' || cTmp == 'p' )  evesetvalue[profile][num] = DAMAGE_POKE;

        if ( cTmp == 'H' || cTmp == 'h' )  evesetvalue[profile][num] = DAMAGE_HOLY;

        if ( cTmp == 'E' || cTmp == 'e' )  evesetvalue[profile][num] = DAMAGE_EVIL;

        if ( cTmp == 'F' || cTmp == 'f' )  evesetvalue[profile][num] = DAMAGE_FIRE;

        if ( cTmp == 'I' || cTmp == 'i' )  evesetvalue[profile][num] = DAMAGE_ICE;

        if ( cTmp == 'Z' || cTmp == 'z' )  evesetvalue[profile][num] = DAMAGE_ZAP;

        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );  iTmp = 0;

        if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;

        if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;

        fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );  iTmp = 0;

        if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;

        if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;

        fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );  iTmp = 0;

        if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;

        if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;

        fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );  iTmp = 0;

        if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;

        if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;

        fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );  iTmp = 0;

        if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;

        if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;

        fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );  iTmp = 0;

        if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;

        if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;

        fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );  iTmp = 0;

        if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;

        if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;

        fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );  iTmp = 0;

        if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;

        if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;

        fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );
        evesetvalue[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );
        evesetvalue[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        cTmp = get_first_letter( fileread );
        evesetvalue[profile][num] = MISNORMAL;

        if ( cTmp == 'R' || cTmp == 'r' )  evesetvalue[profile][num] = MISREFLECT;

        if ( cTmp == 'D' || cTmp == 'd' )  evesetvalue[profile][num] = MISDEFLECT;

        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        fscanf( fileread, "%f", &fTmp );  fTmp = fTmp * 16;
        evesetvalue[profile][num] = (Uint8) fTmp;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        evesetvalue[profile][num] = btrue;
        num++;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
        evesetvalue[profile][num] = btrue;
        num++;

        // Now read in the add values
        num = 0;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 16;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 127;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 127;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 4;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 127;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
        eveaddvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
        eveaddvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
        eveaddvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
        eveaddvalue[profile][num] = iTmp;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Defense is backwards
        eveaddvalue[profile][num] = -iTmp;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 4;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 4;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 4;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 4;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 4;
        num++;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
        eveaddvalue[profile][num] = (Sint32) fTmp * 4;
        num++;

        // Clear expansions...
        evecontspawntime[profile] = 0;
        evecontspawnamount[profile] = 0;
        evecontspawnfacingadd[profile] = 0;
        evecontspawnpip[profile] = 0;
        evewaveindex[profile] = -1;
        evefrequency[profile] = 11025;
        evestayifnoowner[profile] = 0;
        eveoverlay[profile] = 0;

        // Read expansions
        while ( goto_colon_yesno( fileread ) )
        {
            idsz = get_idsz( fileread );
            fscanf( fileread, "%c%d", &cTmp, &iTmp );
            test = Make_IDSZ( "AMOU" );  // [AMOU]

            if ( idsz == test )  evecontspawnamount[profile] = iTmp;

            test = Make_IDSZ( "TYPE" );  // [TYPE]

            if ( idsz == test )  evecontspawnpip[profile] = iTmp;

            test = Make_IDSZ( "TIME" );  // [TIME]

            if ( idsz == test )  evecontspawntime[profile] = iTmp;

            test = Make_IDSZ( "FACE" );  // [FACE]

            if ( idsz == test )  evecontspawnfacingadd[profile] = iTmp;

            test = Make_IDSZ( "SEND" );  // [SEND]

            if ( idsz == test )
            {
                // This is wrong, it gets stored or loaded incorrectly (Loaded in game.c)
                evewaveindex[profile] = CLIP(iTmp, -1, MAXWAVE);
            }

            test = Make_IDSZ( "SFQR" );  // [SFRQ]

            if ( idsz == test )  evefrequency[profile] = iTmp;  // OUTDATED??

            test = Make_IDSZ( "STAY" );  // [STAY]

            if ( idsz == test )  evestayifnoowner[profile] = iTmp;

            test = Make_IDSZ( "OVER" );  // [OVER]

            if ( idsz == test )  eveoverlay[profile] = iTmp;

            test = Make_IDSZ( "CKUR" );  // [CKUR]

            if ( idsz == test )  eveseekurse[profile] = iTmp;
        }

        // All done ( finally )
        fclose( fileread );
    }
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

    if ( encsetyesno[enchantindex][valueindex] )
    {
        character = enctarget[enchantindex];

        switch ( valueindex )
        {
            case SETDAMAGETYPE:
                chr[character].damagetargettype = encsetsave[enchantindex][valueindex];
                break;
            case SETNUMBEROFJUMPS:
                chr[character].jumpnumberreset = encsetsave[enchantindex][valueindex];
                break;
            case SETLIFEBARCOLOR:
                chr[character].lifecolor = encsetsave[enchantindex][valueindex];
                break;
            case SETMANABARCOLOR:
                chr[character].manacolor = encsetsave[enchantindex][valueindex];
                break;
            case SETSLASHMODIFIER:
                chr[character].damagemodifier[DAMAGE_SLASH] = encsetsave[enchantindex][valueindex];
                break;
            case SETCRUSHMODIFIER:
                chr[character].damagemodifier[DAMAGE_CRUSH] = encsetsave[enchantindex][valueindex];
                break;
            case SETPOKEMODIFIER:
                chr[character].damagemodifier[DAMAGE_POKE] = encsetsave[enchantindex][valueindex];
                break;
            case SETHOLYMODIFIER:
                chr[character].damagemodifier[DAMAGE_HOLY] = encsetsave[enchantindex][valueindex];
                break;
            case SETEVILMODIFIER:
                chr[character].damagemodifier[DAMAGE_EVIL] = encsetsave[enchantindex][valueindex];
                break;
            case SETFIREMODIFIER:
                chr[character].damagemodifier[DAMAGE_FIRE] = encsetsave[enchantindex][valueindex];
                break;
            case SETICEMODIFIER:
                chr[character].damagemodifier[DAMAGE_ICE] = encsetsave[enchantindex][valueindex];
                break;
            case SETZAPMODIFIER:
                chr[character].damagemodifier[DAMAGE_ZAP] = encsetsave[enchantindex][valueindex];
                break;
            case SETFLASHINGAND:
                chr[character].flashand = encsetsave[enchantindex][valueindex];
                break;
            case SETLIGHTBLEND:
                chr[character].light = encsetsave[enchantindex][valueindex];
                break;
            case SETALPHABLEND:
                chr[character].alpha = encsetsave[enchantindex][valueindex];
                break;
            case SETSHEEN:
                chr[character].sheen = encsetsave[enchantindex][valueindex];
                break;
            case SETFLYTOHEIGHT:
                chr[character].flyheight = encsetsave[enchantindex][valueindex];
                break;
            case SETWALKONWATER:
                chr[character].waterwalk = encsetsave[enchantindex][valueindex];
                break;
            case SETCANSEEINVISIBLE:
                chr[character].canseeinvisible = encsetsave[enchantindex][valueindex];
                break;
            case SETMISSILETREATMENT:
                chr[character].missiletreatment = encsetsave[enchantindex][valueindex];
                break;
            case SETCOSTFOREACHMISSILE:
                chr[character].missilecost = encsetsave[enchantindex][valueindex];
                chr[character].missilehandler = character;
                break;
            case SETMORPH:
                // Need special handler for when this is removed
                change_character( character, chr[character].basemodel, encsetsave[enchantindex][valueindex], LEAVEALL );
                break;
            case SETCHANNEL:
                chr[character].canchannel = encsetsave[enchantindex][valueindex];
                break;
        }

        encsetyesno[enchantindex][valueindex] = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
void remove_enchant_value( Uint16 enchantindex, Uint8 valueindex )
{
    // ZZ> This function undoes cumulative modification to character stats
    float fvaluetoadd;
    int valuetoadd;

    Uint16 character = enctarget[enchantindex];

    switch ( valueindex )
    {
        case ADDJUMPPOWER:
            fvaluetoadd = encaddsave[enchantindex][valueindex] / 16.0f;
            chr[character].jump -= fvaluetoadd;
            break;
        case ADDBUMPDAMPEN:
            fvaluetoadd = encaddsave[enchantindex][valueindex] / 128.0f;
            chr[character].bumpdampen -= fvaluetoadd;
            break;
        case ADDBOUNCINESS:
            fvaluetoadd = encaddsave[enchantindex][valueindex] / 128.0f;
            chr[character].dampen -= fvaluetoadd;
            break;
        case ADDDAMAGE:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].damageboost -= valuetoadd;
            break;
        case ADDSIZE:
            fvaluetoadd = encaddsave[enchantindex][valueindex] / 128.0f;
            chr[character].sizegoto -= fvaluetoadd;
            chr[character].sizegototime = SIZETIME;
            break;
        case ADDACCEL:
            fvaluetoadd = encaddsave[enchantindex][valueindex] / 1000.0f;
            chr[character].maxaccel -= fvaluetoadd;
            break;
        case ADDRED:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].redshift -= valuetoadd;
            break;
        case ADDGRN:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].grnshift -= valuetoadd;
            break;
        case ADDBLU:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].blushift -= valuetoadd;
            break;
        case ADDDEFENSE:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].defense -= valuetoadd;
            break;
        case ADDMANA:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].manamax -= valuetoadd;
            chr[character].mana -= valuetoadd;

            if ( chr[character].mana < 0 ) chr[character].mana = 0;

            break;
        case ADDLIFE:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].lifemax -= valuetoadd;
            chr[character].life -= valuetoadd;

            if ( chr[character].life < 1 ) chr[character].life = 1;

            break;
        case ADDSTRENGTH:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].strength -= valuetoadd;
            break;
        case ADDWISDOM:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].wisdom -= valuetoadd;
            break;
        case ADDINTELLIGENCE:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].intelligence -= valuetoadd;
            break;
        case ADDDEXTERITY:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chr[character].dexterity -= valuetoadd;
            break;
    }
}

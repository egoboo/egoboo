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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - enchant.h
 * Decleares some stuff used for handling enchants
 */

#include "egoboo_typedef.h"

//Different set values for enchants
typedef enum enchant_set
{
    SETDAMAGETYPE = 0,      //Type of damage dealt
    SETNUMBEROFJUMPS,       //Max number of jumps
    SETLIFEBARCOLOR,        //Color of life bar
    SETMANABARCOLOR,        //Color of mana bar
    SETSLASHMODIFIER,      // Damage modifiers
    SETCRUSHMODIFIER,
    SETPOKEMODIFIER,
    SETHOLYMODIFIER,
    SETEVILMODIFIER,
    SETFIREMODIFIER,
    SETICEMODIFIER,
    SETZAPMODIFIER,
    SETFLASHINGAND,             //Flash rate
    SETLIGHTBLEND,              //Transparency
    SETALPHABLEND,              //Alpha
    SETSHEEN,                   // Shinyness
    SETFLYTOHEIGHT,             //Fly to this height
    SETWALKONWATER,             //Walk on water?
    SETCANSEEINVISIBLE,         //Can it see invisible?
    SETMISSILETREATMENT,        //How to treat missiles
    SETCOSTFOREACHMISSILE,      //Cost for each missile treat
    SETMORPH,                   // Morph character?
    SETCHANNEL,                 // Can channel life as mana?
    MAX_ENCHANT_SET,
} enum_enchant_set;

typedef enum enchant_add
{
    ADDJUMPPOWER = 0,
    ADDBUMPDAMPEN,
    ADDBOUNCINESS,
    ADDDAMAGE,
    ADDSIZE,
    ADDACCEL,
    ADDRED,                        // Red shift
    ADDGRN,                        // Green shift
    ADDBLU,                        // Blue shift
    ADDDEFENSE,                    // Defence adjustments
    ADDMANA,
    ADDLIFE,
    ADDSTRENGTH,
    ADDWISDOM,
    ADDINTELLIGENCE,
    ADDDEXTERITY,
} enum_enchant_add;


//Prototypes
void free_all_enchants();
void getadd( int min, int value, int max, int* valuetoadd );
void fgetadd( float min, float value, float max, float* valuetoadd );
Uint16 get_free_enchant();
Uint16 enchant_value_filled( Uint16 enchantindex, Uint8 valueindex );
bool_t remove_enchant( Uint16 enchantindex );
void set_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
void add_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
Uint16 spawn_enchant( Uint16 owner, Uint16 target,
                      Uint16 spawner, Uint16 enchantindex, Uint16 modeloptional );
bool_t load_one_enchant_type( const char* szLoadName, Uint16 profile );
void unset_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void remove_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void disenchant_character( Uint16 cnt );

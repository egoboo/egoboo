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
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define LEAVEALL                0
#define LEAVEFIRST              1
#define LEAVENONE               2

// missile treatments
#define MISNORMAL               0                  // Treat missiles normally
#define MISDEFLECT              1                  // Deflect incoming missiles
#define MISREFLECT              2                  // Reflect them back!

// Different set values for enchants
typedef enum enchant_set
{
    SETDAMAGETYPE = 0,      // Type of damage dealt
    SETNUMBEROFJUMPS,       // Max number of jumps
    SETLIFEBARCOLOR,        // Color of life bar
    SETMANABARCOLOR,        // Color of mana bar
    SETSLASHMODIFIER,      // Damage modifiers
    SETCRUSHMODIFIER,
    SETPOKEMODIFIER,
    SETHOLYMODIFIER,
    SETEVILMODIFIER,
    SETFIREMODIFIER,
    SETICEMODIFIER,
    SETZAPMODIFIER,
    SETFLASHINGAND,             // Flash rate
    SETLIGHTBLEND,              // Transparency
    SETALPHABLEND,              // Alpha
    SETSHEEN,                   // Shinyness
    SETFLYTOHEIGHT,             // Fly to this height
    SETWALKONWATER,             // Walk on water?
    SETCANSEEINVISIBLE,         // Can it see invisible?
    SETMISSILETREATMENT,        // How to treat missiles
    SETCOSTFOREACHMISSILE,      // Cost for each missile treat
    SETMORPH,                   // Morph character?
    SETCHANNEL,                 // Can channel life as mana?
    MAX_ENCHANT_SET
} enum_enchant_set;

//--------------------------------------------------------------------------------------------
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
    MAX_ENCHANT_ADD
} enum_enchant_add;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Enchantment template

#define MAX_EVE                          MAX_PROFILE    // One enchant type per model

struct s_eve
{
    EGO_PROFILE_STUFF

    bool_t  override;                    // Override other enchants?
    bool_t  removeoverridden;            // Remove other enchants?
    bool_t  retarget;                    // Pick a weapon?
    bool_t  killonend;                   // Kill the target on end?
    bool_t  poofonend;                   // Spawn a poof on end?
    bool_t  endifcantpay;                // End on out of mana
    bool_t  stayifnoowner;               // Stay if owner has died?
    Sint16  time;                        // Time in seconds
    Sint32  endmessage;                  // Message for end -1 for none
    Sint16  ownermana;                   // Boost values
    Sint16  ownerlife;
    Sint16  targetmana;
    Sint16  targetlife;
    Uint8   dontdamagetype;              // Don't work if ...
    Uint8   onlydamagetype;              // Only work if ...
    IDSZ    removedbyidsz;               // By particle or [NONE]
    Uint16  contspawntime;               // Spawn timer
    Uint8   contspawnamount;             // Spawn amount
    Uint16  contspawnfacingadd;          // Spawn in circle
    Uint16  contspawnpip;                // Spawn type ( local )
    Sint16  endsoundindex;               // Sound on end (-1 for none)
    Uint8   overlay;                     // Spawn an overlay?
    Uint16  seekurse;                    // Allow target to see kurses
    bool_t  stayifdead;                  // Stay if target has died?

    bool_t  setyesno[MAX_ENCHANT_SET];    // Set this value?
    Uint8   setvalue[MAX_ENCHANT_SET];    // Value to use
    Sint32  addvalue[MAX_ENCHANT_ADD];    // The values to add

};
typedef struct s_eve eve_t;

DEFINE_STACK( extern, eve_t, EveStack, MAX_EVE );

#define VALID_EVE_RANGE( IEVE ) ( ((IEVE) >= 0) && ((IEVE) < MAX_EVE) )
#define VALID_EVE( IEVE )       ( VALID_EVE_RANGE( IEVE ) && EveStack.lst[IEVE].loaded )
#define INVALID_EVE( IEVE )     ( !VALID_EVE_RANGE( IEVE ) || !EveStack.lst[IEVE].loaded )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Enchantment variables

#define MAX_ENC                      200         // Number of enchantments

struct s_enc
{
    EGO_OBJECT_STUFF

    Sint16  time;                    // Time before end
    Uint16  spawntime;               // Time before spawn

    Uint16  eve;                     // The type

    Uint16  target;                  // Who it enchants
    Uint16  owner;                   // Who cast the enchant
    Uint16  spawner;                 // The spellbook character
	Uint16  spawnermodel;			 // The spellbook character's CapList index
    Uint16  overlay;                 // The overlay character
    Sint16  ownermana;               // Boost values
    Sint16  ownerlife;
    Sint16  targetmana;
    Sint16  targetlife;

    Uint16  nextenchant;             // Next in the list

    bool_t  setyesno[MAX_ENCHANT_SET];// Was it set?
    bool_t  setsave[MAX_ENCHANT_SET]; // The value to restore
    Sint16  addsave[MAX_ENCHANT_ADD]; // The value to take away
};
typedef struct s_enc enc_t;

DEFINE_LIST( extern, enc_t, EncList, MAX_ENC );

#define VALID_ENC_RANGE( IENC ) ( ((IENC) >= 0) && ((IENC) < MAX_ENC) )
#define VALID_ENC( IENC )       ( VALID_ENC_RANGE( IENC ) && EncList.lst[IENC].on )
#define INVALID_ENC( IENC )     ( !VALID_ENC_RANGE( IENC ) || !EncList.lst[IENC].on )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Prototypes
void EncList_free_all();
void getadd( int min, int value, int max, int* valuetoadd );
void fgetadd( float min, float value, float max, float* valuetoadd );
Uint16 EncList_get_free();
Uint16 enchant_value_filled( Uint16 enchantindex, Uint8 valueindex );
bool_t remove_enchant( Uint16 enchantindex );
void set_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
void add_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
Uint16 spawn_enchant( Uint16 owner, Uint16 target,
                      Uint16 spawner, Uint16 enchantindex, Uint16 modeloptional );
bool_t load_one_enchant_profile( const char* szLoadName, Uint16 profile );
void unset_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void remove_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void disenchant_character( Uint16 cnt );

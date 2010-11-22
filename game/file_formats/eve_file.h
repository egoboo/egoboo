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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file file_formats/eve_file.h
/// @details data and functions for reading and writing enchant.txt files

#include "egoboo_typedef.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Special modes for particle reflections from characters
    enum e_missle_treatment
    {
        MISSILE_NORMAL   = 0,                           ///< Treat missiles normally
        MISSILE_DEFLECT,                                ///< Deflect incoming missiles
        MISSILE_REFLECT                                 ///< Reflect them back!
    };

//--------------------------------------------------------------------------------------------

/// All the values that an enchant can override
    enum e_enchant_set
    {
        SETMORPH = 0,           ///< Morph character?
        ///< @details this must be first since the
        ///< character must be morphed before adding any of the other enchants

        SETDAMAGETYPE,          ///< Type of damage dealt
        SETNUMBEROFJUMPS,       ///< Max number of jumps
        SETLIFEBARCOLOR,        ///< Color of life bar
        SETMANABARCOLOR,        ///< Color of mana bar
        SETSLASHMODIFIER,       ///< Damage modifiers
        SETCRUSHMODIFIER,
        SETPOKEMODIFIER,
        SETHOLYMODIFIER,
        SETEVILMODIFIER,
        SETFIREMODIFIER,
        SETICEMODIFIER,
        SETZAPMODIFIER,
        SETFLASHINGAND,             ///< Flash rate
        SETLIGHTBLEND,              ///< Transparency
        SETALPHABLEND,              ///< Alpha
        SETSHEEN,                   ///< Shinyness
        SETFLYTOHEIGHT,             ///< Fly to this height
        SETWALKONWATER,             ///< Walk on water?
        SETCANSEEINVISIBLE,         ///< Can it see invisible?
        SETMISSILETREATMENT,        ///< How to treat missiles
        SETCOSTFOREACHMISSILE,      ///< Cost for each missile treat
        SETCHANNEL,                 ///< Can channel life as mana?
        MAX_ENCHANT_SET,

        ENC_SET_FIRST = SETMORPH,
        ENC_SET_LAST  = SETCHANNEL

    };

    typedef enum e_enchant_set enum_enchant_set;

//--------------------------------------------------------------------------------------------

/// A list of all the variables that can be affested by rnchant add
    enum e_enchant_add
    {
        ADDJUMPPOWER = 0,
        ADDBUMPDAMPEN,
        ADDBOUNCINESS,
        ADDDAMAGE,
        ADDSIZE,
        ADDACCEL,
        ADDRED,                        ///< Red shift
        ADDGRN,                        ///< Green shift
        ADDBLU,                        ///< Blue shift
        ADDDEFENSE,                    ///< Defence adjustments
        ADDMANA,
        ADDLIFE,
        ADDSTRENGTH,
        ADDWISDOM,
        ADDINTELLIGENCE,
        ADDDEXTERITY,
        MAX_ENCHANT_ADD,

        ENC_ADD_FIRST = ADDJUMPPOWER,
        ENC_ADD_LAST  = ADDDEXTERITY

    };

    typedef enum e_enchant_add enum_enchant_add;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// An enchantment profile, or "eve"
/// @details An internal representation of the "enchant.txt" file
    struct s_eve
    {
        EGO_PROFILE_STUFF

        // enchant spawning info
        bool_t  override;                         ///< Override other enchants?
        bool_t  remove_overridden;                ///< Remove other enchants?
        bool_t  retarget;                         ///< Pick a weapon?
        Uint8   required_damagetype;              ///< Don't enchant if the target is immune to required_damagetype
        Uint8   require_damagetarget_damagetype;  ///< Only enchant the target if the target damagetarget_damagetype matches this value
        bool_t  spawn_overlay;                    ///< Spawn an overlay?

        // ending conditions
        int     lifetime;                         ///< Time in seconds
        bool_t  endifcantpay;                     ///< End on out of mana
        IDSZ    removedbyidsz;                    ///< By particle or [NONE]

        // despawning info
        bool_t  stayiftargetdead;                 ///< Stay if target has died?
        bool_t  stayifnoowner;                    ///< Stay if owner has died?

        // skill modifications
        Sint16  owner_mana;
        Sint16  owner_life;
        Sint16  target_mana;
        Sint16  target_life;

        // generic modifications
        bool_t  setyesno[MAX_ENCHANT_SET];    ///< Set this value?
        float   setvalue[MAX_ENCHANT_SET];    ///< Value to use

        bool_t  addyesno[MAX_ENCHANT_ADD];    ///< Add this value?
        float   addvalue[MAX_ENCHANT_ADD];    ///< The values to add

        // special modifications
        int  seekurse;                        ///< Allow target to see kurses
        int  darkvision;                      ///< Allow target to see in darkness

        // continuous spawning
        Uint16  contspawn_delay;              ///< Spawn timer
        Uint8   contspawn_amount;             ///< Spawn amount
        Uint16  contspawn_facingadd;          ///< Spawn in circle
        int     contspawn_lpip;               ///< Spawn type ( local )

        // what to so when the enchant ends
        Sint16  endsound_index;              ///< Sound on end (-1 for none)
        bool_t  killtargetonend;             ///< Kill the target on end?
        bool_t  poofonend;                   ///< Spawn a poof on end?
        int     endmessage;                  ///< Message for end -1 for none

    };

    typedef struct s_eve eve_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    eve_t *  load_one_enchant_file_vfs( const char* szLoadName, eve_t * peve );
    bool_t   save_one_enchant_file_vfs( const char* szLoadName, const char * szTemplateName, eve_t * peve );

    eve_t * eve_init( eve_t * peve );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _eve_file_h
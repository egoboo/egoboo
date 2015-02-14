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

#pragma once
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#include "egolib/typedef.h"

/// Special modes for particle reflections from characters
enum e_missle_treatment
{
    MISSILE_NORMAL = 0,                           ///< Treat missiles normally
    MISSILE_DEFLECT,                                ///< Deflect incoming missiles
    MISSILE_REFLECT                                 ///< Reflect them back!
};

/// All the values that an enchant can override.
enum enum_enchant_set
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
    ENC_SET_LAST = SETCHANNEL

};

/// A list of all the variables that can be affested by enchant add.
enum enum_enchant_add
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

    ADDSLASHRESIST,
    ADDCRUSHRESIST,
    ADDPOKERESIST,
    ADDEVILRESIST,
    ADDHOLYRESIST,
    ADDFIRERESIST,
    ADDICERESIST,
    ADDZAPRESIST,

    MAX_ENCHANT_ADD,

    //these are only for parsing the enchant file
    ENC_ADD_FIRST = ADDJUMPPOWER,
    ENC_ADD_LAST = ADDZAPRESIST

};

/**
* @brief
*  An enchantment profile, or "eve"
* @details
*  An internal representation of the "enchant.txt" file.
*/
struct eve_t
{
    EGO_PROFILE_STUFF

    // enchant spawning info
    bool override;                         ///< Override other enchants?
    bool remove_overridden;                ///< Remove other enchants?
    bool retarget;                         ///< Pick a weapon?
    Uint8 required_damagetype;             ///< Don't enchant if the target is immune to required_damagetype
    Uint8 require_damagetarget_damagetype; ///< Only enchant the target if the target damagetarget_damagetype matches this value
    bool  spawn_overlay;                   ///< Spawn an overlay?

    // ending conditions
    int lifetime;                           ///< Time in seconds
    bool endifcantpay;                      ///< End on out of mana
    IDSZ removedbyidsz;                     ///< By particle or [NONE]

    // despawning info
    bool stayiftargetdead;                  ///< Stay if target has died?
    bool stayifnoowner;                     ///< Stay if owner has died?

    // skill modifications
    Sint16 owner_mana;
    Sint16 owner_life;
    Sint16 target_mana;
    Sint16 target_life;

    // generic modifications
    bool setyesno[MAX_ENCHANT_SET];      ///< Set this value?
    float setvalue[MAX_ENCHANT_SET];     ///< Value to use

    bool addyesno[MAX_ENCHANT_ADD];      ///< Add this value?
    float addvalue[MAX_ENCHANT_ADD];     ///< The values to add

    // special modifications
    int seekurse;                        ///< Allow target to see kurses
    int darkvision;                      ///< Allow target to see in darkness

    // continuous spawning
    Uint16 contspawn_delay;              ///< Spawn timer
    Uint8 contspawn_amount;              ///< Spawn amount
    Uint16 contspawn_facingadd;          ///< Spawn in circle
    int contspawn_lpip;                  ///< Spawn type ( local )

    // what to so when the enchant ends
    Sint16 endsound_index;               ///< Sound on end (-1 for none)
    bool killtargetonend;                ///< Kill the target on end?
    bool poofonend;                      ///< Spawn a poof on end?
    int endmessage;                      ///< Message for end -1 for none

    /**
    * @brief
    *  Initialize an enchant profile with safe default values.
    * @param self
    *  the enchant profile
    * @return
    *  a pointer to the profile on success, @a nullptr on failure
    */
    static eve_t *init(eve_t *self);
};

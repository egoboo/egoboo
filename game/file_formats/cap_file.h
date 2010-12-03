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

/// @file file_formats/cap_file.h
/// @details routines for reading and writing the character profile file data.txt

#include "egoboo_typedef.h"
#include "IDSZ_map.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define MAXCAPNAMESIZE      32                      ///< Character class names

//Levels
#define MAXBASELEVEL            6                 ///< Basic Levels 0-5
#define MAXLEVEL               20                 ///< Absolute max level

#define GRIP_VERTS             4

#define CAP_INFINITE_WEIGHT   0xFF
#define CAP_MAX_WEIGHT        0xFE

/// The various ID strings that every character has
    enum e_idsz_type
    {
        IDSZ_PARENT = 0,                             ///< Parent index
        IDSZ_TYPE,                                   ///< Self index
        IDSZ_SKILL,                                  ///< Skill index
        IDSZ_SPECIAL,                                ///< Special index
        IDSZ_HATE,                                   ///< Hate index
        IDSZ_VULNERABILITY,                          ///< Vulnerability index
        IDSZ_COUNT                                   ///< ID strings per character
    };

/// The possible damage types
    enum e_damage_type
    {
        DAMAGE_SLASH = 0,
        DAMAGE_CRUSH,
        DAMAGE_POKE,
        DAMAGE_HOLY,                             ///< (Most invert Holy damage )
        DAMAGE_EVIL,
        DAMAGE_FIRE,
        DAMAGE_ICE,
        DAMAGE_ZAP,
        DAMAGE_COUNT,

        DAMAGE_NONE      = 255
    };

/// A list of the possible special experience types
    enum e_xp_type
    {
        XP_FINDSECRET = 0,                          ///< Finding a secret
        XP_WINQUEST,                                ///< Beating a module or a subquest
        XP_USEDUNKOWN,                              ///< Used an unknown item
        XP_KILLENEMY,                               ///< Killed an enemy
        XP_KILLSLEEPY,                              ///< Killed a sleeping enemy
        XP_KILLHATED,                               ///< Killed a hated enemy
        XP_TEAMKILL,                                ///< Team has killed an enemy
        XP_TALKGOOD,                                ///< Talk good, er...  I mean well
        XP_COUNT,                                   ///< Number of ways to get experience

        XP_DIRECT     = 255                         ///< No modification
    };
    typedef enum e_xp_type xp_type;

/// BB@> enumerated "speech" soun
/// @details We COULD ge the scripts to classify which
/// sound to use for the "ouch", the "too much baggage", etc.
/// also some left-over sounds from the RTS days, but they might be useful if an NPC
/// uses messages to control his minions.
///
/// for example:
/// necromancer sends message to all minions "attack blah"
/// zombie minion responds with "moooooaaaaannn" automatically because that is the sound
/// registered as his SPEECH_ATTACK sound.
/// It seems to have a lot of potential to me. It *could* be done completely in the scripts,
/// but the idea of having registered sounds for certain actions makes a lot of sense to me! :)

    enum e_sound_types
    {
        SOUND_FOOTFALL = 0,
        SOUND_JUMP,
        SOUND_SPAWN,
        SOUND_DEATH,

        /// old "RTS" stuff
        SPEECH_MOVE,
        SPEECH_MOVEALT,
        SPEECH_ATTACK,
        SPEECH_ASSIST,
        SPEECH_TERRAIN,
        SPEECH_SELECT,

        SOUND_COUNT,

        SPEECH_BEGIN = SPEECH_MOVE,
        SPEECH_END   = SPEECH_SELECT
    };

/// Where an item is being held
    enum e_slots
    {
        SLOT_LEFT  = 0,
        SLOT_RIGHT,
        SLOT_COUNT
    };
    typedef enum e_slots slot_t;

/// The possible extended slots that an object might be equipped in
/// @details This system is not fully implemented yet
    enum e_inventory
    {
        INVEN_PACK = 0,
        INVEN_NECK,
        INVEN_WRIS,
        INVEN_FOOT,
        INVEN_COUNT
    };
    typedef enum e_inventory inventory_t;

/// What gender a character can be spawned with
    enum e_chr_gender
    {
        GENDER_FEMALE = 0,
        GENDER_MALE,
        GENDER_OTHER,
        GENDER_RANDOM,
        GENDER_COUNT
    };

#define ULTRABLUDY           2          ///< This makes any damage draw blud

//Damage shifts
#define DAMAGEINVICTUS      (1 << 5)                      ///< 00x00000 Invictus to this type of damage
#define DAMAGEMANA          (1 << 4)                      ///< 000x0000 Deals damage to mana
#define DAMAGECHARGE        (1 << 3)                       ///< 0000x000 Converts damage to mana
#define DAMAGEINVERT        (1 << 2)                       ///< 00000x00 Makes damage heal
#define DAMAGESHIFT         3                       ///< 000000xx Resistance ( 1 is common )

#define GET_DAMAGE_RESIST(BITS) ( (BITS) & DAMAGESHIFT )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The character statistic data in the form used in data.txt
    struct s_cap_stat
    {
        FRange val;
        FRange perlevel;
    };
    typedef struct s_cap_stat cap_stat_t;

//--------------------------------------------------------------------------------------------

/// The character profile data, or cap
/// The internal representation of the information in data.txt
    struct s_cap
    {
        EGO_PROFILE_STUFF

        // naming
        char         classname[MAXCAPNAMESIZE];            ///< Class name

        // skins
        char         skinname[MAX_SKIN][MAXCAPNAMESIZE];   ///< Skin name
        Uint16       skincost[MAX_SKIN];                   ///< Store prices
        float        maxaccel[MAX_SKIN];                   ///< Acceleration for each skin
        Uint8        skindressy;                           ///< Bits to tell whether the skins are "dressy"

        // overrides
        int          skin_override;                  ///< -1 or 0-3.. For import
        Uint8        level_override;                 ///< 0 for normal
        int          state_override;                 ///< 0 for normal
        int          content_override;               ///< 0 for normal

        IDSZ         idsz[IDSZ_COUNT];              ///< ID strings

        // inventory
        Uint8        ammomax;                       ///< Ammo stuff
        Uint8        ammo;
        Sint16       money;                         ///< Money

        // characer stats
        Uint8        gender;                        ///< Gender

        // life
        cap_stat_t   life_stat;                     ///< Life statistics
        UFP8_T       life_return;                    ///< Life regeneration
        UFP8_T       life_heal;
        UFP8_T       life_spawn;                     ///< Life left from last module

        // mana
        cap_stat_t   mana_stat;                     ///< Mana statistics
        cap_stat_t   manareturn_stat;               ///< Mana regeneration statistics
        cap_stat_t   manaflow_stat;                 ///< Mana channeling
        Uint16       mana_spawn;                      ///< Life left from last module

        cap_stat_t   strength_stat;                 ///< Strength
        cap_stat_t   wisdom_stat;                   ///< Wisdom
        cap_stat_t   intelligence_stat;             ///< Intlligence
        cap_stat_t   dexterity_stat;                ///< Dexterity

        // physics
        Uint8        weight;                        ///< Weight
        float        dampen;                        ///< Bounciness
        float        bumpdampen;                    ///< Mass

        float        size;                          ///< Scale of model
        float        size_perlevel;                  ///< Scale increases
        Uint32       shadow_size;                    ///< Shadow size
        Uint32       bump_size;                     ///< Bounding octagon
        bool_t       bump_override_size;            ///< let bump_size override the measured object size
        Uint32       bump_sizebig;                  ///< For octagonal bumpers
        bool_t       bump_override_sizebig;         ///< let bump_sizebig override the measured object size
        Uint32       bump_height;                   ///< the height of the object
        bool_t       bump_override_height;          ///< let bump_height overrride the measured height of the object
        Uint8        stoppedby;                     ///< Collision Mask

        // movement
        float        jump;                          ///< Jump power
        Uint8        jumpnumber;                    ///< Number of jumps ( Ninja )
        float        anim_speed_sneak;              ///< Sneak threshold
        float        anim_speed_walk;               ///< Walk threshold
        float        anim_speed_run;                ///< Run threshold
        Uint8        flyheight;                     ///< Fly height
        bool_t       waterwalk;                     ///< Walk on water?

        // status graphics
        Uint8        lifecolor;                     ///< Life bar color
        Uint8        manacolor;                     ///< Mana bar color
        bool_t       draw_icon;                     ///< Draw icon

        // model graphics
        Uint8        flashand;                      ///< Flashing rate
        Uint8        alpha;                         ///< Transparency
        Uint8        light;                         ///< Light blending
        bool_t       transferblend;                 ///< Transfer blending to rider/weapons
        Uint8        sheen;                         ///< How shiny it is ( 0-15 )
        bool_t       enviro;                        ///< Phong map this baby?
        SFP8_T       uoffvel;                       ///< "horizontal" texture movement rate
        SFP8_T       voffvel;                       ///< "vertical" texture movement rate
        bool_t       uniformlit;                    ///< Bad lighting?
        bool_t       reflect;                       ///< Draw the reflection
        bool_t       alwaysdraw;                    ///< Always render
        bool_t       forceshadow;                   ///< Draw a shadow?
        bool_t       ripple;                        ///< Spawn ripples?
        bool_t       dont_cull_backfaces;           ///< Force the drawing of backfaces?
        bool_t       skin_has_transparency;         ///< The skin has transparent areas

        // attack blocking info
        Uint16       iframefacing;                  ///< Invincibility frame
        Uint16       iframeangle;
        Uint16       nframefacing;                  ///< Normal frame
        Uint16       nframeangle;

        // defense
        bool_t       resistbumpspawn;                        ///< Don't catch fire
        Uint8        defense[MAX_SKIN];                      ///< Defense for each skin
        Uint8        damage_modifier[DAMAGE_COUNT][MAX_SKIN];

        // xp
        Uint32       experience_forlevel[MAXLEVEL];  ///< Experience needed for next level
        FRange       experience;                     ///< Starting experience
        Uint16       experience_worth;               ///< Amount given to killer/user
        float        experience_exchange;            ///< Adds to worth
        float        experience_rate[XP_COUNT];

        // sound
        Sint8        sound_index[SOUND_COUNT];       ///< a map for soundX.wav to sound types

        // flags
        bool_t       isequipment;                    ///< Behave in silly ways
        bool_t       isitem;                         ///< Is it an item?
        bool_t       ismount;                        ///< Can you ride it?
        bool_t       isstackable;                    ///< Is it arrowlike?
        bool_t       invictus;                       ///< Is it invincible?
        bool_t       platform;                       ///< Can be stood on?
        bool_t       canuseplatforms;                ///< Can use platforms?
        bool_t       cangrabmoney;                   ///< Collect money?
        bool_t       canopenstuff;                   ///< Open chests/doors?
        bool_t       canbedazed;                     ///< Can it be dazed?
        bool_t       canbegrogged;                   ///< Can it be grogged?
        bool_t       istoobig;                       ///< Can't be put in pack
        bool_t       isranged;                       ///< Flag for ranged weapon
        bool_t       nameknown;                      ///< Is the class name known?
        bool_t       usageknown;                     ///< Is its usage known
        bool_t       cancarrytonextmodule;           ///< Take it with you?
        Uint8        damagetarget_damagetype;               ///< For AI DamageTarget
        bool_t       slotvalid[SLOT_COUNT];          ///< Left/Right hands valid
        bool_t       ridercanattack;                 ///< Rider attack?
        Uint8        kursechance;                    ///< Chance of being kursed
        Sint8        hidestate;                      ///< Don't draw when...
        Sint8        isvaluable;                     ///< Force to be valuable
        int          spelleffect_type;               ///< is the object that a spellbook generates

        // item usage
        bool_t       needskillidtouse;               ///< Check IDSZ first?
        Uint8        weaponaction;                   ///< Animation needed to swing
        Sint16       manacost;                       ///< How much mana to use this object?
        Uint8        attack_attached;                ///< Do we have attack particles?
        int          attack_lpip;                    ///< What kind of attack particles?
        bool_t       attack_fast;                    ///< Ignores the default reload time?

        float        str_bonus;                      ///< Strength     damage factor
        float        wis_bonus;                      ///< Wisdom       damage factor
        float        int_bonus;                      ///< Intelligence damage factor
        float        dex_bonus;                      ///< dexterity    damage factor

        // special particle effects
        Uint8        attachedprt_amount;              ///< Number of sticky particles
        Uint8        attachedprt_reaffirm_damagetype; ///< Re-attach sticky particles? Relight that torch...
        int          attachedprt_lpip;                ///< Which kind of sticky particle

        Uint8        gopoofprt_amount;                ///< Amount of poof particles
        Sint16       gopoofprt_facingadd;             ///< Angular spread of poof particles
        int          gopoofprt_lpip;                  ///< Which poof particle

        Uint8        blud_valid;                      ///< Has blud? ( yuck )
        int          blud_lpip;                       ///< What kind of blud?

        // skill system
        IDSZ_node_t  skills[MAX_IDSZ_MAP_SIZE];
        int          see_invisible_level;             ///< Can it see invisible?

        // random stuff
        bool_t       stickybutt;                    ///< Stick to the ground?
    };

    typedef struct s_cap cap_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    cap_t * load_one_cap_file_vfs( const char * tmploadname, cap_t * pcap );
    bool_t  save_one_cap_file_vfs( const char * szSaveName, const char * szTemplateFile, cap_t * pcap );

    cap_t * cap_init( cap_t * pcap );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _cap_file_h
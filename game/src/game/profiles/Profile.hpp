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

/// @file game/Profile.hpp
#pragma once

#include <unordered_map>

#include "game/script.h"     //for script_info_t
#include "game/mad.h"
#include "game/profiles/RandomName.hpp"
#include "game/egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Forward declarations
typedef int SoundID;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Constants

#define MAX_SKIN             4               ///< The maxumum number of skins per model. This must remain hard coded at 4 for the moment.
#define NO_SKIN_OVERRIDE    -1                      ///< For import
#define NOHIDE              127                        ///< Don't hide

/// Stats
#define LOWSTAT             UINT_TO_UFP8(  1)     ///< Worst...
#define PERFECTSTAT         UINT_TO_UFP8( 60)     ///< Maximum stat without magic effects
#define PERFECTBIG          UINT_TO_UFP8(100)     ///< Perfect life or mana...
#define HIGHSTAT            UINT_TO_UFP8(100)     ///< Absolute max adding enchantments as well

//Levels
#define MAXBASELEVEL            6                 ///< Basic Levels 0-5
#define MAXLEVEL               20                 ///< Absolute max level

#define GRIP_VERTS             4

#define CAP_INFINITE_WEIGHT   0xFF
#define CAP_MAX_WEIGHT        0xFE

#define ULTRABLUDY           2          ///< This makes any damage draw blud

//Damage shifts
#define DAMAGEINVICTUS      (1 << 5)                      ///< 00x00000 Invictus to this type of damage
#define DAMAGEMANA          (1 << 4)                      ///< 000x0000 Deals damage to mana
#define DAMAGECHARGE        (1 << 3)                       ///< 0000x000 Converts damage to mana
#define DAMAGEINVERT        (1 << 2)                       ///< 00000x00 Makes damage heal


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Enum constants

/// @author BB
/// @details enumerated "speech" sounds
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
/*enum ProfileSoundTypes : uint8_t
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
};*/


/// What gender a character can be spawned with
enum CharacterGender : uint8_t
{
    GENDER_FEMALE = 0,
    GENDER_MALE,
    GENDER_OTHER,
    GENDER_RANDOM,
    GENDER_COUNT
};

/// The various ID strings that every character has
enum IDSZTypes : uint8_t
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
enum DamageType : uint8_t
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

#define DAMAGE_IS_PHYSICAL( TYPE )  (TYPE < DAMAGE_HOLY)    //Damage types slash, crush or poke are physical

/// A list of the possible special experience types
enum XPType : uint8_t
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


/// Where an item is being held
enum slot_t : uint8_t
{
    SLOT_LEFT  = 0,
    SLOT_RIGHT,
    SLOT_COUNT
};

/// The possible extended slots that an object might be equipped in
/// @details This system is not fully implemented yet
enum inventory_t : uint8_t
{
    INVEN_PACK = 0,
    INVEN_NECK,
    INVEN_WRIS,
    INVEN_FOOT,
    INVEN_COUNT
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Data structures

/// The character statistic data in the form used in data.txt
struct ProfileStat
 {
    FRange val;    
    FRange perlevel;
};

struct SkinInfo
{
    std::string  name;               ///< Skin name
    uint16_t     cost;               ///< Store prices
    float        maxaccel;           ///< Acceleration for each skin
    uint8_t      dressy;             ///< Bits to tell whether the skins are "dressy"
    uint8_t      defence;            ///< Damage reduction
    uint8_t      damageModifier;     ///< Invictus, inverse, mana burn etc.
    uint8_t      damageResistance;   ///< Shift damage
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a wrapper for all the datafiles in the *.obj dir
class ObjectProfile
{
public:
    /**
    * @brief Loads and parses a profile folder and builds a profile object from it.
    **/
    ObjectProfile(const std::string &folderPath, size_t slotNumber);

    /**
    * @brief Deconstructor
    **/
    ~ObjectProfile();

    /// @author ZF
    /// @details This adds one string to the list of messages associated with a profile. The function will
    //              dynamically allocate more memory if there are more messages than array size
    //  @param filterDuplicates don't add it if it already exists
    void addMessage(const std::string &message, const bool filterDuplicates = false);

    /**
    * @return a string loaded into the specified index, or an empty string if the index is not valid
    **/
    const std::string& getMessage(size_t index) const;

    SoundID getSoundID(int index) const;

    IDSZ getIDSZ(size_t type) const;

    inline bool isValidMessageID(int id) const {return id >= 0 && id < _messageList.size();}

    /**
    * @details use the profile's RandomName to generate a new random name. 
    * @return A new string instance containing the name. Return "*NONE*" on a failure or the Class name
    *         if no random names have been loaded for this profile.
    **/
    const std::string generateRandomName();

    /**
    * @return get the skin of the specified index number (or skin 0 if index is invalid)
    **/
    TX_REF getSkin(size_t index);

    /**
    * @return get the icon of the specified index number (or icon 0 if index is invalid)
    **/
    TX_REF getIcon(size_t index);

    inline const std::unordered_map<size_t, TX_REF> &getAllIcons() const {return _iconsLoaded;}

    inline const std::string& getFileName() const {return _fileName;}

    inline MAD_REF getModelRef() const {return _imad;}
    inline EVE_REF getEnchantRef() const {return _ieve;}

    /**
    * @return get which slot number this profile is loaded with
    **/
    inline int getSlotNumber() const {return _slotNumber;}

    /**
    * Gets the particle profile loaded into the specified index number
    **/
    PIP_REF getParticleProfile(size_t index) const;

    /**
    * Write access getter
    **/
    inline script_info_t& getAIScript() {return _aiScript;}
    inline RandomName& getRandomNameData() {return _randomName;}

    /**
    * @brief Exports profile info to a data file
    **/
    static bool saveToFile(const std::string &filePath, const std::string &templateName);

    uint16_t getSkinOverride() const;

    const std::string& getClassName() const {return _className};

    const SkinInfo& getSkinInfo(size_t index) const;

private:
    /**
    * Private default constructor
    **/
    ObjectProfile();

    /**
    * @brief Loads the md2 model for this profile
    **/
    //void loadModel(const std::string &filePath);

    /**
    * @brief Parses message.txt and loads all messages from it
    **/
    void loadAllMessages(const std::string &filePath);

    /**
    * @brief load all tris*.bmp or tris*.png skin and icons
    **/
    void loadTextures(const std::string &folderPath);

    /**
    * @brief Loads profile data from a datafile (data.txt)
    **/
    void loadDataFile(const std::string &filePath);

    bool exportToDataFile(const std::string &filePath);

private:
    std::string _fileName;                      ///< Usually the source filename
    size_t _requestCount;                       ///< the number of attempted spawns
    size_t _spawnCount;                         ///< the number of successful spawns

    // the sub-profiles
    MAD_REF _imad;                             ///< the md2 model for this profile
    EVE_REF _ieve;                             ///< the enchant profile for this profile
    int _slotNumber;

    /// the random naming info
    RandomName _randomName;
    
    script_info_t _aiScript;                    ///< the AI script for this profile

    //Particles
    std::array<PIP_REF, MAX_PIP_PER_PROFILE> _particleProfiles;

    // the profile skins
    std::unordered_map<size_t, TX_REF> _texturesLoaded;
    std::unordered_map<size_t, TX_REF> _iconsLoaded;

    // the profile message info
    std::vector<std::string> _messageList;   ///< Dynamic array of messages

    // sounds
    std::unordered_map<size_t, SoundID> _soundMap;  ///< sounds in a object    

    //---------------------------------------------------------------
    // stuff from data.txt
    //---------------------------------------------------------------

    // naming
    std::string  _className;                     //< Class name

    // skins
    std::unordered_map<size_t, SkinInfo>  _skinInfo;

    // overrides
    int          _skinOverride;                  ///< -1 or 0-3.. For import
    uint8_t      _levelOverride;                 ///< 0 for normal
    int          _stateOverride;                 ///< 0 for normal
    int          _contentOverride;               ///< 0 for normal

    std::array<IDSZ, IDSZ_COUNT> _idsz;          ///< ID strings

    // inventory
    uint16_t       _maxAmmo;                       ///< Ammo stuff
    uint16_t       _ammo;
    int16_t        _money;                         ///< Money

    // characer stats
    CharacterGender _gender;                    ///< Gender

    // life
    ProfileStat  life_stat;                     ///< Life statistics. Base range/current value + by-level bonus.
    UFP8_T       life_return;                   ///< Life regeneration (8.8 fixed point). @todo Should be a cap_stat too.
    UFP8_T       life_spawn;                    ///< Life left from last module (8.8 fixed point)

    // mana
    ProfileStat  mana_stat;                     ///< Mana statistics. Base range/current value + by-level bonus.
    ProfileStat  manareturn_stat;               ///< Mana regeneration statistics
    UFP8_T       mana_spawn;                    ///< Mana left from last module (8.8 fixed point)

    UFP8_T       life_heal;                     ///< (8.8 fixed point) @todo Find out what this is used for.
    ProfileStat  manaflow_stat;                 ///< Mana channeling   @todo Find out what this is used for.

    ProfileStat  strength_stat;                 ///< Strength.    Initial range or current value + per-level increase.
    ProfileStat  wisdom_stat;                   ///< Wisdom.      Initial range or current value + per-level increase.
    ProfileStat  intelligence_stat    ;         ///< Intlligence. Initial range or current value + per-level increase.
    ProfileStat  dexterity_stat;                ///< Dexterity.   Initial range or current value + per-level increase.

    // physics
    uint8_t      weight;                        ///< Weight
    float        dampen;                        ///< Bounciness
    float        bumpdampen;                    ///< Mass

    float        size;                          ///< Scale of model
    float        size_perlevel;                 ///< Scale increases
    Uint32       shadow_size;                   ///< Shadow size
    Uint32       bump_size;                     ///< Bounding octagon
    bool         bump_override_size;            ///< let bump_size override the measured object size
    Uint32       bump_sizebig;                  ///< For octagonal bumpers
    bool         bump_override_sizebig;         ///< let bump_sizebig override the measured object size
    Uint32       bump_height;                   ///< the height of the object
    bool         bump_override_height;          ///< let bump_height overrride the measured height of the object
    uint8_t      _stoppedBy;                    ///< Collision Mask

    // movement
    float        _jumpPower;                    ///< Jump power
    uint8_t      _jumpNumber;                   ///< Number of jumps ( Ninja )
    float        anim_speed_sneak;              ///< Sneak threshold
    float        anim_speed_walk;               ///< Walk threshold
    float        anim_speed_run;                ///< Run threshold
    uint8_t      _flyHeight;                    ///< Fly height
    bool         _waterWalking;                 ///< Walk on water?
    int          _jumpSound;
    int          _footFallSound

    // status graphics
    uint8_t      _lifeColor;                     ///< Life bar color
    uint8_t      _manaColor;                     ///< Mana bar color
    bool         _drawIcon;                      ///< Draw icon

    // model graphics
    uint8_t      _flashAND;                     ///< Flashing rate
    uint8_t      _alpha;                        ///< Transparency
    uint8_t      _light;                        ///< Light blending
    bool         _transferBlending;             ///< Transfer blending to rider/weapons
    uint8_t      _sheen;                        ///< How shiny it is ( 0-15 )
    bool         _phongMapping;                 ///< Phong map this baby?
    SFP8_T       uoffvel;                       ///< "horizontal" texture movement rate (8.8 fixed point)
    SFP8_T       voffvel;                       ///< "vertical" texture movement rate (8.8 fixed point)
    bool         _uniformLit;                   ///< Bad lighting?
    bool         _hasReflection;                ///< Draw the reflection
    bool         _alwaysDraw;                   ///< Always render
    bool         _forceShadow;                  ///< Draw a shadow?
    bool         _causesRipples;                ///< Spawn ripples?
    bool         _dontCullBackfaces;            ///< Force the drawing of backfaces?
    bool         _skinHasTransparency;          ///< The skin has transparent areas

    // attack blocking info
    uint16_t       iframefacing;                  ///< Invincibility frame
    uint16_t       iframeangle;
    uint16_t       nframefacing;                  ///< Normal frame
    uint16_t       nframeangle;

    // defense
    bool           _resistBumpSpawn;             ///< Don't catch fire

    // xp
    std::array<uint32_t, MAX_LEVEL> _experienceForLevel;  ///< Experience needed for next level
    FRange                          _startingExperience;  ///< Starting experience
    uint16_t                        _experienceWorth;     ///< Amount given to killer/user
    float                           _experienceExchange;  ///< Adds to worth
    std::array<float, XP_COUNT>     _experienceRate;

    // flags
    bool       _isEquipment;                   ///< Behave in silly ways
    bool       _isItem;                        ///< Is it an item?
    bool       _isMount;                       ///< Can you ride it?
    bool       _isStackable;                   ///< Is it arrowlike?
    bool       _isInvincible;                  ///< Is it invincible?
    bool       _isPlatform;                    ///< Can be stood on?
    bool       _canUsePlatforms;               ///< Can use platforms?
    bool       _canGrabMoney;                  ///< Collect money?
    bool       _canOpenStuff;                  ///< Open chests/doors?
    bool       canbedazed;                     ///< Can it be dazed?
    bool       canbegrogged;                   ///< Can it be grogged?
    bool       istoobig;                       ///< Can't be put in pack
    bool       isranged;                       ///< Flag for ranged weapon
    bool       nameknown;                      ///< Is the class name known?
    bool       usageknown;                     ///< Is its usage known
    bool       cancarrytonextmodule;           ///< Take it with you?
    uint8_t    damagetarget_damagetype;        ///< For AI DamageTarget
    bool       slotvalid[SLOT_COUNT];          ///< Left/Right hands valid
    bool       ridercanattack;                 ///< Rider attack?
    uint8_t    _kurseChance;                   ///< Chance of being kursed (0 to 100%)
    Sint8      _hideState;                     ///< Don't draw when...
    Sint8      _isValuable;                    ///< Force to be valuable
    int        _spellEffectType;               ///< is the object that a spellbook generates

    // item usage
    bool         needskillidtouse;               ///< Check IDSZ first?
    uint8_t      weaponaction;                   ///< Animation needed to swing
    int16_t      manacost;                       ///< How much mana to use this object?
    uint8_t      attack_attached;                ///< Do we have attack particles?
    int          attack_lpip;                    ///< What kind of attack particles?
    bool         attack_fast;                    ///< Ignores the default reload time?

    float        str_bonus;                      ///< Strength     damage factor
    float        wis_bonus;                      ///< Wisdom       damage factor
    float        int_bonus;                      ///< Intelligence damage factor
    float        dex_bonus;                      ///< dexterity    damage factor

    // special particle effects
    uint8_t      attachedprt_amount;              ///< Number of sticky particles
    uint8_t      attachedprt_reaffirm_damagetype; ///< Re-attach sticky particles? Relight that torch...
    int          attachedprt_lpip;                ///< Which kind of sticky particle

    uint8_t      gopoofprt_amount;                ///< Amount of poof particles
    int16_t      gopoofprt_facingadd;             ///< Angular spread of poof particles
    int          gopoofprt_lpip;                  ///< Which poof particle

    //Blood
    uint8_t      blud_valid;                      ///< Has blud? ( yuck )
    int          blud_lpip;                       ///< What kind of blud?

    // skill system
    std::unordered_set<IDSZ> _skills;             ///< Set of skills this character posesses
    int          see_invisible_level;             ///< Can it see invisible?

    // random stuff
    bool       stickybutt;                    ///< Stick to the ground?

};

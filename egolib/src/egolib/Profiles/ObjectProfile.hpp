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

/// @file egolib/Profiles/ObjectProfile.hpp
/// @author Johan Jansen

#pragma once
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#include "egolib/Script/script.h"
#include "egolib/Graphics/mad.h"
#include "egolib/Profiles/_Include.hpp"
#include "egolib/Logic/Gender.hpp"
#include "egolib/Logic/Attribute.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Forward declarations
typedef int SoundID;
class Object;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Constants
#define NOHIDE              127                        ///< Don't hide

/// Stats
#define LOWSTAT             UINT_TO_UFP8(  1)     ///< Worst...
#define PERFECTSTAT         UINT_TO_UFP8( 60)     ///< Maximum stat without magic effects
#define PERFECTBIG          UINT_TO_UFP8(100)     ///< Perfect life or mana...
#define HIGHSTAT            UINT_TO_UFP8(100)     ///< Absolute max adding enchantments as well

//Levels
#define MAXBASELEVEL            6                 ///< Basic Levels 0-5
#define MAXLEVEL               20                 ///< Absolute max level

#define CAP_INFINITE_WEIGHT   0xFF
#define CAP_MAX_WEIGHT        0xFE

#define ULTRABLUDY           2          ///< This makes any damage draw blud

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
struct SkinInfo
{
    std::string  name;                             ///< Skin name
    uint16_t     cost;                             ///< Store prices
    float        maxAccel;                         ///< Acceleration for each skin
    bool         dressy;                           ///< True if this is light armour
    uint8_t      defence;                          ///< Damage reduction
    uint8_t      damageModifier[DAMAGE_COUNT];     ///< Invictus, inverse, mana burn etc.
    float        damageResistance[DAMAGE_COUNT];   ///< Damage Resistance (can be negative)
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a wrapper for all the datafiles in the *.obj dir
class ObjectProfile
{
public:
    /**
    * Default constructor
    **/
    ObjectProfile();

    /**
    * @brief Deconstructor
    **/
    ~ObjectProfile();

    /// @author ZF
    /// @details This adds one string to the list of messages associated with a profile. The function will
    ///             dynamically allocate more memory if there are more messages than array size
    /// @param filterDuplicates don't add it if it already exists
    /// @return the index of the message added
    size_t addMessage(const std::string &message, const bool filterDuplicates = false);

    /**
    * @return a string loaded into the specified index, or an empty string if the index is not valid
    **/
    const std::string& getMessage(size_t index) const;

    SoundID getSoundID(int index) const;

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

    /**
    *@return the folder path where this profile was loaded
    **/
    inline const std::string& getPathname() const {
        return _pathname;
    }

    inline MAD_REF getModelRef() const {return _imad;}
    inline EVE_REF getEnchantRef() const {return _ieve;}

    /**
    * @return get which slot number this profile is loaded with
    **/
    inline int getSlotNumber() const {return _slotNumber;}

    /**
     * @brief
     *  Get the particle profile loaded for the specified local particle profile reference.
     * @return
     *  the particle profile if it exists, #INVALID_PIP_REF otherwise
     */
    PIP_REF getParticleProfile(const LocalParticleProfileRef& lppref) const;

    /**
    * Write access getter
    **/
    inline script_info_t& getAIScript() {return _aiScript;}
    inline RandomName& getRandomNameData() {return _randomName;}

    /**
    * @brief
    **/
    uint16_t getSkinOverride() const;

    /**
    * @brief
    *   Get a random ID for a valid skin for this ObjectProfile
    * @return
    *   A random Skin number for this Profile (always returns 0 if it has no valid skins)
    **/
    size_t getRandomSkinID() const;

    /**
    * @brief get the class name of this object profile (e.g Sword, Healer or Lumpkin)
    **/
    inline const std::string& getClassName() const {return _className;}

    /**
    * @brief
    **/
    const SkinInfo& getSkinInfo(size_t index) const;

    /**
    * @brief return true if this character is immune to all damage
    **/
    inline bool isInvincible() const {return _isInvincible;}

    /**
    * @return true if the specified grip slot is valid for this profile
    **/
    bool isSlotValid(slot_t slot) const;

    inline bool canCarryToNextModule() const {return _canCarryToNextModule;}

    inline bool hasResistBumpSpawn() const {return _resistBumpSpawn;}

    inline bool canBeDazed() const {return _canBeDazed;}

    inline bool canBeGrogged() const {return _canBeGrogged;}

    inline bool isBigItem() const {return _isBigItem;}
    inline bool isItem() const {return _isItem;}

    inline bool isUsageKnown() const {return _usageIsKnown;}

    inline bool isEquipment() const {return _isEquipment;}

    inline bool isStackable() const {return _isStackable;}

    inline PIP_REF getAttackParticleProfile() const {return getParticleProfile(_attackParticle);}

    inline bool spawnsAttackParticle() const {return _spawnsAttackParticle;}

    inline uint8_t getKurseChance() const {return _kurseChance;}

    inline uint8_t getAttachedParticleAmount() const {return _attachedParticleAmount;}

    inline PIP_REF getAttachedParticleProfile() const {return getParticleProfile(_attachedParticle);}

    inline bool causesRipples() const {return _causesRipples;}

    inline uint8_t getLight() const {return _light;}

    inline uint8_t getAlpha() const {return _alpha;}

    inline bool isPhongMapped() const {return _phongMapping;}

    inline uint8_t getSheen() const {return _sheen;}

    /**
    *@brief If this is true, then draw textures on the inside of characters as well
    **/
    inline bool isDontCullBackfaces() const {return _dontCullBackfaces;}


    inline float getSizeGainPerLevel() const {return _sizeGainPerLevel;}

    inline uint8_t getWeight() const {return _weight;}

    inline uint8_t getParticlePoofAmount() const {return _goPoofParticleAmount;}

    inline PIP_REF getParticlePoofProfile() const {return getParticleProfile(_goPoofParticle);}

    inline int16_t getParticlePoofFacingAdd() const {return _goPoofParticleFacingAdd;}

    inline bool isPlatform() const      {return _isPlatform;}

    inline bool canUsePlatforms() const {return _canUsePlatforms;}

    inline uint8_t getFlyHeight() const {return _flyHeight;}

    inline float getBumpDampen() const  {return _bumpDampen;}

    inline float getBounciness() const  {return _bounciness;}

    inline float getSize() const        {return _size;}
    inline float getShadowSize() const  {return _shadowSize;}
    inline float getBumpSize() const    {return _bumpSize;}
    inline float getBumpSizeBig() const {return _bumpSizeBig;}
    inline float getBumpHeight() const  {return _bumpHeight;}

    inline bool isMount() const {return _isMount;}
    inline bool riderCanAttack() const {return  _riderCanAttack;}

    inline uint8_t getFlashAND() const {return _flashAND;}

    inline UFP8_T getSpawnLife() const {return _spawnLife;}
    inline UFP8_T getSpawnMana() const {return _spawnMana;}

    inline uint16_t getStartingMoney() const {return _money;}

    inline FRange getStartingExperience() const {return _startingExperience;}

    inline uint8_t getStartingLevel() const {return _levelOverride;}

    /**
    * @brief how high can it jump?
    **/
    inline float getJumpPower() const  {return _jumpPower;}

    /**
    * @brief number of jumps character can do before hitting the ground
    **/
    inline uint8_t getJumpNumber() const  {return _jumpNumber;}

    inline uint8_t getStoppedByMask() const {return _stoppedBy;}

    inline uint16_t getMaxAmmo() const {return _maxAmmo;}

    inline uint16_t getAmmo() const {return _ammo;}

    inline CharacterGender getGender() const {return _gender;}

    inline bool hasStickyButt() const {return _stickyButt;}

    inline bool canOpenStuff() const {return _canOpenStuff;}

    inline bool canGrabMoney() const {return _canGrabMoney;}

    inline bool canSeeInvisible() const {return _seeInvisibleLevel > 0;}

    inline uint8_t getWeaponAction() const {return _weaponAction;}

    inline uint8_t requiresSkillIDToUse() const {return _needSkillIDToUse;}

    inline int getStateOverride() const {return _stateOverride;}
    inline int getContentOverride() const {return _contentOverride;}

    inline bool hasFastAttack() const {return  _attackFast;}

    inline int getSpellEffectType() const {return _spellEffectType;}

    inline bool isRangedWeapon() const {return  _isRanged;}

    inline bool isDrawIcon() const {return _drawIcon;}

    inline bool isNameKnown() const {return _nameIsKnown;}

    inline DamageType getDamageTargetType() const {return _damageTargetDamageType;}

    inline bool canWalkOnWater() const {return _waterWalking;}

    /**
    * @return hide this character (don't draw) if AI state is equal to this value (returns NOHIDE for never hide)
    **/
    inline int8_t getHideState() const {return _hideState;}

    /**
    * @return true if only use pchr->bump.size if it was overridden in data.txt through the [MODL] expansion
    **/
    inline bool getBumpOverrideSize() const {return _bumpOverrideSize;}

    /**
    * @return true if only use pchr->bump.height if it was overridden in data.txt through the [MODL] expansion
    **/
    inline bool getBumpOverrideHeight() const {return _bumpOverrideHeight;}
    
    /**
    * @return true if only use pchr->bump.size_big if it was overridden in data.txt through the [MODL] expansion
    **/
    inline bool getBumpOverrideSizeBig() const {return _bumpOverrideSizeBig;}

    /**
    * @return the SoundID for jumping when this object profile jumps
    **/
    inline SoundID getJumpSound() const {return getSoundID(_jumpSound);}

    /**
    * @return the SoundID for footfall effects for this object profile
    **/
    inline SoundID getFootFallSound() const {return getSoundID(_footFallSound);}

    /**
    * @return true if this object should transfer its blending effect upon any
    *         items it might be holding or its rider (if it is a mount)
    **/ 
    inline bool transferBlending() const {return _isEquipment;}

    /**
    * @brief If this character should spawn blood particles when it gets hurt
    **/ 
    inline uint8_t getBludType() const {return _bludValid;}

    /**
    * @brief If this character should spawn blood particles when it gets hurt
    **/ 
    inline PIP_REF getBludParticleProfile() const {return getParticleProfile(_bludParticle);}

    /**
    * @brief experience multiplies for a given kind of experience type
    *        e.g a Wizard learns more from reading books than a Soldier
    **/
    float getExperienceRate(XPType type) const;

    /**
    * @return how much xp this is worth
    **/
    inline uint16_t getExperienceValue() const {return _experienceWorth;}

    /**
    * @return how much of this characters xp is transferred upon it's killer
    **/
    inline float getExperienceExchangeRate() const {return _experienceExchange;}

    /**
    * @return the amount of experience needed to obtain a given character level
    **/
    uint32_t getXPNeededForLevel(uint8_t level) const;

    /**
    * @brief Damage bonus on attack particles from a given stat
    *        E.g if this object is a sword and has 0.5f strength factor, the user
    *        gets to add 50% of her strength to all attacks with it.
    *        Same goes for unarmed attacks if this object is actually a character
    **/
    inline float getStrengthDamageFactor() const      {return _strengthBonus;}
    inline float getWisdomDamageFactor() const        {return _wisdomBonus;}
    inline float getIntelligenceDamageFactor() const  {return _intelligenceBonus;}
    inline float getDexterityDamageFactor() const     {return _dexterityBonus;}

    inline const std::unordered_map<IDSZ, int>& getSkillMap() const {return _skills;}

    inline uint8_t getManaColor() const {return _manaColor;}

    inline uint8_t getLifeColor() const {return _lifeColor;}

    /**
    * @brief Moving textures effect
    **/
    inline SFP8_T getTextureMovementRateX() const {return _textureMovementRateX;}
    inline SFP8_T getTextureMovementRateY() const {return _textureMovementRateY;}
 
    inline float getSneakAnimationSpeed() const {return _animationSpeedSneak;}
    inline float getWalkAnimationSpeed() const {return _animationSpeedWalk;}
    inline float getRunAnimationSpeed() const {return _animationSpeedRun;}

    inline uint16_t getNormalFrameAngle() const {return nframeangle;}
    inline uint16_t getNormalFrameFacing() const {return nframefacing;}
    inline uint16_t getInvictusFrameAngle() const {return iframeangle;}
    inline uint16_t getInvictusFrameFacing() const {return iframefacing;}

    /**
    * @return How much mana the object spends when doing an unarmed attack (used for Healers and Paladins)
    **/
    inline float getUseManaCost() const {return _useManaCost*256.0f;}

    /**
    * @brief ZF> I'm not sure what this is. 
    *            Something to do with particles reaffirming if it gets hurt by this damage type
    **/
    inline DamageType getReaffirmDamageType() const {return _attachedParticleReaffirmDamageType;}

    /**
    * @author BB
    * @brief check IDSZ_PARENT and IDSZ_TYPE to see if the test_idsz matches. If we are not
    *        picky (i.e. IDSZ_NONE == idsz), then it matches any valid item.
    **/
    bool hasTypeIDSZ(const IDSZ idsz) const;

    /**
    * @author BB
    * @brief does idsz match any of the stored values in pcap->idsz[]?
    *        Matches anything if not picky (idsz == IDSZ_NONE)
    **/
    bool hasIDSZ(const IDSZ idsz) const;

    IDSZ getIDSZ(size_t type) const;

    /**
    * @brief makes the usage of this type of object known to all players
    **/
    void makeUsageKnown() {_usageIsKnown = true;}

    /**
    * @return
    *   Get the attribute increase each time character increases in experience level
    **/
    const FRange& getAttributeGain(Ego::Attribute::AttributeType type) const;

    /**
    * @return
    *   Get base starting attribute this Object spawns with (can be random range)
    **/
    const FRange& getAttributeBase(Ego::Attribute::AttributeType type) const;

    /**
    * @brief Loads a new ObjectProfile object by loading all data specified in the folder path
    * @param slotOverride Which slot number to load this profile in
    * @param lightWeight If true, then no 3D model, sounds, particle or enchant will be loaded (for menu)
    **/
    static std::shared_ptr<ObjectProfile> loadFromFile(const std::string &folderPath, const PRO_REF slotOverride, const bool lightWeight = false);

    /**
    * @brief Writes the contents of this character instance to a profile data.txt file
    **/
    static bool exportCharacterToFile(const std::string &filePath, const Object *character);

    //ZF> TODO: these should not be public
    size_t _spawnRequestCount;                       ///< the number of attempted spawns
    size_t _spawnCount;                         ///< the number of successful spawns

private:

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
    * @return true if it was successfully parsed and loaded
    **/
    bool loadDataFile(const std::string &filePath);

    /**
    * @author ZF
    * @details This calculates the xp needed to reach next level and stores it in an array for later use
    **/
    void setupXPTable();

private:
    std::string _pathname;                      ///< Usually the source filename

    // the sub-profiles
    MAD_REF _imad;                             ///< the md2 model for this profile
    EVE_REF _ieve;                             ///< the enchant profile for this profile
    int _slotNumber;

    /// the random naming info
    RandomName _randomName;
    
    script_info_t _aiScript;                    ///< the AI script for this profile

    //Particles
    std::unordered_map<LocalParticleProfileRef, PIP_REF> _particleProfiles;

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

    //for imports
    UFP8_T       _spawnLife;                    ///< Life left from last module (8.8 fixed point)
    UFP8_T       _spawnMana;                    ///< Mana left from last module (8.8 fixed point)

    std::array<FRange, Ego::Attribute::NR_OF_ATTRIBUTES> _baseAttribute; ///< Base attributes
    std::array<FRange, Ego::Attribute::NR_OF_ATTRIBUTES> _attributeGain; ///< Attribute increase each level

    // physics
    uint8_t      _weight;                        ///< Weight
    float        _bounciness;                        ///< Bounciness
    float        _bumpDampen;                    ///< Mass

    float        _size;                         ///< Scale of model
    float        _sizeGainPerLevel;             ///< Scale increases
    uint32_t     _shadowSize;                   ///< Shadow size
    uint32_t     _bumpSize;                     ///< Bounding octagon
    bool         _bumpOverrideSize;             ///< let bump_size override the measured object size
    uint32_t     _bumpSizeBig;                  ///< For octagonal bumpers
    bool         _bumpOverrideSizeBig;          ///< let bump_sizebig override the measured object size
    uint32_t     _bumpHeight;                   ///< the height of the object
    bool         _bumpOverrideHeight;           ///< let bump_height overrride the measured height of the object
    uint8_t      _stoppedBy;                    ///< Collision Mask

    // movement
    float        _jumpPower;                    ///< Jump power
    uint8_t      _jumpNumber;                   ///< Number of jumps ( Ninja )
    float        _animationSpeedSneak;          ///< Sneak threshold
    float        _animationSpeedWalk;           ///< Walk threshold
    float        _animationSpeedRun;            ///< Run threshold
    uint8_t      _flyHeight;                    ///< Fly height
    bool         _waterWalking;                 ///< Walk on water?
    int          _jumpSound;
    int          _footFallSound;

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
    SFP8_T       _textureMovementRateX;         ///< "horizontal" texture movement rate (8.8 fixed point)
    SFP8_T       _textureMovementRateY;         ///< "vertical" texture movement rate (8.8 fixed point)
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
    std::array<uint32_t, MAXLEVEL> _experienceForLevel;  ///< Experience needed for next level
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
    bool       _canBeDazed;                    ///< Can it be dazed?
    bool       _canBeGrogged;                  ///< Can it be grogged?
    bool       _isBigItem;                     ///< Can't be put in pack
    bool       _isRanged;                      ///< Flag for ranged weapon
    bool       _nameIsKnown;                   ///< Is the class name known?
    bool       _usageIsKnown;                  ///< Is its usage known
    bool       _canCarryToNextModule;          ///< Take it with you?
    DamageType _damageTargetDamageType;        ///< For AI DamageTarget
    std::array<bool, SLOT_COUNT> _slotsValid;  ///< Left/Right hands valid
    bool       _riderCanAttack;                ///< Rider attack?
    uint8_t    _kurseChance;                   ///< Chance of being kursed (0 to 100%)
    int8_t     _hideState;                     ///< Don't draw when...
    int8_t     _isValuable;                    ///< Force to be valuable
    int        _spellEffectType;               ///< is the object that a spellbook generates

    // item usage
    bool         _needSkillIDToUse;              ///< Check IDSZ first?
    uint8_t      _weaponAction;                  ///< Animation needed to swing
    bool         _spawnsAttackParticle;          ///< Do we have attack particles?
    LocalParticleProfileRef _attackParticle;     ///< What kind of attack particles?
    bool         _attackFast;                    ///< Ignores the default reload time?

    float        _strengthBonus;                      ///< Strength     damage factor
    float        _wisdomBonus;                      ///< Wisdom       damage factor
    float        _intelligenceBonus;                      ///< Intelligence damage factor
    float        _dexterityBonus;                      ///< dexterity    damage factor

    // special particle effects
    uint8_t _attachedParticleAmount;                  ///< Number of sticky particles
    DamageType _attachedParticleReaffirmDamageType;   ///< Re-attach sticky particles? Relight that torch...
    LocalParticleProfileRef _attachedParticle;        ///< Which kind of sticky particle

    uint8_t _goPoofParticleAmount;           ///< Amount of poof particles
    int16_t _goPoofParticleFacingAdd;        ///< Angular spread of poof particles
    LocalParticleProfileRef _goPoofParticle; ///< Which poof particle

    //Blood
    uint8_t _bludValid;                      ///< Has blud? ( yuck )
    LocalParticleProfileRef _bludParticle;   ///< What kind of blud?

    // skill system
    std::unordered_map<IDSZ, int> _skills;        ///< Set of skills this character posesses
    int          _seeInvisibleLevel;              ///< Can it see invisible?

    // random stuff
    bool       _stickyButt;                       ///< Stick to the ground? (conform to hills like chair)
    float      _useManaCost;                      ///< Mana usage for unarmed attack
};

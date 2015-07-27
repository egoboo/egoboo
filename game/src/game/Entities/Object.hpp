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

/// @file game/Entities/Object.hpp
/// @details An object representing instances of in-game egoboo objects (Object)
/// @author Johan Jansen

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/Entities/_Include.hpp` instead)
#endif

#include "game/egoboo_typedef.h"
#include "game/physics.h"
#include "egolib/Script/script.h"
#include "game/graphic_mad.h"
#include "game/Entities/Common.hpp"
#include "game/graphic_billboard.h"
#include "egolib/IDSZ_map.h"
#include "game/Module/Module.hpp"
#include "game/Inventory.hpp"

//Forward declarations
namespace Ego { class Enchantment; }

/// The possible methods for characters to determine what direction they are facing
enum turn_mode_t : uint8_t
{
    TURNMODE_VELOCITY = 0,                       ///< Character gets rotation from velocity (normal)
    TURNMODE_WATCH,                              ///< For watch towers, look towards waypoint
    TURNMODE_SPIN,                               ///< For spinning objects
    TURNMODE_WATCHTARGET,                        ///< For combat intensive AI
    TURNMODE_COUNT
};

/// Everything that is necessary to compute the character's interaction with the environment
struct chr_environment_t
{
    chr_environment_t() :
        grid_twist(0),
        grid_level(0.0f),
        grid_lerp(0.0f),
        water_level(0.0f),
        water_lerp(0.0f),
        floor_level(0.0f),
        level(0.0f),
        fly_level(0.0f),
        zlerp(0.0f),
        floor_speed(),
        is_slipping(false),
        is_slippy(false),
        is_watery(false),
        air_friction(0.0f),
        ice_friction(0.0f),
        fluid_friction_hrz(0.0f),
        fluid_friction_vrt(0.0f),
        traction(0.0f),
        friction_hrz(0.0f),
        inwater(false),
        grounded(true),
        new_v(),
        acc(),
        vel()
    {
        //ctor
    }

    // floor stuff
    uint8_t   grid_twist;           ///< The twist parameter of the current grid (what angle it it at)
    float   grid_level;           ///< Height relative to the current grid
    float   grid_lerp;

    float   water_level;           ///< Height relative to the current water level
    float   water_lerp;

    float  floor_level;           ///< Height of tile
    float  level;                 ///< Height of a tile or a platform
    float  fly_level;             ///< Height of tile, platform, or water, whever is highest.

    float  zlerp;

    fvec3_t floor_speed;

    // friction stuff
    bool is_slipping;
    bool is_slippy,    is_watery;
    float  air_friction, ice_friction;
    float  fluid_friction_hrz, fluid_friction_vrt;
    float  traction, friction_hrz;

    // misc states
    bool inwater;
    bool grounded;              ///< standing on something?

    // various motion parameters
    fvec3_t  new_v;
    fvec3_t  acc;
    fvec3_t  vel;
};

/// the data used to define the spawning of a character
struct chr_spawn_data_t
{
    chr_spawn_data_t() :
        pos(),
        profile(INVALID_PRO_REF),
        team(0),
        skin(0),
        facing(0),
        name(),
        override(INVALID_CHR_REF)
    {
        //ctor
    }

    fvec3_t     pos;
    PRO_REF     profile;
    TEAM_REF    team;
    int         skin;
    FACING_T    facing;
    STRING      name;
    CHR_REF     override;
};

/// The definition of the character object.
class Object : public PhysicsData, public Id::NonCopyable
{
public:
    static const std::shared_ptr<Object> INVALID_OBJECT;            ///< Invalid object reference

public:
    /**
     * @brief
     *  Constructor
     * @param profile
     *  which character profile this character should be spawned with
     * @param id
     *  the unique CHR_REF associated with this character
     */
    Object(const PRO_REF profile, const CHR_REF id);

    /**
     * @brief
     *  Destructor.
     */
    virtual ~Object();

    /**
    * @brief Gets a shared_ptr to the current ObjectProfile associated with this character.
    *        The ObjectProfile can change for polymorphing objects.
    **/
    std::shared_ptr<ObjectProfile> getProfile() const {
        return ProfileSystem::get().getProfile(profile_ref);
    }

    /**
    * @return the unique CHR_REF associated with this character
    **/
    inline CHR_REF getCharacterID() const {return _characterID;}

    /**
    * @return the current team this object is on. This can change in-game (mounts or pets for example)
    **/
    inline Team& getTeam() const {return _currentModule->getTeamList()[team];}

    /**
    * @brief
    *   True if this Object is a item that can be grabbed
    **/
    inline bool isItem() const {return isitem;}

    /**
    * @return
    *   true if this Object is currently levitating above the ground
    **/
    bool isFlying() const;

    /**
    * @brief
    *   Respawns a Object, bringing it back to life and moving it to its initial position and state.
    *   Does nothing if character is already alive.
    **/
    void respawn();

    /**
    * @brief
    *   This function updates stats and such for this Object (called once per update loop)
    **/
    void update();

    /**
    * @brief
    *   This function returns true if the character is on a water tile
    * @param anyLiquid
    *   Return true for any fluid and not only water (acid, lava etc.)
    * @return 
    *   true if it is on a water tile
    **/
    bool isOverWater(bool anyLiquid) const;

    /**
    * @brief
    *   This function returns true if this Object is being held by another Object
    * @return
    *   true if held by another existing Object that is not marked for removal
    **/
    bool isBeingHeld() const;

    /**
    * @brief
    *   This function returns true if this Object is inside another Objects inventory
    * @return
    *   true if inside another existing Object's inventory
    **/
    bool isInsideInventory() const;

    /**
    * @return
    *   true if this Object has been terminated and will be removed from the game.
    *   If this value is true, then this Object is effectively no longer a part of
    *   the game and should not be interacted with.
    **/
    inline bool isTerminated() const {return _terminateRequested;}

    /**
    * @brief
    *   This function returns true if this Object is emerged in water
    * @param anyLiquid
    *   Return true for any fluid and not only water (acid, lava etc.)
    * @return 
    *   true if it is on emerged in water (fully or partially)
    **/
    bool isInWater(bool anyLiquid) const;

    /**
    * @brief Set current X, Y, Z position of this Object
    * @return true if the position of this object has changed
    **/
    bool setPosition(const fvec3_t &position);

    /**
    * @brief Set current X, Y, Z position of this Object
    * @return true if the position of this object has changed
    **/
    inline bool setPosition(const float x, const float y, const float z) {return setPosition(fvec3_t(x, y, z));}

    /**
    * @brief Translate the current X, Y, Z position of this object by the specified values
    **/
    void movePosition(const float x, const float y, const float z);

    /**
    * @brief Sets the transparency for this Object
    * @param alpha Transparency level between 0 (fully transparent) and 255 (fully opaque)
    **/
    void setAlpha(const int alpha);

    /**
    * @brief Sets the shininess of this Object
    * @param alpha Transparency level between 0 (no shine effect) and 255 (completely shiny)
    **/
    void setSheen(const int sheen);

    /**
    * @brief Sets the transparency for this Object
    * @param light Transparency level between 0 (fully transparent) and 255 (fully opaque)
    */
    void setLight(const int light);

    /**
     * @brief Checks if this Object is able to mount (ride) another Object
     * @param mount Which Object we are trying to mount
     * @return true if we are able to mount the specified Object
     */
    bool canMount(const std::shared_ptr<Object> mount) const;

    /**
     * @return true if this Object is mountable by other Objects
     */
    bool isMount() const {return getProfile()->isMount();}

    /**
    * @brief
    *   Mark this object as terminated, it will be removed from the game by the update.
    **/
    void requestTerminate();

    /**
    * @return
    *   Get the amount of money this character has
    **/
    int16_t getMoney() const { return money; }

    /**
    * @brief 
    *   This function calculates and applies damage to a character.  It also
    *   sets alerts and begins actions.  Blocking and frame invincibility are done here too.  
    *
    * @param direction
    *   Direction is ATK_FRONT if the attack is coming head on, ATK_RIGHT if from the right, 
    *   ATK_BEHIND if from the back, ATK_LEFT if from the left.
    *
    * @param damage 
    *   is a random range of damage to deal
    *
    * @param damageType 
    *   indicates what kind of damage this is (ZAP, CRUSH, FIRE, etc.) which is again 
    *   affected by resistances immunities, etc.
    *
    * @param team 
    *   which team is dealing the damage
    *
    * @param attacker 
    *   The Object which is dealing the damage to this Object
    *
    * @param effects 
    *   is a BIT_FIELD of various flags which affect how we determine damage.
    *
    * @param ignore_invictus 
    *   if this is true, then we allow damaging this object even though it is normally immune to damage.
    **/
    int damage(const FACING_T direction, const IPair  damage, const DamageType damagetype, const TEAM_REF team,
               const std::shared_ptr<Object> &attacker, const BIT_FIELD effects, const bool ignore_invictus);

    /**
     * @brief
     *  This function gives some purelife points to the target, ignoring any resistances and so forth.
     * @param healer
     *  the healer
     * @param amount
     *  the amount to heal the character
     */
    bool heal(const std::shared_ptr<Object> &healer, const UFP8_T amount, const bool ignoreInvincibility);

    /**
    * @return true if this Object is currently doing an attack animation
    **/
    bool isAttacking() const;

    /**
    * @return true if this Object is controlled by a player
    **/
    bool isPlayer() const {return islocalplayer;}

    /**
    * @brief
    *   Returns true if this Object has not been killed by anything
    **/
    bool isAlive() const {return alive;}

    bool isHidden() const {return is_hidden;}

    bool isNameKnown() const {return nameknown;}

    bool isInvincible() const {return invictus;}

    /**
    * @brief
    *   Tries to teleport this Object to the specified location if it is valid
    * @result
    *   Success returns true, failure returns false;
    **/
    bool teleport(const fvec3_t& position, const FACING_T facing_z);

    /**
    * @brief
    *   Get the name of this character if it is known by the players (e.g Fluffy) or it's class name otherwise (e.g Sheep)
    * @param prefixArticle
    *   if the appropriate article "a" or "an" should be prefixed (only valid for class name)
    * @param prefixDefinite
    *   prefix defeinite article, i.e "the" (only valid for class name)
    * @param captialLetter
    *   Capitalize the first letter in the name or class name (e.g "fluffy" -> "Fluffy")
    **/
    std::string getName(bool prefixArticle = true, bool prefixDefinite = true, bool capitalLetter = true) const;

    /**
    * @brief
    *   Checks if this Object is facing (looking) towards the specified location
    * @return
    *   true if the specified location is within a 60 degree cone of vision for this Object
    **/
    bool isFacingLocation(const float x, const float y) const;

    /**
    * @brief
    *   This makes this Object detatch from any holder (or dismount of riding a mount)
    * @return
    *   true if the detach was successful (could fail because of a kurse for example)
    **/
    bool detatchFromHolder(const bool ignoreKurse, const bool doShop);

    /**
    * @return
    *   Any Object held in the LEFT grip of this Object (or nullptr if no item is held)
    **/
    const std::shared_ptr<Object>& getLeftHandItem() const;

    /**
    * @return
    *   Any Object held in the RIGHT grip of this Object (or nullptr if no item is held)
    **/
    const std::shared_ptr<Object>& getRightHandItem() const;

    /**
    * @return
    *   true if this Object has line of sight and can see the specified Object
    **/
    bool canSeeObject(const std::shared_ptr<Object> &target) const;

    /**
    * @brief Set the fat value of a character.
    * @param chr the character
    * @param fat the new fat value
    * @remark The fat value influences the character size.
    **/
    void setFat(const float fat);

    /**
    * @brief Set the (base) height of a character.
    * @param chr the character
    * @param height the new height
    * @remark The (base) height influences the character size.
    **/
    void setBumpHeight(const float height);

    /**
    * @brief Set the (base) width of a character.
    * @param chr the character
    * @param width the new width
    * @remark Also modifies the shadow size.
    **/
    void setBumpWidth(const float width);

    //TODO: should be private
    /// @author BB
    /// @details Convert the base size values to the size values that are used in the game
    void recalculateCollisionSize();

    /**
    * @author BB
    * @details Handle a character death. Set various states, disconnect it from the world, etc.
    **/
    void kill(const std::shared_ptr<Object> &originalKiller, bool ignoreInvincibility);

    /// @author ZZ
    /// @details This function fixes an item's transparency
    void resetAlpha();

    /**
    * @brief
    *   Awards some experience points to this object, potentionally allowing it to reach another
    *   character level. This function handles additional experience gain modifiers such as
    *   XP bonus, roleplay or game difficulity.
    * @param xptype
    *   What kind of experience to give. Different classes gain experience differently depending
    *   on the kind of xp.
    * @param overrideInvincibility
    *   Invincible objects usually gain no experience (scenery objects such as a rock for example).
    *   Set this parameter to true to override this and give the experience anyways.
    **/
    void giveExperience(const int amount, const XPType xptype, const bool overrideInvincibility);


    /// @author BB
    /// @details determine the correct price for an item
    int getPrice() const;

	/** @override */
	BIT_FIELD hit_wall(fvec2_t& nrm, float *pressure, mesh_wall_data_t *data) override;
	/** @override */
	BIT_FIELD hit_wall(const fvec3_t& pos, fvec2_t& nrm, float *pressure, mesh_wall_data_t *data) override;
	/** @override */
	BIT_FIELD test_wall(mesh_wall_data_t *data) override;
	/** @override */
	BIT_FIELD test_wall(const fvec3_t& pos, mesh_wall_data_t *data) override;

    inline AABB_2D getAABB2D() const
    {
        return AABB_2D(Vector2f(getPosX() + chr_min_cv.getMin()[OCT_X], getPosY() + chr_min_cv.getMin()[OCT_Y]),
                       Vector2f(getPosX() + chr_min_cv.getMax()[OCT_X], getPosY() + chr_min_cv.getMax()[OCT_Y]));
    }

    /**
    * @brief
    *   This function takes mana from a character ( or gives mana ), and returns true if the character had enough to pay, or false
    *   otherwise. This can kill a character in hard mode.
    * @param amount
    *   How much mana to take (positive value) of give (negative value)
    * @param killer
    *   If characters have channeling they can use life instead of mana. This can actually kill them (ghosts that drain mana for example)
    * @return
    *   true if all the requested mana was successfully consumed by the Object
    **/
    bool costMana(int amount, const CHR_REF killer);

    /**
    * @return
    *   Get current mana
    **/
    float getMana() const;

    /**
    * @brief
    *   Get max allowed mana for this Object
    **/
    inline float getMaxMana() const { return getAttribute(Ego::Attribute::MAX_MANA); }

    /**
    * @return
    *   current life remaining in float format
    **/
    float getLife() const;

    /**
    * @brief
    *   Set the current life of this Object to the specified value.
    *   The value will automatically be clipped to a valid value between
    *   0.01f and the maximum life of this Object. This cannot kill the Object.
    **/
    void setLife(const float value);

    /**
    * @brief
    *   Set the current mana of this Object to the specified value.
    *   The value will automatically be clipped to a valid value between
    *   0.00f and the maximum mana of this Object
    **/
    void setMana(const float value);

    /**
    * @brief
    *   True if this object is added to a statusbar monitor
    **/
    bool getShowStatus() const { return _showStatus; }
    void setShowStatus(const bool val) { _showStatus = val; }

    /**
    * @return
    *   Get the experience level of this Object (1 being the first level)
    **/
    uint8_t getExperienceLevel() const { return experiencelevel + 1; }

    /**
    * @return
    *   The gender of this Object (if applicable)
    **/
    CharacterGender getGender() const { return static_cast<CharacterGender>(gender); }

    /**
    * @brief
    *   Gets how resistant this Object is to a specific type of damage (ZAP, FIRE, POKE, etc.)
    *   For positive Defence, damage reduction =((defence)*0.06)/(1+0.06*(defence))
    *   For negative Defence, it is damage increase = 1-0.94^(defence).
    * @param type
    *   What kind of damage resistance to retrieve
    * @param includeArmor
    *   true if Defence should be included in damage reduction calculation
    * @return
    *   A floating point value representing the damage reduction (0.0f = no reduction, 1.0f = no damage, -1.0f = double damage)
    *   I.e a return value of 0.05f would mean damage reduction of 5%.
    **/
    float getDamageReduction(const DamageType type, const bool includeArmor = true) const;

    /**
    * @brief
    *   Get character damage resistance to a specific damage. This value is non-linear.
    *   To get the actual damage scaling value, use getDamageReduction() instead.
    **/
    float getRawDamageResistance(const DamageType type, const bool includeArmor = true) const;

    /**
    * @brief
    *   Get total value for the specified attribute. Includes bonuses from Enchants, Perks
    *   and other active boni or penalties.
    **/
    float getAttribute(const Ego::Attribute::AttributeType type) const;

    /**
    * @brief
    *   Get base value for the specified attribute (without applying effects from Enchants and Perks)
    **/
    float getBaseAttribute(const Ego::Attribute::AttributeType type) const;

    /**
    * @brief
    *   Permanently increases or decreases an attribute of this Object
    **/
    void increaseBaseAttribute(const Ego::Attribute::AttributeType type, float value);

    /**
    * @brief
    *   Permanently changes the base attribute of this character to something else
    **/
    void setBaseAttribute(const Ego::Attribute::AttributeType type, float value);

    /**
    * @return
    *   The Inventory of this Object
    **/
    Inventory& getInventory();

    uint16_t getAmmo() const { return ammo; }

    /**
    * @return
    *   true if this Object has mastered the specified perk. Returns always true for NR_OF_PERKS
    **/
    bool hasPerk(Ego::Perks::PerkID perk) const;

    /**
    * @brief
    *   Generates a list of all Perks that the character can currently learn
    **/
    std::vector<Ego::Perks::PerkID> getValidPerks() const;

    /**
    * @brief
    *   permanently adds a new Perk to this character object
    **/
    void addPerk(Ego::Perks::PerkID perk);

    /**
    * @return
    *   true if this Object can detect and see invisible objects
    **/
    bool canSeeInvisible() const { return getAttribute(Ego::Attribute::SEE_INVISIBLE); }

    /**
    * @return
    *   The logic update frame when the rally bonus ends
    **/
    uint32_t getRallyDuration() const { return _reallyDuration; }


    /**
    * @brief
    *   Get the random seed used for determining which perks will be available when leveling and
    *   how much attributes get improved
    **/
    uint32_t getLevelUpSeed() const { return _levelUpSeed; }

    /**
    * @brief
    *   Generates a new random level up seed. Should be called every time a level up is complete
    *   or first time generating a character from scratch (not a save game)
    **/
    void randomizeLevelUpSeed() { _levelUpSeed = Random::next(Random::next<uint32_t>(numeric_limits<uint32_t>::max())); }

    /**
    * @brief
    *   Applies an enchantment to this object
    * @param enchantProfile
    *   The unique profile ID for the Enchantment (ENC_REF)
    * @param spawnerProfile
    *   The unique ObjectProfile ID for the object that creates this enchant
    * @brief
    *   pointer to the enchant that was added (or nullptr if it failed)
    **/
    std::shared_ptr<Ego::Enchantment> addEnchant(ENC_REF enchantProfile, PRO_REF spawnerProfile, const std::shared_ptr<Object>& owner, const std::shared_ptr<Object> &spawner);

    void removeEnchantsWithIDSZ(const IDSZ idsz);

    std::forward_list<std::shared_ptr<Ego::Enchantment>>& getActiveEnchants();

    /**
    * @brief
    *   Removes all enchantments from character
    **/
    bool disenchant();

    /**
    * @brief
    *   Changes the skin of this Object to the specified skin number.
    *   This changes this Objects damage resistances and movement speed accordingly to the new
    *   armor of the skin.
    * @return
    *   true if the skin could be changed into the specified number or false if it fails
    **/
    bool setSkin(const size_t skinNumber);

    std::unordered_map<Ego::Attribute::AttributeType, float, std::hash<uint8_t>>& getTempAttributes();

    std::shared_ptr<Ego::Enchantment> getLastEnchantmentSpawned() const;

    const std::shared_ptr<Object>& toSharedPointer() const;

private:

    /**
    * @brief This function should be used whenever a character gets attacked or healed. The function
    *        handles if the attacker is a held item (so that the holder becomes the attacker). The function also
    *        updates alerts, timers, etc. This function can trigger character cries like "That tickles!" or "Be careful!"
    **/
    void updateLastAttacker(const std::shared_ptr<Object> &attacker, bool healing);

    /**
    * @brief 
    *   This function makes the characters get bigger or smaller, depending
    *   on their fat_goto and fat_goto_time. Spellbooks do not resize
    */
    void updateResize();
    
    /**
    * @brief
    *   Checks if this Object has attained enough experience to increase its Experience Level
    **/
    void checkLevelUp();

public:
    BSP_leaf_t     bsp_leaf;

    chr_spawn_data_t  spawn_data;

    // character state
    ai_state_t     ai;              ///< ai data
    latch_t        latch;

    // character stats
    STRING         Name;            ///< My name
    uint8_t        gender;          ///< Gender

    uint32_t       experience;      ///< Experience
    uint8_t        experiencelevel; ///< Experience Level

    int16_t        money;            ///< Money
    uint16_t       ammomax;          ///< Ammo stuff
    uint16_t       ammo;

    // equipment and inventory
    std::array<CHR_REF, SLOT_COUNT> holdingwhich; ///< != INVALID_CHR_REF if character is holding something
    std::array<CHR_REF, INVEN_COUNT> equipment;   ///< != INVALID_CHR_REF if character has equipped something

    // team stuff
    TEAM_REF       team;            ///< Character's team
    TEAM_REF       team_base;        ///< Character's starting team

    float          fat_stt;                       ///< Character's initial size
    float          fat;                           ///< Character's size
    float          fat_goto;                      ///< Character's size goto
    int16_t         fat_goto_time;                 ///< Time left in size change

    // jump stuff
    uint8_t          jump_timer;                      ///< Delay until next jump
    uint8_t          jumpnumber;                    ///< Number of jumps remaining
    bool             jumpready;                     ///< For standing on a platform character

    // attachments
    CHR_REF        attachedto;                    ///< != INVALID_CHR_REF if character is a held weapon
    slot_t         inwhich_slot;                  ///< SLOT_LEFT or SLOT_RIGHT
    CHR_REF        inwhich_inventory;             ///< != INVALID_CHR_REF if character is inside an inventory

    // platform stuff
    bool         platform;                      ///< Can it be stood on
    bool         canuseplatforms;               ///< Can use platforms?
    int            holdingweight;                 ///< For weighted buttons

    // combat stuff
    DamageType          damagetarget_damagetype;       ///< Type of damage for AI DamageTarget
    DamageType          reaffirm_damagetype;           ///< For relighting torches
    SFP8_T         damage_threshold;              ///< Damage below this number is ignored (8.8 fixed point)

    // missle handling
    uint8_t          missiletreatment;              ///< For deflection, etc.
    uint8_t          missilecost;                   ///< Mana cost for each one
    CHR_REF        missilehandler;                ///< Who pays the bill for each one...

    // "variable" properties
    bool         is_hidden;
    bool         alive;                         ///< Is it alive?
    PLA_REF      is_which_player;               ///< true = player
    bool         islocalplayer;                 ///< true = local player
    bool         invictus;                      ///< Totally invincible?
    bool         iskursed;                      ///< Can't be dropped?
    bool         nameknown;                     ///< Is the name known?
    bool         ammoknown;                     ///< Is the ammo known?
    bool         hitready;                      ///< Was it just dropped?
    bool         isequipped;                    ///< For boots and rings and stuff

    // "constant" properties
    bool         isitem;                        ///< Is it grabbable?
    bool         cangrabmoney;                  ///< Picks up coins?
    bool         openstuff;                     ///< Can it open chests/doors?
    bool         stickybutt;                    ///< Rests on floor
    bool         isshopitem;                    ///< Spawned in a shop?
    bool         canbecrushed;                  ///< Crush in a door?

    // misc timers
    int16_t         grog_timer;                    ///< Grog timer
    int16_t         daze_timer;                    ///< Daze timer
    int16_t         bore_timer;                    ///< Boredom timer
    uint8_t          careful_timer;                 ///< "You hurt me!" timer
    uint16_t         reload_timer;                  ///< Time before another shot
    uint8_t          damage_timer;                  ///< Invincibility timer

    // graphical info
    uint8_t          flashand;        ///< 1,3,7,15,31 = Flash, 255 = Don't
    bool         transferblend;   ///< Give transparency to weapons?
    bool         draw_icon;       ///< Show the icon?
    uint8_t          sparkle;         ///< Sparkle color or 0 for off
    SFP8_T         uoffvel;         ///< Moving texture speed (8.8 fixed point)
    SFP8_T         voffvel;          ///< Moving texture speed (8.8 fixed point)
    float          shadow_size_stt;  ///< Initial shadow size
    uint32_t         shadow_size;      ///< Size of shadow
    uint32_t         shadow_size_save; ///< Without size modifiers

    // model info
    bool         is_overlay;                    ///< Is this an overlay? Track aitarget...
    SKIN_T         skin;                          ///< Character's skin
    PRO_REF        profile_ref;                      ///< Character's profile
    PRO_REF        basemodel_ref;                     ///< The true form
    chr_instance_t inst;                          ///< the render data

    // collision info

    /// @note - to make it easier for things to "hit" one another (like a damage particle from
    ///        a torch hitting a grub bug), Aaron sometimes made the bumper size much different
    ///        than the shape of the actual object.
    ///        The old bumper data that is read from the data.txt file will be kept in
    ///        the struct "bump". A new bumper that actually matches the size of the object will
    ///        be kept in the struct "collision"
    bumper_t     bump_stt;
    bumper_t     bump;
    bumper_t     bump_save;

    bumper_t     bump_1;       ///< the loosest collision volume that mimics the current bump
    oct_bb_t     chr_max_cv;   ///< a looser collision volume for chr-prt interactions
    oct_bb_t     chr_min_cv;   ///< the tightest collision volume for chr-chr interactions

    std::array<oct_bb_t, SLOT_COUNT>     slot_cv;  ///< the cv's for the object's slots

    uint8_t        stoppedby;                     ///< Collision mask

    orientation_t  ori;                           ///< Character's orientation
    orientation_t  ori_old;                       ///< Character's last orientation


    CHR_REF        bumplist_next;                 ///< Next character on fanblock

    // movement properties
    turn_mode_t  turnmode;                      ///< Turning mode

    BIT_FIELD      movement_bits;                 ///< What movement modes are allowed?
    float          anim_speed_sneak;              ///< Movement rate of the sneak animation
    float          anim_speed_walk;               ///< Walking if above this speed
    float          anim_speed_run;                ///< Running if above this speed
    float          maxaccel;                      ///< Current maximum acceleration

    // data for doing the physics in bump_all_objects()

    chr_environment_t enviro;

    int               dismount_timer;                ///< a timer BB added in to make mounts and dismounts not so unpredictable
    CHR_REF           dismount_object;               ///< the object that you were dismounting from

    breadcrumb_list_t crumbs;                     ///< a list of previous valid positions that the object has passed through

private:
    bool _terminateRequested;                            ///< True if this character no longer exists in the game and should be destructed
    CHR_REF _characterID;                                ///< Our unique CHR_REF id
    std::shared_ptr<ObjectProfile> _profile;             ///< Our Profile
    bool _showStatus;                                    ///< Display stats?

    //Attributes
    float _currentLife;
    float _currentMana;
    std::array<float, Ego::Attribute::NR_OF_ATTRIBUTES> _baseAttribute; ///< Character attributes
    std::unordered_map<Ego::Attribute::AttributeType, float, std::hash<uint8_t>> _tempAttribute; ///< Character attributes with enchants

    Inventory _inventory;
    std::bitset<Ego::Perks::NR_OF_PERKS> _perks;         ///< Perks known (super-efficient bool array)
    uint32_t _levelUpSeed;

    //Non persistent variables. Once game ends these are not saved
    bool _hasBeenKilled;                                 ///< If this Object has been killed at least once this module (many can respawn)
    uint32_t _reallyDuration;                            ///< Game Logic Update frame duration for rally bonus gained from the Perk

    //Enchantment stuff
    std::forward_list<std::shared_ptr<Ego::Enchantment>> _activeEnchants;    ///< List of all active enchants on this Object
    std::weak_ptr<Ego::Enchantment> _lastEnchantSpawned;    //< Last enchantment that his Object has spawned

    friend class ObjectHandler;
};

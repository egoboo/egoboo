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
/// @file game/Module/Module.hpp
/// @details Code handling a game module
/// @author Johan Jansen

#pragma once

#include "game/egoboo.h"
#include "game/mesh.h"
#include "game/Module/Water.hpp"

//@todo This is an ugly hack to work around cyclic dependency and private header guards
#ifndef GAME_ENTITIES_PRIVATE
    #define GAME_ENTITIES_PRIVATE 1
    #include "game/Entities/ObjectHandler.hpp"
    #undef GAME_ENTITIES_PRIVATE
#else
    #include "game/Entities/ObjectHandler.hpp"
#endif

// Forward declarations.
class ModuleProfile;
class Passage;
class Team;
namespace Ego { class Player; }
namespace Ego { namespace Input { class InputDevice; } }

/// The actual in-game state of the damage tiles
struct damagetile_instance_t
{
    IPair amount;                    ///< Amount of damage
    DamageType damagetype;

    LocalParticleProfileRef part_gpip;
    uint32_t partand;
    int    sound_index;

    damagetile_instance_t() :
        amount(),
        damagetype(DamageType::DAMAGE_DIRECT),
        part_gpip(LocalParticleProfileRef::Invalid),
        partand(0),
        sound_index(INVALID_SOUND_ID)
    {
        //ctor
    }

    void upload(const wawalite_damagetile_t& source)
    {
        this->amount.base = source.amount;
        this->amount.rand = 1;
        this->damagetype = source.damagetype;

        this->part_gpip = source.part_gpip;
        this->partand = source.partand;
        this->sound_index = Ego::Math::constrain(source.sound_index, INVALID_SOUND_ID, MAX_WAVE);
    }
};


/// The module data that the game needs.
class GameModule : public Id::NonCopyable
{
public:
    static constexpr float PITDEPTH = -60;  ///< Depth to kill character

    /**
     * @brief
     *  Prepeares a module to be played
     */
    GameModule(const std::shared_ptr<ModuleProfile> &module, const uint32_t seed);

    /**
     * Destructor.
     */
    ~GameModule();

    /**
     * @return
     *  name of the module
     */
    inline const std::string& getName() const {return _name;}

    /**
     * @return
     *   number of players that can join this module
     **/
    uint8_t getImportAmount() const;

    uint8_t getPlayerAmount() const;

    bool isImportValid() const;

    /**
     * @return
     *  @a true if the players have won
     */
    inline bool isBeaten() const {return _isBeaten;}

    /**
     * @brief
     *  Make the players win the module.
     *  If they press ESC the game ends and the end screen is shown instead of going to the pause menu
     */
    void beatModule() {_isBeaten = true;}

    /**
     * @return
     *  @a true if the players are allowed to respawn upon death
     */
    inline bool isRespawnValid() const {return _isRespawnValid;}

    /**
     * @return
     *  @a true if the players are allowed to export (save) their progress in this module upon exit
     */
    inline bool isExportValid() const {return _exportValid;}

    void setExportValid(bool valid) {_exportValid = valid;}

    bool canRespawnAnyTime() const;

    void setRespawnValid(bool valid) {_isRespawnValid = valid;}

    /// @author ZF
    /// @details This function checks all passages if there is a player in it, if it is, it plays a specified
    /// song set in by the AI script functions
    void checkPassageMusic();

    /// @author ZZ
    /// @details This function returns the owner of a item in a shop
    ObjectRef getShopOwner(const float x, const float y);

    /**
     * @brief
     *  Mark all shop passages having this owner as no longer a shop
     */
    void removeShopOwner(ObjectRef owner);

    /**
     * @return
     *  number of passages currently loaded
     */
    int getPassageCount();

    /**
     * @brief
     *   Get Passage by index number
     * @return
     *  @a nullptr if the id is invalid else the Passage located in the ordered index number
     */
    std::shared_ptr<Passage> getPassageByID(int id);

    /**
     * @brief
     *  Get folder path to the Profile of this module
     */
    const std::string& getPath() const;

    uint8_t getMaxPlayers() const;
    uint8_t getMinPlayers() const;

    const std::shared_ptr<ModuleProfile>& getModuleProfile() const {return _moduleProfile;}

    void setImportPlayers(const std::list<std::string> &players) {_playerNameList = players;}

    const std::list<std::string>& getImportPlayers() const {return _playerNameList;}

    /**
    * @brief
    *   Get list of all teams in this Module. Teams determine who like each other and who don't
    **/
    std::vector<Team>& getTeamList() {return _teamList;}

    /**
    * @return
    *   Get the ObjectHandler associated with this Module instance
    **/
    ObjectHandler& getObjectHandler() {return _gameObjects;}

    /**
    * @return
    *   true if the specified position is inside the level
    **/
    bool isInside(const float x, const float y) const;

    /**
    * Porting hack, TODO: remove
    **/
    std::shared_ptr<ego_mesh_t> getMeshPointer() { return _mesh; }

    /**
     * @brief
     *  Spawn an Object into the game.
     * @return
     *  The object that was spawned or nullptr on failure
     */
     std::shared_ptr<Object> spawnObject(const Vector3f& pos, const PRO_REF profile, const TEAM_REF team, const int skin,
                                         const Facing& facing, const std::string &name, const ObjectRef override);

     std::shared_ptr<const Ego::Texture> getTileTexture(const size_t index);
     std::shared_ptr<const Ego::Texture> getWaterTexture(const uint8_t layer);

    /**
    * @brief
    *   Update all active objects in the module
    **/
    void updateAllObjects();

    water_instance_t& getWater();

    std::shared_ptr<Ego::Player>& getPlayer(size_t index);

    const std::vector<std::shared_ptr<Ego::Player>>& getPlayerList() const;

    bool addPlayer(const std::shared_ptr<Object>& object, const Ego::Input::InputDevice &device);

    /**
    * @brief
    *   This function kills any character in a deep pit...
    **/
    void updatePits();

    /**
    * @brief
    *   Enables that falling into a pit will instantly kill characters.
    *   This is mutual exclusive with setPitsTeleport() and will disable
    *   teleporting in pits.
    **/
    void enablePitsKill();

    /**
    * @brief
    *   Enables that falling into a pit will teleport you back to a specified
    *   location. This is mutual exclusive to setPitsKill() and will disable
    *   killing in pits.
    **/
    void enablePitsTeleport(const Vector3f &location);

    /**
    * @brief
    *   This makes tiles flagged as damage tiles hurt any characters standing on 
    *   top of them
    **/
    void updateDamageTiles();

private:
    /**
    * @brief
    *   Load all passages from file
    **/
    void loadAllPassages();

    /**
    * @brief
    *   Load alliance.txt which tells which teams like which teams
    *   and which ones hate each other
    **/
    void loadTeamAlliances();

    /**
    * @brief
    *   Load all profiles required by this module into memory
    **/
    void loadProfiles();

private:
    static constexpr uint32_t PIT_CLOCK_RATE = 20;  ///< How many game ticks between each pit check
    static constexpr uint32_t DAMAGETILETIME = 32;  ///< Invincibility time

    const std::shared_ptr<ModuleProfile> _moduleProfile;
    std::vector<std::shared_ptr<Passage>> _passages;    ///< All passages in this module
    std::vector<Team> _teamList;
    ObjectHandler _gameObjects;
    std::list<std::string> _playerNameList;     ///< List of all import players
    std::vector<std::shared_ptr<Ego::Player>> _playerList;

    std::string  _name;                       ///< Module load names
    bool _exportValid;                          ///< Allow to export when module is reset?
    bool  _exportReset;                       ///< Remember original export mode if the module is restarted
    bool  _canRespawnAnyTime;                 ///< True if it's a small level...
    bool _isRespawnValid;                      ///< Can players respawn with Spacebar?
    bool _isBeaten;                               ///< Have the players won?
    uint32_t  _seed;                          ///< The module seed

    // special terrain and wawalite-related data structs
    water_instance_t _water;
    damagetile_instance_t _damageTile;

	/// @brief The mesh of the module.
	std::shared_ptr<ego_mesh_t> _mesh;

    std::array<Ego::DeferredTexture, 4> _tileTextures;
    std::array<Ego::DeferredTexture, 2> _waterTextures;

    //Pit Info
    uint32_t _pitsClock;
    bool _pitsKill;              ///< Do they kill?
    bool _pitsTeleport;          ///< Do they teleport?
    Vector3f _pitsTeleportPos;   ///< If they teleport, then where to?
};

/// @todo Remove this global.
extern std::unique_ptr<GameModule> _currentModule;


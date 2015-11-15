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

#include "game/egoboo_typedef.h"
#include "game/mesh.h"

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

/// The module data that the game needs.
class GameModule : public Id::NonCopyable
{
public:
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

    // clear passage memory
    void clearPassages();

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
    void removeShopOwner(const ObjectRef& owner);

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

    // Load all passages from file
    void loadAllPassages();

    /**
     * @brief
     *  Get folder path to the Profile of this module
     */
    const std::string& getPath() const;

    uint8_t getMaxPlayers() const;
    uint8_t getMinPlayers() const;

    const std::shared_ptr<ModuleProfile>& getModuleProfile() const {return _moduleProfile;}

    void setImportPlayers(const std::list<std::string> &players) {_playerList = players;}

    const std::list<std::string>& getImportPlayers() const {return _playerList;}

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
	void setMeshPointer(std::shared_ptr<ego_mesh_t> mesh) { _mesh = mesh; }

    /**
     * @brief
     *  Spawn an Object into the game.
     * @return
     *  The object that was spawned or nullptr on failure
     */
     std::shared_ptr<Object> spawnObject(const Vector3f& pos, const PRO_REF profile, const TEAM_REF team, const int skin,
                                         const FACING_T facing, const std::string &name, const CHR_REF override);

     const oglx_texture_t* getTileTexture(const size_t index);
     const oglx_texture_t* getWaterTexture(const uint8_t layer);

    /**
    * @brief
    *   Update all active objects in the module
    **/
    void updateAllObjects();

private:
    const std::shared_ptr<ModuleProfile> _moduleProfile;
    std::vector<std::shared_ptr<Passage>> _passages;    ///< All passages in this module
    std::vector<Team> _teamList;
    ObjectHandler _gameObjects;
    std::list<std::string> _playerList;     ///< List of all import players

    std::string  _name;                       ///< Module load names
    bool _exportValid;                          ///< Allow to export when module is reset?
    bool  _exportReset;                       ///< Remember original export mode if the module is restarted
    bool  _canRespawnAnyTime;                 ///< True if it's a small level...
    bool _isRespawnValid;                      ///< Can players respawn with Spacebar?
    bool _isBeaten;                               ///< Have the players won?
    uint32_t  _seed;                          ///< The module seed

	/// @brief The mesh of the module.
	std::shared_ptr<ego_mesh_t> _mesh;

    std::array<Ego::DeferredOpenGLTexture, 4> _tileTextures;
    std::array<Ego::DeferredOpenGLTexture, 2> _waterTextures;
};

/// @todo Remove this global.
extern std::unique_ptr<GameModule> _currentModule;


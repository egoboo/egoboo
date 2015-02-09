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
/// @file game/module/Module.hpp
/// @details Code handling a game module
/// @author Johan Jansen

#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include "game/egoboo_typedef.h"

//Forward declarations
class ModuleProfile;
class Passage;


/// the module data that the game needs
class GameModule
{
public:
    //Prepeares a module to be played
	GameModule(const std::shared_ptr<ModuleProfile> &module, const uint32_t seed);

	/**
	* Deconstructor
	**/
	~GameModule();

	/**
	* @return name of the module
	**/
	inline const std::string& getName() const {return _name;}

	/**
	* @return number of players that can join this module
	**/
	uint8_t getImportAmount() const;

	uint8_t getPlayerAmount() const;

	bool isImportValid() const;

	/**
	* @return true if the players have won
	**/
	inline bool isBeaten() const {return _isBeaten;}

	/**
	* @brief Make the players win the module. If they press ESC the game ends and 
	*		 the end screen is shown instead of going to the pause menu
	**/
	void beatModule() {_isBeaten = true;}

	/**
	* @return true if the players are allowed to respawn upon death
	**/
	inline bool isRespawnValid() const {return _isRespawnValid;}

	/**
	* @return true if the players are allowed to export (save) their progress in this 
	*		  module upon exit
	**/
	inline bool isExportValid() const {return _exportValid;}

	void setExportValid(bool valid) {_exportValid = valid;}


	inline bool canRespawnAnyTime() const {return _canRespawnAnyTime;}

	void setRespawnValid(bool valid) {_isRespawnValid = valid;}

	//clear passage memory
	void clearPassages();

    /// @author ZF
    /// @details This function checks all passages if there is a player in it, if it is, it plays a specified
    /// song set in by the AI script functions
	void checkPassageMusic();

    /// @author ZZ
    /// @details This function returns the owner of a item in a shop
	CHR_REF getShopOwner(const float x, const float y);

	/**
	* @brief Mark all shop passages having this owner as no longer a shop
	**/
	void removeShopOwner(CHR_REF owner);

	/**
	* @return number of passages currently loaded
	**/
	int getPassageCount();

	/**
	* @brief Get Passage by index number
	* @return nullptr if the id is invalid else the Passage located in the ordered index number
	**/
	std::shared_ptr<Passage> getPassageByID(int id);

	//Load all passages from file
	void loadAllPassages();

	/**
	* @brief
	* 	Get folder path to the Profile of this module
	**/
	const std::string& getPath() const;

private:
	const std::shared_ptr<ModuleProfile> _moduleProfile;

    std::string  _name;               ///< Module load names
    bool _exportValid;				  ///< Allow to export when module is reset?
    bool  _exportReset;               ///< Remember original export mode if the module is restarted
    bool  _canRespawnAnyTime;         ///< True if it's a small level...
    bool _isRespawnValid;			  ///< Can players respawn with Spacebar?
    bool _isBeaten;				 	  ///< Have the players won?
    uint32_t  _seed;                  ///< The module seed

    std::vector<std::shared_ptr<Passage>> _passages;	///< All passages in this module
};


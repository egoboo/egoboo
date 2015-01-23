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

struct mod_file_t;


/// the module data that the game needs
class GameModule
{
public:
	GameModule();

public:

    //Prepeares a module to be played
	bool setup(const mod_file_t * pdata, const std::string& loadname, const uint32_t seed);

	bool reset(const uint32_t seed);

    /// @author BB
    /// @details Let the module go
	bool start();

    /// @author BB
    /// @details stop the module
	bool stop();

	/**
	* @return name of the module
	**/
	inline const std::string& getName() const {return _name;}

	/**
	* @return number of players that can join this module
	**/
	inline uint8_t getImportAmount() const {return _importAmount;}

	inline uint8_t getPlayerAmount() const {return _playerAmount;}

	inline bool isImportValid() const {return _importAmount > 0;}

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

private:
    std::string  _name;               ///< Module load names
    uint8_t   _importAmount;          ///< Number of imports for this module
    bool _exportValid;				  ///< Allow to export when module is reset?
    bool  _exportReset;               ///< Remember original export mode if the module is restarted
    uint8_t _playerAmount;            ///< How many players?
    bool  _canRespawnAnyTime;         ///< True if it's a small level...
    bool _isRespawnValid;			  ///< Can players respawn with Spacebar?
    bool _isBeaten;				 	  ///< Have the players won?
    uint32_t  _seed;                  ///< The module seed

};


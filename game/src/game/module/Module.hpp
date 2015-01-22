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

	/**
	* @return true if the players are allowed to export (save) their progress in this 
	*		  module upon exit
	**/
	inline bool isExportValid() const {return _exportValid;}

	void setExportValid(bool valid) {_exportValid = valid;}

public:
    bool  exportreset;                
    uint8_t   playeramount;               ///< How many players?
    bool  importvalid;                ///< Can it import?
    bool  respawnvalid;               ///< Can players respawn with Spacebar?
    bool  respawnanytime;             ///< True if it's a small level...

    bool  active;                     ///< Is the control loop still going?
    bool  beat;                       ///< Show Module Ended text?
    uint32_t  seed;                       ///< The module seed

private:
    std::string  _name;               ///< Module load names
    uint8_t   _importAmount;          ///< Number of imports for this module
    bool _exportValid;				  ///< Allow to export when module is reset?

};


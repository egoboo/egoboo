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
/// @file game/Profiles/ProfileSystem.hpp
/// @author Johan Jansen

#pragma once
#if !defined(GAME_PROFILES_PRIVATE) || GAME_PROFILES_PRIVATE != 1
#error(do not include directly, include `game/Profiles/_Include.hpp` instead)
#endif

#include "game/egoboo_typedef.h"

//Forward declarations
class ObjectProfile;
class ModuleProfile;
class LoadPlayerElement;
struct mad_t;

/// Placeholders used while importing profiles
struct pro_import_t
{
	pro_import_t() :
		slot(-1),
		player(0),
		slot_lst(),
		max_slot(0)
	{
		//ctor
	}

    int   slot;
    int   player;
    std::array<int, 10*4> slot_lst; //MAX_IMPORT_PER_PLAYER * MAX_PLAYER;
    int   max_slot;
};

class ProfileSystem
{
public:
	ProfileSystem();

    /**
     * @brief
     *  Initialize the profile system.
     * @return
     *  @a true on success, @a false on failure.
     */
	bool initialize();

	/**
     * @brief
     *  Uninitialize the profile system.
     */
	void uninitialize();

    /**
     * @brief
     *  Reset all profile systems to state (conceptually) equivalent to their states directly after their initialization.
     */
	void reset();

	//ZF> TODO: These C-like functions need to be removed
	EVE_REF pro_get_ieve( const PRO_REF iobj );
	mad_t * pro_get_pmad( const PRO_REF iobj );
	eve_t * pro_get_peve( const PRO_REF iobj );
	pip_t * pro_get_ppip( const PRO_REF iobj, int pip_index );

	/**
	* Returns true if the specified profile id has been loaded
	**/
	inline bool isValidProfileID(PRO_REF id) const {return _profilesLoaded.find(id) != _profilesLoaded.end();}

    /**
    *  @details This function loads one object and returns the object slot
    **/
	PRO_REF loadOneProfile(const std::string &folderPath, int slot_override = -1);

	/**
	* @brief Loads only the slot number from data.txt
	*        If slot_override is valid, then that is used indead
	**/
	int getProfileSlotNumber(const std::string &folderPath, int slot_override = -1);

	/**
	* @return Returns the ObjectProfile loaded into the specified slot number
	**/
	const std::shared_ptr<ObjectProfile> &getProfile(PRO_REF slotNumber) const;

	TX_REF getSpellBookIcon(size_t index) const;

	/**
	* Get map of all profiles loaded
	**/
	inline const std::unordered_map<PRO_REF, std::shared_ptr<ObjectProfile>>& getLoadedProfiles() const {return _profilesLoaded;}

	/**
	* @brief Scans the module folder and loads all ModuleProfiles (needs only to be done once)
	**/
	void loadModuleProfiles();

	/**
	* @return list of all ModuleProfiles currently loaded
	**/
	const std::vector<std::shared_ptr<ModuleProfile>>& getModuleProfiles() const {return _moduleProfilesLoaded;}

	void printDebugModuleList();

	/**
	* @brief
	* 	Reload list of all possible characters we might load.
	**/
	void loadAllSavedCharacters(const std::string &saveGameDirectory);

	const std::vector<std::shared_ptr<LoadPlayerElement>>& getSavedPlayers() const { return _loadPlayerList;}

private:
	bool _initialized;
	std::unordered_map<size_t, TX_REF> _bookIcons;									//List of all book icons loaded											
	std::unordered_map<PRO_REF, std::shared_ptr<ObjectProfile>> _profilesLoaded;	//Maps slot numbers to ObjectProfiles

	std::vector<std::shared_ptr<ModuleProfile>> _moduleProfilesLoaded;				//List of all valid game modules loaded

	std::vector<std::shared_ptr<LoadPlayerElement>> _loadPlayerList;				//List of characters that can be loaded (lightweight)
};

extern ProfileSystem _profileSystem;
extern pro_import_t import_data;


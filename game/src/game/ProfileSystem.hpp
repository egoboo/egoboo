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
/// @file game/ProfileSystem.hpp
/// @author Johan Jansen

#pragma once
#include <vector>
#include <unordered_map>
#include "egoboo_typedef.h"

//Forward declarations
class ObjectProfile;
struct mad_t;

/// Placeholders used while importing profiles
struct pro_import_t
{
    int   slot;
    int   player;
    std::array<int, 10*4> slot_lst; //MAX_IMPORT_PER_PLAYER * MAX_PLAYER;
    int   max_slot;
};

class ProfileSystem
{
public:
	ProfileSystem();

    /// @author BB
    /// @details initialize the profile list and load up some intialization files
    ///     necessary for the the profile loading code to work
	void begin();

	/**
	* Unloads all object profiles
	**/
	void end();

	void releaseAllProfiles();

	//ZF> TODO: These C-like functions need to be removed
	CAP_REF pro_get_icap( const PRO_REF iobj );
	MAD_REF pro_get_imad( const PRO_REF iobj );
	EVE_REF pro_get_ieve( const PRO_REF iobj );
	PIP_REF pro_get_ipip( const PRO_REF iobj, int ipip );
	cap_t * pro_get_pcap( const PRO_REF iobj );
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
	int loadOneProfile( const char* tmploadname, int slot_override = -1 );

	/**
	* @brief Loads only the slot number from data.txt
	*        If slot_override is valid, then that is used indead
	**/
	int getProfileSlotNumber(const char* tmploadname, int slot_override = -1);

	/**
	* @return Returns the ObjectProfile loaded into the specified slot number
	**/
	const std::shared_ptr<ObjectProfile> &getProfile(PRO_REF slotNumber) const;

	TX_REF getSpellBookIcon(size_t index) const;

	/**
	* Get map of all profiles loaded
	**/
	inline const std::unordered_map<PRO_REF, std::shared_ptr<ObjectProfile>>& getLoadedProfiles() const {return _profilesLoaded;}

private:
	bool _initialized;
	std::vector<TX_REF> _bookIcons;													//List of all book icons loaded
	std::unordered_map<PRO_REF, std::shared_ptr<ObjectProfile>> _profilesLoaded;	//Maps slot numbers to ObjectProfiles
};

extern ProfileSystem _profileSystem;
extern pro_import_t import_data;


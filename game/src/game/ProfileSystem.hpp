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
#pragma once
#include <vector>

/// Placeholders used while importing profiles
struct pro_import_t
{
    int   slot;
    int   player;
    int   slot_lst[MAX_PROFILE];
    int   max_slot;
};

extern pro_import_t import_data;

class ProfileSystem
{
public:
	ProfileSystem();

    /// @author BB
    /// @details initialize the profile list and load up some intialization files
    ///     necessary for the the profile loading code to work
	begin();

	end();

	void releaseAllProfiles();

	CAP_REF pro_get_icap( const PRO_REF iobj );
	MAD_REF pro_get_imad( const PRO_REF iobj );
	EVE_REF pro_get_ieve( const PRO_REF iobj );
	PIP_REF pro_get_ipip( const PRO_REF iobj, int ipip );

	cap_t * pro_get_pcap( const PRO_REF iobj );
	mad_t * pro_get_pmad( const PRO_REF iobj );
	eve_t * pro_get_peve( const PRO_REF iobj );
	pip_t * pro_get_ppip( const PRO_REF iobj, int pip_index );

	inline bool isValidProfileID(PRO_REF id) const {return id >= 0 && id < _profilesLoaded.size();}

	int loadOneProfile( const char* tmploadname, int slot_override );

private:
	bool _initialized;
	std::vector<TX_REF> _bookIcons;
	std::unordered_map<PRO_REF, std::shared_ptr<ObjectProfile>> _profilesLoaded;
};

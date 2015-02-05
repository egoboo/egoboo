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

/// @file game/ModuleProfile.hpp
/// @author Johan Jansen
#pragma once

#include "game/graphic_texture.h"
#include "egolib/file_formats/module_file.h"

/// the module data that the menu system needs
class ModuleProfile
{
public:
	ModuleProfile();

	~ModuleProfile();

	bool isModuleUnlocked() const;

	ModuleFilter getModuleType() const;

	inline oglx_texture_t& getIcon() {return _icon;}

	const char* getName() const {return _base.longname;}

	const mod_file_t& getBase() const {return _base;}

	bool isStarterModule() const {return _base.importamount == 0;}

	const char* getRank() const {return _base.rank;}

	const std::string& getPath() const {return _vfsPath;}

	int getImportAmount() const {return _base.importamount;}

	int getMinPlayers() const {return _base.minplayers;}

	int getMaxPlayers() const {return _base.maxplayers;}

private:
    bool _loaded;
    std::string _name;
    mod_file_t _base;                            ///< the data for the "base class" of the module

    oglx_texture_t _icon;                        ///< the index of the module's tile image
    std::string _vfsPath;                        ///< the virtual pathname of the module
    std::string _destPath;                       ///< the path that module data can be written into

    friend class ProfileSystem;
};

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

/// @file game/gamestates/SelectModuleState.hpp
/// @details A screen for selecting which module to play. 
///			 Could be New Game or a Load Game with one or more players
/// @author Johan Jansen

#pragma once
#include <vector>
#include "game/gamestates/GameState.hpp"
#include "game/graphic_texture.h"

//Forward declarations
class Image;
class Button;
class LoadPlayerElement;
class ModuleSelector;

/// the module data that the menu system needs
class MenuLoadModuleData
{
public:
	MenuLoadModuleData();

	~MenuLoadModuleData();

	bool isModuleUnlocked() const;

	inline oglx_texture_t& getIcon() {return _icon;}

	const char* getName() const {return _base.longname;}

	const mod_file_t& getBase() const {return _base;}

	bool isStarterModule() const {return _base.importamount == 0;}

	const char* getRank() const {return _base.rank;}

	const std::string& getPath() const {return _vfsPath;}

private:
    bool _loaded;
    std::string _name;
    mod_file_t _base;                            ///< the data for the "base class" of the module

    oglx_texture_t _icon;                        ///< the index of the module's tile image
    std::string _vfsPath;                        ///< the virtual pathname of the module
    std::string _destPath;                       ///< the path that module data can be written into

    friend class SelectModuleState;
};

class SelectModuleState : public GameState
{
public:
	/**
	* @brief Constructor with no import players (starter modules)
	**/
	SelectModuleState();

	/**
	* @brief Constructor with a list of players who are going to play
	**/
	SelectModuleState(const std::list<std::shared_ptr<LoadPlayerElement>> &players);

	void update() override;

	void beginState() override;

protected:
	enum ModuleFilter : uint8_t
	{
		FILTER_OFF,
		FILTER_MAIN,
		FILTER_SIDE,
		FILTER_TOWN,
		FILTER_FUN,
		NR_OF_MODULE_FILTERS,

		FILTER_STARTER				//Starter modules are special, place after last
	};

	void drawContainer() override;

	void setModuleFilter(const ModuleFilter filter);

	void printDebugModuleList();

	static void loadAllModules();

private:
	bool _onlyStarterModules;
	std::vector<std::shared_ptr<MenuLoadModuleData>> _validModules;		///< Current selectable modules (filtered, unlocked, etc.)
	std::shared_ptr<Image> _background;
	std::shared_ptr<Button> _filterButton;
	std::shared_ptr<ModuleSelector> _moduleSelector;
	ModuleFilter _moduleFilter;
	const std::list<std::shared_ptr<LoadPlayerElement>> &_selectedPlayerList;

	//No need to load this list more than once
	static std::vector<std::shared_ptr<MenuLoadModuleData>> _loadedModules;	///< All modules loaded
};

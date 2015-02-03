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

//Forward declarations
class Image;
class Button;
class LoadPlayerElement;
class MenuLoadModuleData;

class SelectModuleState : public GameState
{
public:
	SelectModuleState();

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

	void loadAllModules();

private:
	bool _onlyStarterModules;
	std::shared_ptr<Image> _background;
	std::shared_ptr<Button> _filterButton;
	ModuleFilter _moduleFilter;
	std::vector<std::shared_ptr<MenuLoadModuleData>> _validModules;		///< Current selectable modules (filtered, unlocked, etc.)
	const std::list<std::shared_ptr<LoadPlayerElement>> &_selectedPlayerList;

	std::vector<std::shared_ptr<MenuLoadModuleData>> _loadedModules;	///< All modules loaded
};

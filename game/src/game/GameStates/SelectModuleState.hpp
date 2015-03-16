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

/// @file game/GameStates/SelectModuleState.hpp
/// @details A screen for selecting which module to play. 
///			 Could be New Game or a Load Game with one or more players
/// @author Johan Jansen

#pragma once

#include "game/GameStates/GameState.hpp"
#include "game/Profiles/_Include.hpp"

//Forward declarations
class Image;
class Button;
class ModuleSelector;

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
	SelectModuleState(const std::list<std::string> &playersToLoad);

	void update() override;

	void beginState() override;

protected:
	void drawContainer() override;

	void setModuleFilter(const ModuleFilter filter);

private:
	bool _onlyStarterModules;
	std::vector<std::shared_ptr<ModuleProfile>> _validModules;		///< Current selectable modules (filtered, unlocked, etc.)
	std::shared_ptr<Image> _background;
	std::shared_ptr<Button> _filterButton;
	std::shared_ptr<Button> _chooseModule;
	std::shared_ptr<ModuleSelector> _moduleSelector;
	ModuleFilter _moduleFilter;
	std::list<std::string> _selectedPlayerList;
};

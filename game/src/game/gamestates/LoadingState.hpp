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

/// @file game/gamestates/LoadingState.hpp
/// @details Main state where the players are currently playing a module
/// @author Johan Jansen

#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include "game/gamestates/GameState.hpp"

//Forward declarations
class ModuleProfile;
class LoadPlayerElement;
class Label;

class LoadingState : public GameState
{
public:
	LoadingState(std::shared_ptr<ModuleProfile> module, const std::list<std::shared_ptr<LoadPlayerElement>> &players);
	~LoadingState();

	void update() override;

	void beginState() override;
	
protected:
	void drawContainer() override;

	void loadModuleData();

	const std::string getRandomHint() const;

    /// @author ZF
    /// @details This function loads all module specific hints and tips. If this fails, the game will
    ///       default to the global hints and tips instead
  	bool loadLocalModuleHints();

    /// @author ZF
    /// @details This function loads all of the game hints and tips
	bool loadGlobalHints();

	/**
	* ZF> This function is a place-holder hack until we get proper threaded loading working
	**/
	void singleThreadRedrawHack(const std::string &loadingText);

private:
	std::atomic_bool _finishedLoading;
	std::thread _loadingThread;
	std::shared_ptr<Label> _loadingLabel;
	const std::shared_ptr<ModuleProfile> _loadModule;
	std::list<std::shared_ptr<LoadPlayerElement>> _players;

	std::vector<std::string> _globalGameTips;		//Generic game tips for the whole game
	std::vector<std::string> _localGameTips;		//Game tips specific to this module
};

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

/// @file game/gamestates/SelectCharacterState.hpp
/// @details Select which character this player is going to play
/// @author Johan Jansen

#pragma once

#include <vector>
#include "game/gamestates/GameState.hpp"

//Forward declarations
class LoadPlayerElement;

class SelectCharacterState : public GameState
{
public:
	/**
	* @brief
	*	Constructor for the SelectCharacterState
	* @param loadPlayerList
	*	A list containing all characters that can be selected by a player
	* @param selectedCharacter
	*	Reference to the current selected character by the current selecting player. This can be nullptr if the current selected player
	*   has not picked a Character yet. This reference is modified by this GameState to determine which Character is actually picked by
	*   the current player. Note that it is *not* const, which means it is mutable by intention.
	**/
	SelectCharacterState(const std::vector<std::shared_ptr<LoadPlayerElement>> &loadPlayerList, std::shared_ptr<LoadPlayerElement> &selectedCharacter);

	void update() override;

	void beginState() override;

protected:
	void drawContainer() override;

private:
	const std::vector<std::shared_ptr<LoadPlayerElement>> &_loadPlayerList;
};

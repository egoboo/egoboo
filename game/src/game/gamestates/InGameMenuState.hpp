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

/// @file game/gamestates/InGameMenuState.hpp
/// @details Menu while PlayingState is running in background
/// @author Johan Jansen

#pragma once

#include "game/gamestates/GameState.hpp"

//Forward declarations
class Button;
class PlayingState;

class InGameMenuState : public GameState
{
public:
	InGameMenuState(PlayingState *playingState);

	void update() override;

	void beginState() override;

protected:
	void drawContainer() override;

private:
	std::forward_list<std::shared_ptr<Button>> _slidyButtons;
	PlayingState *_playingState;
};

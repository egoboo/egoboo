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

/// @file game/gamestates/PlayingState.cpp
/// @details Main state where the players are currently playing a module
/// @author Johan Jansen

#include "game/gamestates/PlayingState.hpp"
#include "egolib/egoboo_setup.h"
#include "game/game.h"
#include "game/graphic.h"
#include "game/renderer_2d.h"
#include "game/module/Module.hpp"

PlayingState::PlayingState()
{
	//ctor
}

void PlayingState::update()
{
    update_game();
}

void PlayingState::drawContainer()
{
	gfx_system_main();

	DisplayMsg_timechange++;
}

void PlayingState::beginState()
{
	// in-game settings
    SDL_ShowCursor( cfg.hide_mouse ? SDL_DISABLE : SDL_ENABLE );
    SDL_WM_GrabInput( cfg.grab_mouse ? SDL_GRAB_ON : SDL_GRAB_OFF );
}
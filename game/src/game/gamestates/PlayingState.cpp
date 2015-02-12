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
#include "game/gamestates/InGameMenuState.hpp"
#include "game/gamestates/VictoryScreen.hpp"
#include "game/core/GameEngine.hpp"
#include "game/profiles/ProfileSystem.hpp"
#include "egolib/egoboo_setup.h"
#include "game/game.h"
#include "game/graphic.h"
#include "game/renderer_2d.h"

//For cheats
#include "game/module/ObjectHandler.hpp"
#include "game/entities/GameObject.hpp"
#include "game/module/Module.hpp"
#include "game/char.h"

PlayingState::PlayingState()
{
	//ctor
}

PlayingState::~PlayingState()
{
	//Check for player exports
    if ( PMod && PMod->isExportValid() )
    {
        // export the players
        export_all_players(false);

        //Reload list of loadable characters
        _profileSystem.loadAllSavedCharacters("mp_players");
    }

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

    if(cfg.hide_mouse) {
	    _gameEngine->disableMouseCursor();
    }
    else {
	    _gameEngine->enableMouseCursor();
    }
}

bool PlayingState::notifyKeyDown(const int keyCode)
{
	switch(keyCode)
	{
		case SDLK_ESCAPE:

			//If we have won show the Victory Screen
			if(PMod->isBeaten()) {
				_gameEngine->pushGameState(std::make_shared<VictoryScreen>(this));
			}

			//Else do the ingame menu
			else {
				_gameEngine->pushGameState(std::make_shared<InGameMenuState>(this));				
			}
		return true;

		//Cheat debug button to win a module
		case SDLK_F9:
			if(cfg.dev_mode)
			{
				for(const std::shared_ptr<GameObject> &object : _gameObjects.iterator())
				{
					if(object->isTerminated() || object->getProfile()->isInvincible()) {
						continue;
					}

					if(!object->isPlayer() && object->isAlive())
					{
						kill_character(object->getCharacterID(), MAX_CHR, false);
					}
				}
				return true;
			}
		break;
	}

	return ComponentContainer::notifyKeyDown(keyCode);
}
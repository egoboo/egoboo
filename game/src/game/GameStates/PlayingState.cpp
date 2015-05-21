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

/// @file game/GameStates/PlayingState.cpp
/// @details Main state where the players are currently playing a module
/// @author Johan Jansen

#include "game/GameStates/PlayingState.hpp"
#include "game/GameStates/InGameMenuState.hpp"
#include "game/GameStates/VictoryScreen.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/InternalDebugWindow.hpp"
#include "game/game.h"
#include "game/graphic.h"
#include "game/renderer_2d.h"

//For cheats
#include "game/Entities/_Include.hpp"
#include "game/Module/Module.hpp"
#include "game/char.h"

PlayingState::PlayingState(std::shared_ptr<CameraSystem> cameraSystem) :
    _cameraSystem(cameraSystem)
{
    //For debug only
    if (egoboo_config_t::get().debug_developerMode_enable.getValue())
    {
        std::shared_ptr<InternalDebugWindow> debugWindow = std::make_shared<InternalDebugWindow>("CurrentModule");
        debugWindow->addWatchVariable("Passages", []{return std::to_string(PMod->getPassageCount());} );
        debugWindow->addWatchVariable("ExportValid", []{return PMod->isExportValid() ? "true" : "false";} );
        debugWindow->addWatchVariable("ModuleBeaten", []{return PMod->isBeaten() ? "true" : "false";} );
        debugWindow->addWatchVariable("Players", []{return std::to_string(PMod->getPlayerAmount());} );
        debugWindow->addWatchVariable("Imports", []{return std::to_string(PMod->getImportAmount());} );
        debugWindow->addWatchVariable("Name", []{return PMod->getName();} );
        debugWindow->addWatchVariable("Path", []{return PMod->getPath();} );
        addComponent(debugWindow);        
    }
}

PlayingState::~PlayingState()
{
    //Check for player exports
    if ( PMod && PMod->isExportValid() )
    {
        // export the players
        export_all_players(false);

        //Reload list of loadable characters
        ProfileSystem::get().loadAllSavedCharacters("mp_players");
    }

    //Stop music
    AudioSystem::get().fadeAllSounds();  
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
    SDL_ShowCursor(egoboo_config_t::get().debug_hideMouse.getValue() ? SDL_DISABLE : SDL_ENABLE );
    SDL_SetWindowGrab(sdl_scr.window, egoboo_config_t::get().debug_grabMouse.getValue() ? SDL_TRUE : SDL_FALSE);

    if(egoboo_config_t::get().debug_hideMouse.getValue())
    {
        _gameEngine->disableMouseCursor();
    }
    else
    {
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
            if (egoboo_config_t::get().debug_developerMode_enable.getValue())
            {
                for(const std::shared_ptr<Object> &object : _gameObjects.iterator())
                {
                    if(object->isTerminated() || object->getProfile()->isInvincible()) {
                        continue;
                    }

                    if(!object->isPlayer() && object->isAlive())
                    {
                        object->kill(Object::INVALID_OBJECT, false);
                    }
                }
                return true;
            }
        break;
    }

    return ComponentContainer::notifyKeyDown(keyCode);
}

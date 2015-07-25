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
#include "game/GUI/MiniMap.hpp"
#include "game/GUI/CharacterStatus.hpp"
#include "game/GUI/CharacterWindow.hpp"
#include "game/game.h"
#include "game/graphic.h"
#include "game/renderer_2d.h"
#include "game/player.h"

//For cheats
#include "game/Entities/_Include.hpp"
#include "game/Module/Module.hpp"
#include "game/char.h"

PlayingState::PlayingState(std::shared_ptr<CameraSystem> cameraSystem) :
    _cameraSystem(cameraSystem),
    _miniMap(std::make_shared<MiniMap>()),
    _statusList()
{
    //For debug only
    if (egoboo_config_t::get().debug_developerMode_enable.getValue())
    {
        std::shared_ptr<InternalDebugWindow> debugWindow = std::make_shared<InternalDebugWindow>("CurrentModule");
        debugWindow->addWatchVariable("Passages", []{return std::to_string(_currentModule->getPassageCount());} );
        debugWindow->addWatchVariable("ExportValid", []{return _currentModule->isExportValid() ? "true" : "false";} );
        debugWindow->addWatchVariable("ModuleBeaten", []{return _currentModule->isBeaten() ? "true" : "false";} );
        debugWindow->addWatchVariable("Players", []{return std::to_string(_currentModule->getPlayerAmount());} );
        debugWindow->addWatchVariable("Imports", []{return std::to_string(_currentModule->getImportAmount());} );
        debugWindow->addWatchVariable("Name", []{return _currentModule->getName();} );
        debugWindow->addWatchVariable("Path", []{return _currentModule->getPath();} );
        addComponent(debugWindow);        
    }

    //Add minimap to the list of GUI components to render
    _miniMap->setSize(MiniMap::MAPSIZE, MiniMap::MAPSIZE);
    _miniMap->setPosition(0, _gameEngine->getUIManager()->getScreenHeight()-_miniMap->getHeight());
    addComponent(_miniMap);

    //Show status display for all players
    for (PLA_REF iplayer = 0; iplayer < MAX_PLAYER; iplayer++)
    {
        // Only valid players
        if (!PlaStack.lst[iplayer].valid) continue;
        addStatusMonitor(_currentModule->getObjectHandler()[PlaStack.lst[iplayer].index]);
    }
}

PlayingState::~PlayingState()
{
    //Check for player exports
    if ( _currentModule && _currentModule->isExportValid() )
    {
        // export the players
        export_all_players(false);

        //Reload list of loadable characters
        ProfileSystem::get().loadAllSavedCharacters("mp_players");
    }

    //Stop music
    AudioSystem::get().fadeAllSounds();  
}

void PlayingState::updateStatusBarPosition()
{
    static uint32_t recalculateStatusBarPosition = 0;
    if(SDL_GetTicks() > recalculateStatusBarPosition) 
    {
        //Apply throttle... no need to do every update frame
        recalculateStatusBarPosition = SDL_GetTicks() + 200;

        std::unordered_map<std::shared_ptr<Camera>, float> maxY;
        for(const std::weak_ptr<CharacterStatus> &weakStatus : _statusList)
        {
            std::shared_ptr<CharacterStatus> status = weakStatus.lock();
            if(status)
            {
                const std::shared_ptr<Camera> &camera = _cameraSystem->getCameraByChrID(status->getObject()->getCharacterID());

                //Shift component down a bit if required
                status->setPosition(status->getX(), maxY[camera] + 10.0f);

                //Calculate bottom Y coordinate for this component
                maxY[camera] = std::max<float>(maxY[camera], status->getY() + status->getHeight());
            }
        }

    }
}

void PlayingState::update()
{
    update_game();

    //Calculate position of all status bars
    updateStatusBarPosition();
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

//    if(egoboo_config_t::get().debug_hideMouse.getValue())
//    {
//        _gameEngine->disableMouseCursor();
//    }
//    else
//    {
//        _gameEngine->enableMouseCursor();
//    }
}

bool PlayingState::notifyKeyDown(const int keyCode)
{
    switch(keyCode)
    {
        case SDLK_ESCAPE:

            //If we have won show the Victory Screen
            if(_currentModule->isBeaten()) {
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
                for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
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

        //Show character sheet
        case SDLK_1:
        case SDLK_2:
        case SDLK_3:
        case SDLK_4:
        case SDLK_5:
        case SDLK_6:
        case SDLK_7:
        case SDLK_8:
        {
            //Ensure that the same character cannot open more than 1 character window
            const size_t statusNumber = keyCode - SDLK_1;
            displayCharacterWindow(statusNumber);
        }
        return true;
    }

    return ComponentContainer::notifyKeyDown(keyCode);
}

const std::shared_ptr<MiniMap>& PlayingState::getMiniMap() const
{
    return _miniMap;
}

void PlayingState::addStatusMonitor(const std::shared_ptr<Object> &object)
{
    //Disabled by configuration?
    if(!egoboo_config_t::get().hud_displayStatusBars.getValue()) {
        return;
    }

    //Already added?
    if(object->getShowStatus()) {
        return;
    }

    //Get the camera that is following this object (defaults to main camera)
    const std::shared_ptr<Camera> &camera = CameraSystem::get()->getCameraByChrID(object->getCharacterID());

    std::shared_ptr<CharacterStatus> status = std::make_shared<CharacterStatus>(object);

    status->setSize(BARX, BARY);
    status->setPosition(camera->getScreen().xmax - status->getWidth(), camera->getScreen().ymin);

    addComponent(status);
    _statusList.push_back(status);

    object->setShowStatus(true);
}

std::shared_ptr<Object> PlayingState::getStatusCharacter(size_t index)
{
    //First remove all expired elements
    auto condition = 
        [](const std::weak_ptr<CharacterStatus> &element)
        {   
            return element.expired();
        };
    _statusList.erase(std::remove_if(_statusList.begin(), _statusList.end(), condition), _statusList.end());


    if(index >= _statusList.size()) {
        return nullptr;
    }

    std::shared_ptr<CharacterStatus> status = _statusList[index].lock();
    if(!status) {
        return nullptr;
    }

    return status->getObject();
}

void PlayingState::displayCharacterWindow(uint8_t statusNumber)
{
    if(statusNumber >= _characterWindows.size()) {
        return;
    }

    std::shared_ptr<Ego::GUI::CharacterWindow> chrWindow = _characterWindows[statusNumber].lock();
    if(chrWindow == nullptr || _characterWindows[statusNumber].expired())
    {
        if(getStatusCharacter(statusNumber) != nullptr)
        {
            chrWindow = std::make_shared<Ego::GUI::CharacterWindow>(getStatusCharacter(statusNumber));
            _characterWindows[statusNumber] = chrWindow;
            addComponent(chrWindow);
        }
    }
    else
    {
        //Close window if same button is pressed twice
        chrWindow->destroy();
    }
}

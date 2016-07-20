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
#include "game/GUI/MessageLog.hpp"
#include "game/game.h"
#include "game/graphic.h"
#include "game/Logic/Player.hpp"

//For cheats
#include "game/Entities/_Include.hpp"
#include "game/Module/Module.hpp"

PlayingState::PlayingState(std::shared_ptr<CameraSystem> cameraSystem) :
    _cameraSystem(cameraSystem),
    _miniMap(std::make_shared<Ego::GUI::MiniMap>()),
    _messageLog(std::make_shared<Ego::GUI::MessageLog>()),
    _statusList()
{
    //For debug only
    if (egoboo_config_t::get().debug_developerMode_enable.getValue())
    {
        auto debugWindow = std::make_shared<Ego::GUI::InternalDebugWindow>("CurrentModule");
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
    _miniMap->setSize(Vector2f(Ego::GUI::MiniMap::MAPSIZE, Ego::GUI::MiniMap::MAPSIZE));
    _miniMap->setPosition(Point2f(0, _gameEngine->getUIManager()->getScreenHeight()-_miniMap->getHeight()));
    addComponent(_miniMap);

    //Add the message log
    _messageLog->setSize(Vector2f(_gameEngine->getUIManager()->getScreenWidth() - WRAP_TOLERANCE, _gameEngine->getUIManager()->getScreenHeight() / 3));
    _messageLog->setPosition(Point2f(0, fontyspacing));
    addComponent(_messageLog);

    //Show status display for all players
    for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList()) {
        addStatusMonitor(player->getObject());
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
    if(Time::now<Time::Unit::Ticks>() > recalculateStatusBarPosition) 
    {
        //Apply throttle... no need to do every update frame (5 Hz)
        recalculateStatusBarPosition = Time::now<Time::Unit::Ticks>() + 200;

        std::unordered_map<std::shared_ptr<Camera>, float> maxY;
        for(const std::weak_ptr<Ego::GUI::CharacterStatus> &weakStatus : _statusList)
        {
            auto status = weakStatus.lock();
            if(status)
            {
                std::shared_ptr<Object> object = status->getObject();
                if(object)
                {
                    auto camera = _cameraSystem->getCamera(object->getObjRef());

                    //Shift component down a bit if required
                    status->setPosition(Point2f(status->getX(), maxY[camera] + 10.0f));

                    //Calculate bottom Y coordinate for this component
                    maxY[camera] = std::max<float>(maxY[camera], status->getY() + status->getHeight());                    
                }
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

void PlayingState::drawContainer(Ego::GUI::DrawingContext& drawingContext)
{
    gfx_system_main();
}

void PlayingState::beginState()
{
    // in-game settings
    Ego::GraphicsSystem::setCursorVisibility(egoboo_config_t::get().debug_hideMouse.getValue());
    Ego::GraphicsSystem::window->setGrabEnabled(egoboo_config_t::get().debug_grabMouse.getValue());

//    if(egoboo_config_t::get().debug_hideMouse.getValue())
//    {
//        _gameEngine->disableMouseCursor();
//    }
//    else
//    {
//        _gameEngine->enableMouseCursor();
//    }
}

bool PlayingState::notifyKeyboardKeyPressed(const Ego::Events::KeyboardKeyPressedEventArgs& e)
{
    switch(e.getKey())
    {
        case SDLK_ESCAPE:

            //If we have won show the Victory Screen
            if(_currentModule->isBeaten()) {
                _gameEngine->pushGameState(std::make_shared<VictoryScreen>(this));
            }

            //Else do the ingame menu
            else {
                _gameEngine->pushGameState(std::make_shared<InGameMenuState>(*this));                
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
            const size_t statusNumber = e.getKey() - SDLK_1;
            displayCharacterWindow(statusNumber);
        }
        return true;
    }

    return Container::notifyKeyboardKeyPressed(e);
}

const std::shared_ptr<Ego::GUI::MiniMap>& PlayingState::getMiniMap() const
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
    auto camera = CameraSystem::get()->getCamera(object->getObjRef());

    auto status = std::make_shared<Ego::GUI::CharacterStatus>(object);

    status->setSize(Vector2f(BARX, BARY));
    status->setPosition(Point2f(camera->getScreen().xmax - status->getWidth(), camera->getScreen().ymin));

    addComponent(status);
    _statusList.push_back(status);

    object->setShowStatus(true);
}

std::shared_ptr<Object> PlayingState::getStatusCharacter(size_t index)
{
    //First remove all expired elements
    auto condition = 
        [](const std::weak_ptr<Ego::GUI::CharacterStatus> &element)
        {   
            return element.expired();
        };
    _statusList.erase(std::remove_if(_statusList.begin(), _statusList.end(), condition), _statusList.end());


    if(index >= _statusList.size()) {
        return nullptr;
    }

    std::shared_ptr<Ego::GUI::CharacterStatus> status = _statusList[index].lock();
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

const std::shared_ptr<Ego::GUI::MessageLog>& PlayingState::getMessageLog() const
{
    return _messageLog;
}

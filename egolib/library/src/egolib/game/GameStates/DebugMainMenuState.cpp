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

/// @file egolib/game/GameStates/DebugMainMenuState.cpp
/// @details State that contains buttons to all non-module related debugging states.
/// @author Johan Jansen, penguinflyer5234

#include "egolib/game/GameStates/DebugMainMenuState.hpp"
#include "egolib/game/Core/GameEngine.hpp"
#include "egolib/game/GUI/Button.hpp"

#include "egolib/game/GameStates/DebugFontRenderingState.hpp"
#include "egolib/game/GameStates/DebugModuleLoadingState.hpp"
#include "egolib/game/GameStates/DebugObjectLoadingState.hpp"

DebugMainMenuState::DebugMainMenuState() {
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();
    
    auto button = std::make_shared<Ego::GUI::Button>("Back");
    button->setPosition({ 5, SCREEN_HEIGHT - button->getHeight() - 5 });
    button->setWidth(150);
    button->setOnClickFunction([this] { endState(); });
    addComponent(button);
    
    int y = 5;
    
    button = std::make_shared<Ego::GUI::Button>("Load Modules");
    button->setPosition({ 5, y });
    button->setWidth(300);
    button->setOnClickFunction([] { _gameEngine->pushGameState(std::make_shared<DebugModuleLoadingState>()); });
    addComponent(button);
    y += button->getHeight() + 10;
    
    button = std::make_shared<Ego::GUI::Button>("Load Objects");
    button->setPosition({ 5, y });
    button->setWidth(300);
    button->setOnClickFunction([] { _gameEngine->pushGameState(std::make_shared<DebugObjectLoadingState>()); });
    addComponent(button);
    y += button->getHeight() + 10;
    
    button = std::make_shared<Ego::GUI::Button>("Font Rendering");
    button->setPosition({ 5, y });
    button->setWidth(300);
    button->setOnClickFunction([] { _gameEngine->pushGameState(std::make_shared<DebugFontRenderingState>()); });
    addComponent(button);
    y += button->getHeight() + 10;
}

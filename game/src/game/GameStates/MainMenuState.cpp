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

/// @file game/GameStates/MainMenuState.cpp
/// @details The Main Menu of the game, the first screen presented to the players
/// @author Johan Jansen

#include "game/GameStates/MainMenuState.hpp"
#include "game/GameStates/DebugMainMenuState.hpp"
#include "game/GameStates/SelectModuleState.hpp"
#include "game/GameStates/SelectPlayersState.hpp"
#include "game/GameStates/OptionsScreen.hpp"
#include "game/GameStates/MapEditorSelectModuleState.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/game.h"
#include "game/GUI/Button.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/Label.hpp"

MainMenuState::MainMenuState() :
	_slidyButtons()
{
	std::shared_ptr<Ego::GUI::Image> background;
	std::shared_ptr<Ego::GUI::Image> gameLogo;

	//Special xmas theme
	if (Zeitgeist::CheckTime(Zeitgeist::Time::Christmas))
	{
	    background = std::make_shared<Ego::GUI::Image>("mp_data/menu/menu_xmas");
	    gameLogo = std::make_shared<Ego::GUI::Image>("mp_data/menu/snowy_logo");
	}

	//Special Halloween theme
	else if (Zeitgeist::CheckTime(Zeitgeist::Time::Halloween))
	{
	    background = std::make_shared<Ego::GUI::Image>("mp_data/menu/menu_halloween");
	    gameLogo = std::make_shared<Ego::GUI::Image>("mp_data/menu/creepy_logo");
	}

	//Default egoboo theme
	else
	{
	    background = std::make_shared<Ego::GUI::Image>("mp_data/menu/menu_main");
	    gameLogo = std::make_shared<Ego::GUI::Image>("mp_data/menu/menu_logo");
	}

	const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
	const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

	// calculate the centered position of the background
	background->setSize(background->getTextureWidth(), background->getTextureHeight());
	background->setCenterPosition(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
	addComponent(background);

	// calculate the position of the logo
	gameLogo->setPosition(background->getX(), background->getY());
	gameLogo->setSize(gameLogo->getTextureWidth(), gameLogo->getTextureHeight());
	addComponent(gameLogo);

	//Add the buttons
	int yOffset = SCREEN_HEIGHT-80;
    auto exitButton = std::make_shared<Ego::GUI::Button>("Exit Game", SDLK_ESCAPE);
	exitButton->setPosition(20, yOffset);
	exitButton->setSize(200, 30);
	exitButton->setOnClickFunction(
	[]{
		_gameEngine->shutdown();
	});
	addComponent(exitButton);
	_slidyButtons.push_front(exitButton);

	yOffset -= exitButton->getHeight() + 10;

	auto optionsButton = std::make_shared<Ego::GUI::Button>("Options", SDLK_o);
	optionsButton->setPosition(20, yOffset);
	optionsButton->setSize(200, 30);
	optionsButton->setOnClickFunction(
	[]{
		_gameEngine->pushGameState(std::make_shared<OptionsScreen>());
	});
	addComponent(optionsButton);
	_slidyButtons.push_front(optionsButton);

	yOffset -= optionsButton->getHeight() + 10;

	auto loadGameButton = std::make_shared<Ego::GUI::Button>("Load Game", SDLK_l);
	loadGameButton->setPosition(20, yOffset);
	loadGameButton->setSize(200, 30);
	loadGameButton->setOnClickFunction(
	[]{
		_gameEngine->pushGameState(std::make_shared<SelectPlayersState>());
	});
	addComponent(loadGameButton);
	_slidyButtons.push_front(loadGameButton);

	yOffset -= loadGameButton->getHeight() + 10;

	auto newGameButton = std::make_shared<Ego::GUI::Button>("New Game", SDLK_n);
	newGameButton->setPosition(20, yOffset);
	newGameButton->setSize(200, 30);
	newGameButton->setOnClickFunction(
	[]{
		_gameEngine->pushGameState(std::make_shared<SelectModuleState>());
	});
	addComponent(newGameButton);
	_slidyButtons.push_front(newGameButton);

    yOffset -= newGameButton->getHeight() + 10;
    
    if (egoboo_config_t::get().debug_developerMode_enable.getValue())
    {
        auto debugButton = std::make_shared<Ego::GUI::Button>("Debug", SDLK_UNKNOWN);
        debugButton->setPosition(20, yOffset);
        debugButton->setSize(200, 30);
        debugButton->setOnClickFunction(
        []{
            _gameEngine->pushGameState(std::make_shared<DebugMainMenuState>());
        });
        addComponent(debugButton);
        _slidyButtons.push_front(debugButton);

	    yOffset -= debugButton->getHeight() + 10;

        auto mapEditorButton = std::make_shared<Ego::GUI::Button>("Map Editor", SDLK_m);
        mapEditorButton->setPosition(20, yOffset);
        mapEditorButton->setSize(200, 30);
        mapEditorButton->setOnClickFunction(
        []{
            _gameEngine->pushGameState(std::make_shared<Ego::GameStates::MapEditorSelectModuleState>());
        });
        addComponent(mapEditorButton);
        _slidyButtons.push_front(mapEditorButton);
    }

	//Add version label and copyright text
    auto welcomeLabel = std::make_shared<Ego::GUI::Label>(
		"Welcome to Egoboo!\n"
		"http://egoboo.sourceforge.net\n"
		"Version 2.9.0");
	welcomeLabel->setPosition(exitButton->getX() + exitButton->getWidth() + 40,
		                      SCREEN_HEIGHT - SCREEN_HEIGHT/60 - welcomeLabel->getHeight());
	addComponent(welcomeLabel);
}

void MainMenuState::update()
{
}

void MainMenuState::drawContainer()
{

}

void MainMenuState::beginState()
{
    // menu settings
    sdl_scr.window->setGrabEnabled(false);
    _gameEngine->enableMouseCursor();

    //Play the Egoboo theme music
    playMainMenuSong();

    float offset = 0;
    for(const auto& button : _slidyButtons)
    {
        button->beginSlidyButtonEffect(button->getWidth() + offset);
        offset += 20;
    }
}

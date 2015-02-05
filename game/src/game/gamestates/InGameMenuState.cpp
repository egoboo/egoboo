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

/// @file game/gamestates/InGameMenuState.cpp
/// @details Menu while PlayingState is running in background
/// @author Johan Jansen

#include "game/gamestates/InGameMenuState.hpp"
#include "game/gamestates/MainMenuState.hpp"
#include "game/gamestates/PlayingState.hpp"
#include "game/core/GameEngine.hpp"
#include "game/ui.h"
#include "game/game.h"
#include "game/gui/Button.hpp"

InGameMenuState::InGameMenuState(PlayingState *playingState) :
	_slidyButtons(),
	_playingState(playingState)
{
	//Add the buttons
	int yOffset = GFX_HEIGHT-80;
	std::shared_ptr<Button> exitButton = std::make_shared<Button>("Exit to Menu", SDLK_q);
	exitButton->setPosition(20, yOffset);
	exitButton->setSize(200, 30);
	exitButton->setOnClickFunction(
	[]{
		_gameEngine->setGameState(std::make_shared<MainMenuState>());
	});
	addComponent(exitButton);
	_slidyButtons.push_front(exitButton);

	yOffset -= exitButton->getHeight() + 10;

	std::shared_ptr<Button> optionsButton = std::make_shared<Button>("Options", SDLK_o);
	optionsButton->setPosition(20, yOffset);
	optionsButton->setSize(200, 30);
	addComponent(optionsButton);
	_slidyButtons.push_front(optionsButton);

	yOffset -= optionsButton->getHeight() + 10;

	std::shared_ptr<Button> loadGameButton = std::make_shared<Button>("Restart Module", SDLK_r);
	loadGameButton->setPosition(20, yOffset);
	loadGameButton->setSize(200, 30);
	addComponent(loadGameButton);
	_slidyButtons.push_front(loadGameButton);

	yOffset -= loadGameButton->getHeight() + 10;

	std::shared_ptr<Button> newGameButton = std::make_shared<Button>("Return to Module", SDLK_ESCAPE);
	newGameButton->setPosition(20, yOffset);
	newGameButton->setSize(200, 30);
	newGameButton->setOnClickFunction(
	[this]{
		endState();
	});
	addComponent(newGameButton);
	_slidyButtons.push_front(newGameButton);

	yOffset -= newGameButton->getHeight() + 10;
}

void InGameMenuState::update()
{
}

void InGameMenuState::drawContainer()
{
	//Render the playing state beackground first
	_playingState->drawAll();

	ui_beginFrame(0);
	{
	    draw_mouse_cursor();
	}
	ui_endFrame();
}

void InGameMenuState::beginState()
{
	// menu settings
    SDL_WM_GrabInput( SDL_GRAB_OFF );

    //Sliding buttons effect
    float offset = 0;
    for(const std::shared_ptr<Button> &button : _slidyButtons)
    {
		button->beginSlidyButtonEffect(button->getWidth() + offset);
		offset += 20;
    }
}

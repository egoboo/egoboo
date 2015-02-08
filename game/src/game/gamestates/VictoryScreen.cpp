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

/// @file game/gamestates/VictoryScreen.cpp
/// @details After beating a module, this screen is display with some end-game text
/// @author Johan Jansen

#include "game/gamestates/VictoryScreen.hpp"
#include "game/gamestates/PlayingState.hpp"
#include "game/gamestates/SelectPlayersState.hpp"
#include "game/core/GameEngine.hpp"
#include "game/gui/Button.hpp"
#include "game/gui/Label.hpp"
#include "game/module/Module.hpp"
#include "game/game.h"

VictoryScreen::VictoryScreen(PlayingState *playingState) :
	_playingState(playingState)
{
	//Add the buttons
	int yOffset = GFX_HEIGHT-80;
	std::shared_ptr<Button> exitButton = std::make_shared<Button>(PMod->isExportValid() ? "Save and Exit" : "Exit Game", SDLK_SPACE);
	exitButton->setSize(200, 30);
	exitButton->setPosition(20, yOffset);
	exitButton->setOnClickFunction(
	[]{
		_gameEngine->setGameState(std::make_shared<SelectPlayersState>());
	});
	addComponent(exitButton);

	//Add a button to allow players continue playing if they want
	std::shared_ptr<Button> abortButton = std::make_shared<Button>("Continue Playing", SDLK_ESCAPE);
	abortButton->setSize(200, 30);
	abortButton->setPosition(exitButton->getX() + exitButton->getWidth() + 20, exitButton->getY());
	abortButton->setOnClickFunction(
	[this]{
		endState();
	});
	addComponent(abortButton);

	std::shared_ptr<Label> victoryText = std::make_shared<Label>(endtext);
	victoryText->setPosition(50, 50);
	addComponent(victoryText);
}

void VictoryScreen::update()
{
}

void VictoryScreen::drawContainer()
{
	//Render the playing state beackground first
	_playingState->drawAll();
}

void VictoryScreen::beginState()
{
	// menu settings
    SDL_WM_GrabInput( SDL_GRAB_OFF );
    _gameEngine->enableMouseCursor();
}
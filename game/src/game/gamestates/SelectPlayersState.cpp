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

/// @file game/gamestates/SelectPlayersState.cpp
/// @details The Main Menu of the game, the first screen presented to the players
/// @author Johan Jansen

#include "game/gamestates/SelectPlayersState.hpp"
#include "game/gamestates/SelectModuleState.hpp"
#include "game/core/GameEngine.hpp"
#include "game/ui.h"
#include "game/gui/Button.hpp"
#include "game/gui/Image.hpp"
#include "game/gui/Label.hpp"

SelectPlayersState::SelectPlayersState() 
{
	//Load background
	std::shared_ptr<Image> background = std::make_shared<Image>("mp_data/menu/menu_selectplayers");
	background->setPosition(0, 0);
	background->setSize(GFX_WIDTH, GFX_HEIGHT);
	addComponent(background);

	//Add the buttons
	int yOffset = GFX_HEIGHT-80;
	std::shared_ptr<Button> backButton = std::make_shared<Button>("Back", SDLK_ESCAPE);
	backButton->setPosition(20, yOffset);
	backButton->setSize(200, 30);
	backButton->setOnClickFunction(
	[this]{
		endState();
	});
	addComponent(backButton);

	yOffset -= backButton->getHeight() + 10;

	std::shared_ptr<Button> continueButton = std::make_shared<Button>("Continue", SDLK_RETURN);
	continueButton->setPosition(20, yOffset);
	continueButton->setSize(200, 30);
	continueButton->setOnClickFunction(
	[]{
		//TODO
	});
	addComponent(continueButton);
	continueButton->setEnabled(false);

	//Tell them what this screen is all about
	std::shared_ptr<Label> infoText = std::make_shared<Label>("Select a character for each player that is going ot play.");
	infoText->setPosition(150, GFX_HEIGHT - 40);
	addComponent(infoText);

	//Players Label
	std::shared_ptr<Label> playersLabel = std::make_shared<Label>("PLAYERS");
	playersLabel->setPosition(20, 20);
	addComponent(playersLabel);

	std::shared_ptr<Label> characterLabel = std::make_shared<Label>("CHARACTER");
	characterLabel->setPosition(GFX_WIDTH/3, 20);
	addComponent(characterLabel);

	yOffset = playersLabel->getY() + playersLabel->getHeight() + 20;
	for(int i = 0; i < 4; ++i) {
		std::shared_ptr<Label> playerLabel = std::make_shared<Label>(std::string("Player ") + std::to_string(i+1));
		playerLabel->setPosition(40, yOffset);
		addComponent(playerLabel);

		std::shared_ptr<Button> playerButton = std::make_shared<Button>("Not playing");
		playerButton->setSize(200, 42);
		playerButton->setPosition(GFX_WIDTH/3, yOffset-10);
		addComponent(playerButton);

		yOffset += playerLabel->getHeight() + 50;
	}
}

void SelectPlayersState::update()
{
}

void SelectPlayersState::drawContainer()
{
	ui_beginFrame(0);
	{
	    draw_mouse_cursor();
	}
	ui_endFrame();
}

void SelectPlayersState::beginState()
{
	// menu settings
    SDL_WM_GrabInput(SDL_GRAB_OFF);
}
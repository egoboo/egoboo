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
#include "game/audio/AudioSystem.hpp"
#include "egolib/platform.h"
#include "game/ui.h"
#include "game/menu.h"
#include "game/game.h"
#include "game/gui/Button.hpp"
#include "game/gui/Image.hpp"

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

}
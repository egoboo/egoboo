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

/// @file game/GameStates/VideoOptionsScreen.cpp
/// @details Video settings
/// @author Johan Jansen

#include "game/GameStates/VideoOptionsScreen.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/Label.hpp"

VideoOptionsScreen::VideoOptionsScreen()
{
	std::shared_ptr<Image> background = std::make_shared<Image>("mp_data/menu/menu_video");

	const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
	const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

	// calculate the centered position of the background
	background->setSize(background->getTextureWidth(), background->getTextureHeight());
	background->setCenterPosition(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
	addComponent(background);

	//Add the buttons
	int yOffset = SCREEN_HEIGHT-80;
	std::shared_ptr<Button> backButton = std::make_shared<Button>("Back", SDLK_ESCAPE);
	backButton->setPosition(20, yOffset);
	backButton->setSize(200, 30);
	backButton->setOnClickFunction(
	[this]{
		endState();
	});
	addComponent(backButton);

	//Add version label and copyright text
	std::shared_ptr<Label> welcomeLabel = std::make_shared<Label>("Change video settings here");
	welcomeLabel->setPosition(backButton->getX() + backButton->getWidth() + 40,
		SCREEN_HEIGHT - SCREEN_HEIGHT/60 - welcomeLabel->getHeight());
	addComponent(welcomeLabel);
}

void VideoOptionsScreen::update()
{
}

void VideoOptionsScreen::drawContainer()
{

}

void VideoOptionsScreen::beginState()
{
	// menu settings
    SDL_WM_GrabInput(SDL_GRAB_OFF);
    _gameEngine->enableMouseCursor();
}

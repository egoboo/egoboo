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

/// @file game/GameStates/OptionsScreen.cpp
/// @details Options Screen menu
/// @author Johan Jansen

#include "game/GameStates/OptionsScreen.hpp"
#include "game/GameStates/VideoOptionsScreen.hpp"
#include "game/GameStates/AudioOptionsScreen.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/Label.hpp"

OptionsScreen::OptionsScreen() :
	_slidyButtons()
{
	std::shared_ptr<Image> background = std::make_shared<Image>("mp_data/menu/menu_gnome");

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
	_slidyButtons.push_front(backButton);

	yOffset -= backButton->getHeight() + 10;

	std::shared_ptr<Button> gameOptions = std::make_shared<Button>("Game Settings", SDLK_g);
	gameOptions->setPosition(20, yOffset);
	gameOptions->setSize(200, 30);
	addComponent(gameOptions);
	_slidyButtons.push_front(gameOptions);

	yOffset -= gameOptions->getHeight() + 10;

	std::shared_ptr<Button> videoOptions = std::make_shared<Button>("Video Settings", SDLK_v);
	videoOptions->setPosition(20, yOffset);
	videoOptions->setSize(200, 30);
	videoOptions->setOnClickFunction(
	[this]{
		_gameEngine->pushGameState(std::make_shared<VideoOptionsScreen>());
	});
	addComponent(videoOptions);
	_slidyButtons.push_front(videoOptions);

	yOffset -= videoOptions->getHeight() + 10;

	std::shared_ptr<Button> audioOptions = std::make_shared<Button>("Audio Settings", SDLK_a);
	audioOptions->setPosition(20, yOffset);
	audioOptions->setSize(200, 30);
	audioOptions->setOnClickFunction(
	[this]{
		_gameEngine->pushGameState(std::make_shared<AudioOptionsScreen>());
	});
	addComponent(audioOptions);
	_slidyButtons.push_front(audioOptions);

	yOffset -= audioOptions->getHeight() + 10;

	std::shared_ptr<Button> inputOptions = std::make_shared<Button>("Input Controls", SDLK_i);
	inputOptions->setPosition(20, yOffset);
	inputOptions->setSize(200, 30);
	addComponent(inputOptions);
	_slidyButtons.push_front(inputOptions);

	yOffset -= inputOptions->getHeight() + 10;

	//Add version label and copyright text
	std::shared_ptr<Label> welcomeLabel = std::make_shared<Label>("Select which settings to change");
	welcomeLabel->setPosition(backButton->getX() + backButton->getWidth() + 40,
		SCREEN_HEIGHT - SCREEN_HEIGHT/60 - welcomeLabel->getHeight());
	addComponent(welcomeLabel);
}

void OptionsScreen::update()
{
}

void OptionsScreen::drawContainer()
{

}

void OptionsScreen::beginState()
{
    // menu settings
    SDL_SetWindowGrab(sdl_scr.window, SDL_FALSE);
    _gameEngine->enableMouseCursor();

    float offset = 0;
    for(const std::shared_ptr<Button> &button : _slidyButtons)
    {
		button->beginSlidyButtonEffect(button->getWidth() + offset);
		offset += 20;
    }
}

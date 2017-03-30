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
#include "game/GameStates/InputOptionsScreen.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/Label.hpp"

OptionsScreen::OptionsScreen() :
	_slidyButtons()
{
	auto background = std::make_shared<Ego::GUI::Image>("mp_data/menu/menu_gnome");

	const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
	const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

	// calculate the centered position of the background
	background->setSize(Vector2f(background->getTextureWidth(), background->getTextureHeight()));
	background->setCenterPosition(Point2f(SCREEN_WIDTH/2, SCREEN_HEIGHT/2));
	addComponent(background);

	//Add the buttons
	int yOffset = SCREEN_HEIGHT-80;
	auto backButton = std::make_shared<Ego::GUI::Button>("Back", SDLK_ESCAPE);
	backButton->setPosition(Point2f(20, yOffset));
	backButton->setSize(Vector2f(200, 30));
	backButton->setOnClickFunction(
	[this]{
		endState();
	});
	addComponent(backButton);
	_slidyButtons.push_front(backButton);

	yOffset -= backButton->getHeight() + 10;

	auto gameOptions = std::make_shared<Ego::GUI::Button>("Game Settings", SDLK_g);
	gameOptions->setPosition(Point2f(20, yOffset));
	gameOptions->setSize(Vector2f(200, 30));
	addComponent(gameOptions);
	_slidyButtons.push_front(gameOptions);

	yOffset -= gameOptions->getHeight() + 10;

	auto videoOptions = std::make_shared<Ego::GUI::Button>("Video Settings", SDLK_v);
	videoOptions->setPosition(Point2f(20, yOffset));
	videoOptions->setSize(Vector2f(200, 30));
	videoOptions->setOnClickFunction(
	[this]{
		_gameEngine->pushGameState(std::make_shared<VideoOptionsScreen>());
	});
	addComponent(videoOptions);
	_slidyButtons.push_front(videoOptions);

	yOffset -= videoOptions->getHeight() + 10;

	auto audioOptions = std::make_shared<Ego::GUI::Button>("Audio Settings", SDLK_a);
	audioOptions->setPosition(Point2f(20, yOffset));
	audioOptions->setSize(Vector2f(200, 30));
	audioOptions->setOnClickFunction(
	[this]{
		_gameEngine->pushGameState(std::make_shared<AudioOptionsScreen>());
	});
	addComponent(audioOptions);
	_slidyButtons.push_front(audioOptions);

	yOffset -= audioOptions->getHeight() + 10;

	auto inputOptions = std::make_shared<Ego::GUI::Button>("Input Controls", SDLK_i);
	inputOptions->setPosition(Point2f(20, yOffset));
	inputOptions->setSize(Vector2f(200, 30));
	inputOptions->setOnClickFunction(
	[this]{
		_gameEngine->pushGameState(std::make_shared<Ego::GameStates::InputOptionsScreen>());
	});
	addComponent(inputOptions);
	_slidyButtons.push_front(inputOptions);

	yOffset -= inputOptions->getHeight() + 10;

	//Add version label and copyright text
	auto welcomeLabel = std::make_shared<Ego::GUI::Label>("Select which settings to change");
	welcomeLabel->setPosition(Point2f(backButton->getX() + backButton->getWidth() + 40,
		                      SCREEN_HEIGHT - SCREEN_HEIGHT/60 - welcomeLabel->getHeight()));
	addComponent(welcomeLabel);
}

void OptionsScreen::update()
{
}

void OptionsScreen::drawContainer(Ego::GUI::DrawingContext& drawingContext)
{

}

void OptionsScreen::beginState()
{
    // menu settings
    Ego::GraphicsSystem::get().window->setGrabEnabled(false);
    _gameEngine->enableMouseCursor();

    float offset = 0;
    for(const auto& button : _slidyButtons)
    {
		button->beginSlidyButtonEffect(button->getWidth() + offset);
		offset += 20;
    }
}

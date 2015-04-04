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
#include "game/GUI/ScrollableList.hpp"

VideoOptionsScreen::VideoOptionsScreen() :
	_resolutionList(std::make_shared<ScrollableList>())
{
	std::shared_ptr<Image> background = std::make_shared<Image>("mp_data/menu/menu_video");

	const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
	const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

	// calculate the centered position of the background
	background->setSize(background->getTextureWidth(), background->getTextureHeight());
	background->setCenterPosition(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
	addComponent(background);

	//Resolution
	std::shared_ptr<Label> resolutionLabel = std::make_shared<Label>("Resolution");
	resolutionLabel->setPosition(20, 5);
	addComponent(resolutionLabel);

	_resolutionList->setSize(SCREEN_WIDTH/3, SCREEN_HEIGHT/2);
	_resolutionList->setPosition(resolutionLabel->getX(), resolutionLabel->getY() + resolutionLabel->getHeight());
	addComponent(_resolutionList);

	//Build list of available resolutions
	int yOffset = 20;
    for (int i = 0; nullptr != sdl_scr.video_mode_list[i]; ++i )
    {
    	yOffset = addResolutionButton(sdl_scr.video_mode_list[i]->w, sdl_scr.video_mode_list[i]->h, yOffset);
    }

	//Back button
	yOffset = SCREEN_HEIGHT-80;
	std::shared_ptr<Button> backButton = std::make_shared<Button>("Back", SDLK_ESCAPE);
	backButton->setPosition(20, yOffset);
	backButton->setSize(200, 30);
	backButton->setOnClickFunction(
	[this]{
		endState();

		// save the setup file
		setup_upload(&cfg);
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

int VideoOptionsScreen::addResolutionButton(int width, int height, int yOffset)
{
	std::shared_ptr<Button> resolutionButton = std::make_shared<Button>(std::to_string(width) + "x" + std::to_string(height));
	resolutionButton->setSize(200, 30);
	resolutionButton->setPosition(20, 30 + yOffset);
	resolutionButton->setOnClickFunction(
		[width, height, resolutionButton, this] {

			//Set new resolution requested
			cfg.scrx_req = width;
			cfg.scry_req = height;

			//enable all resolution buttons except the one we just selected
			for(const std::shared_ptr<GUIComponent> &button : *_resolutionList.get()) {
				button->setEnabled(true);
			}
			resolutionButton->setEnabled(false);
		}
	);
	_resolutionList->addComponent(resolutionButton);

	//If this is our current resolution then make it greyed out
	if(cfg.scrx_req == width && cfg.scry_req == height) {
		resolutionButton->setEnabled(false);
	}

	//return position of next resolution button
	return yOffset + resolutionButton->getHeight() + 5;
}

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

/// @file game/GameStates/AudioOptionsScreen.cpp
/// @details Video settings
/// @author Johan Jansen

#include "game/GameStates/AudioOptionsScreen.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Slider.hpp"

AudioOptionsScreen::AudioOptionsScreen()
{
    std::shared_ptr<Image> background = std::make_shared<Image>("mp_data/menu/menu_sound");

    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    // calculate the centered position of the background
    background->setSize(background->getTextureWidth() * 0.75f, background->getTextureHeight() * 0.75f);
    background->setPosition(SCREEN_WIDTH- background->getWidth(), SCREEN_HEIGHT - background->getHeight());
    addComponent(background);

    int xPos = 50;
    int yPos = 30;

    //Music volume slider
    std::shared_ptr<Label> musicVolumeLable = std::make_shared<Label>("Music Volume:");
    musicVolumeLable->setPosition(xPos, yPos);
    addComponent(musicVolumeLable);
    yPos += musicVolumeLable->getHeight() + 5;

    std::shared_ptr<Ego::GUI::Slider> musicVolumeSlider = std::make_shared<Ego::GUI::Slider>(0, 255);
    musicVolumeSlider->setSize(SCREEN_WIDTH/3, 35);
    musicVolumeSlider->setPosition(xPos, yPos);
    addComponent(musicVolumeSlider);
    yPos += musicVolumeSlider->getHeight() + 20;

    //Sound Effect volume slider
    std::shared_ptr<Label> soundEffectLabel = std::make_shared<Label>("Sound Effect Volume:");
    soundEffectLabel->setPosition(xPos, yPos);
    addComponent(soundEffectLabel);
    yPos += soundEffectLabel->getHeight() + 5;

    std::shared_ptr<Ego::GUI::Slider> soundEffectVolumeSlider = std::make_shared<Ego::GUI::Slider>(0, 255);
    soundEffectVolumeSlider->setSize(SCREEN_WIDTH/3, 35);
    soundEffectVolumeSlider->setPosition(xPos, yPos);
    addComponent(soundEffectVolumeSlider);
    yPos += soundEffectVolumeSlider->getHeight() + 20;

    //Back button
    std::shared_ptr<Button> backButton = std::make_shared<Button>("Back", SDLK_ESCAPE);
    backButton->setPosition(20, SCREEN_HEIGHT-80);
    backButton->setSize(200, 30);
    backButton->setOnClickFunction(
    [this]{
        endState();

        // save the setup file
        setup_upload(&egoboo_config_t::get());
    });
    addComponent(backButton);

    //Add version label and copyright text
    std::shared_ptr<Label> welcomeLabel = std::make_shared<Label>("Change audio settings here");
    welcomeLabel->setPosition(backButton->getX() + backButton->getWidth() + 40,
        SCREEN_HEIGHT - SCREEN_HEIGHT/60 - welcomeLabel->getHeight());
    addComponent(welcomeLabel);
}

void AudioOptionsScreen::update()
{
}

void AudioOptionsScreen::drawContainer()
{

}

void AudioOptionsScreen::beginState()
{
    // menu settings
    SDL_SetWindowGrab(sdl_scr.window, SDL_FALSE);
    _gameEngine->enableMouseCursor();
}

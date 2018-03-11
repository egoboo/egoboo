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

/// @file egolib/game/GameStates/AudioOptionsScreen.cpp
/// @details Video settings
/// @author Johan Jansen

#include "egolib/game/GameStates/AudioOptionsScreen.hpp"
#include "egolib/game/GUI/Button.hpp"
#include "egolib/game/GUI/Image.hpp"
#include "egolib/game/GUI/Label.hpp"
#include "egolib/game/GUI/Slider.hpp"

AudioOptionsScreen::AudioOptionsScreen()
{
    auto background = std::make_shared<Ego::GUI::Image>("mp_data/menu/menu_sound");

    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    // calculate the centered position of the background
    background->setSize({ background->getTextureWidth() * 0.75f, background->getTextureHeight() * 0.75f });
    background->setPosition({ SCREEN_WIDTH - background->getWidth(), SCREEN_HEIGHT - background->getHeight() });
    addComponent(background);

    int xPos = 50;
    int yPos = 30;

    //Music volume slider
    auto musicVolumeLable = std::make_shared<Ego::GUI::Label>("Music Volume:");
    musicVolumeLable->setPosition({ xPos, yPos });
    addComponent(musicVolumeLable);
    yPos += musicVolumeLable->getHeight() + 5;

    auto musicVolumeSlider = std::make_shared<Ego::GUI::Slider>(0, MIX_MAX_VOLUME);
    musicVolumeSlider->setSize({ std::min(200, SCREEN_WIDTH / 3), 30 });
    musicVolumeSlider->setPosition({ xPos, yPos });
    musicVolumeSlider->setOnChangeFunction(
        [](int value)
    {
        egoboo_config_t::get().sound_music_volume.setValue(value);
        egoboo_config_t::get().sound_music_enable.setValue(value > 0);
        AudioSystem::get().setMusicVolume(value);
    });
    musicVolumeSlider->setValue(egoboo_config_t::get().sound_music_volume.getValue());
    addComponent(musicVolumeSlider);
    yPos += musicVolumeSlider->getHeight() + 20;

    //Sound Effect volume slider
    auto soundEffectLabel = std::make_shared<Ego::GUI::Label>("Sound Effect Volume:");
    soundEffectLabel->setPosition({ xPos, yPos });
    addComponent(soundEffectLabel);
    yPos += soundEffectLabel->getHeight() + 5;

    auto soundEffectVolumeSlider = std::make_shared<Ego::GUI::Slider>(0, MIX_MAX_VOLUME);
    soundEffectVolumeSlider->setSize({ std::min(200, SCREEN_WIDTH / 3), 30 });
    soundEffectVolumeSlider->setPosition({ xPos, yPos });
    soundEffectVolumeSlider->setOnChangeFunction(
        [](int value)
    {
        egoboo_config_t::get().sound_effects_volume.setValue(value);
        egoboo_config_t::get().sound_effects_enable.setValue(value > 0);
        AudioSystem::get().setSoundEffectVolume(value);
        AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_BUTTON_CLICK));
    });
    soundEffectVolumeSlider->setValue(egoboo_config_t::get().sound_effects_volume.getValue());
    addComponent(soundEffectVolumeSlider);
    yPos += soundEffectVolumeSlider->getHeight() + 20;

    // Sound channels slider
    auto soundChannelsLabel = std::make_shared<Ego::GUI::Label>("Sound Channels:");
    soundChannelsLabel->setPosition({ xPos, yPos });
    addComponent(soundChannelsLabel);
    yPos += soundChannelsLabel->getHeight() + 5;

    std::shared_ptr<Ego::GUI::Slider> soundChannelsSlider = std::make_shared<Ego::GUI::Slider>(8, 128);
    soundChannelsSlider->setSize({ std::min(200, SCREEN_WIDTH / 3), 30 });
    soundChannelsSlider->setPosition({ xPos, yPos });
    soundChannelsSlider->setOnChangeFunction(
        [](int value) { 
        egoboo_config_t::get().sound_channel_count.setValue(value);
        Mix_AllocateChannels(egoboo_config_t::get().sound_channel_count.getValue());
    });
    soundChannelsSlider->setValue(egoboo_config_t::get().sound_channel_count.getValue());
    addComponent(soundChannelsSlider);
    yPos += soundChannelsSlider->getHeight() + 20;

    // Footstep button
    auto footstepLabel = std::make_shared<Ego::GUI::Label>("Play Footsteps:");
    footstepLabel->setPosition({ xPos, yPos });
    addComponent(footstepLabel);
    yPos += footstepLabel->getHeight() + 5;

    auto footstepButton = std::make_shared<Ego::GUI::Button>(egoboo_config_t::get().sound_footfallEffects_enable.getValue() ? "Yes" : "No");
    footstepButton->setPosition({ xPos + footstepLabel->getWidth() + 10, footstepLabel->getY() });
    footstepButton->setSize({ 100, 30 });
    _connections.push_back(footstepButton->Clicked.subscribe(
    [footstepButton]{
        egoboo_config_t::get().sound_footfallEffects_enable.setValue(!egoboo_config_t::get().sound_footfallEffects_enable.getValue());
        footstepButton->setText(egoboo_config_t::get().sound_footfallEffects_enable.getValue() ? "Yes" : "No");
    }));
    addComponent(footstepButton);

    // Back button
    auto backButton = std::make_shared<Ego::GUI::Button>("Back", SDLK_ESCAPE);
    backButton->setPosition({ 20, SCREEN_HEIGHT - 80 });
    backButton->setSize({ 200, 30 });
    _connections.push_back(backButton->Clicked.subscribe(
    [this]{
        endState();

        // Save the setup file
        Ego::Setup::upload(egoboo_config_t::get());
    }));
    addComponent(backButton);

    //Add version label and copyright text
    auto welcomeLabel = std::make_shared<Ego::GUI::Label>("Change audio settings here");
    welcomeLabel->setPosition({ backButton->getX() + backButton->getWidth() + 40,
                                SCREEN_HEIGHT - SCREEN_HEIGHT / 60 - welcomeLabel->getHeight() });
    addComponent(welcomeLabel);
}

AudioOptionsScreen::~AudioOptionsScreen() {
    for (auto connection : _connections) {
        connection.disconnect();
    }
}

void AudioOptionsScreen::update()
{
}

void AudioOptionsScreen::drawContainer(Ego::GUI::DrawingContext& drawingContext)
{

}

void AudioOptionsScreen::beginState()
{
    // menu settings
    Ego::GraphicsSystem::get().window->grab_enabled(false);
    _gameEngine->enableMouseCursor();
}

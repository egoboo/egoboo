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
    background->setSize(background->getTextureWidth() * 0.75f, background->getTextureHeight() * 0.75f);
    background->setPosition(SCREEN_WIDTH- background->getWidth(), SCREEN_HEIGHT - background->getHeight());
    addComponent(background);

    //Resolution
    std::shared_ptr<Label> resolutionLabel = std::make_shared<Label>("Resolution");
    resolutionLabel->setPosition(20, 5);
    addComponent(resolutionLabel);

    _resolutionList->setSize(SCREEN_WIDTH/3, SCREEN_HEIGHT/2);
    _resolutionList->setPosition(resolutionLabel->getX(), resolutionLabel->getY() + resolutionLabel->getHeight());
    addComponent(_resolutionList);

    //Build list of available resolutions
    std::unordered_set<uint32_t> resolutions;
    for (const auto &mode : sdl_scr.video_mode_list)
    {
        //Skip duplicate resolutions (32-bit, 24-bit, 16-bit etc.)
        if(resolutions.find(mode.w | mode.h << 16) != resolutions.end()) {
            continue;
        }

        addResolutionButton(mode.w, mode.h);
        resolutions.insert(mode.w | mode.h << 16);
    }
    
    _resolutionList->forceUpdate();

    int xPos = 50 + SCREEN_WIDTH/3;
    int yPos = 30;

    //Fullscreen button
    yPos += addOptionsButton(xPos, yPos, 
        "Fullscreen", 
        
        //String description of current state
        []{ 
            return egoboo_config_t::get().graphic_fullscreen.getValue() ? "Enabled" : "Disabled"; 
        },

        //Change option effect
        []{
            egoboo_config_t::get().graphic_fullscreen.setValue(!egoboo_config_t::get().graphic_fullscreen.getValue());
            SDL_SetWindowFullscreen(sdl_scr.window->get(), egoboo_config_t::get().graphic_fullscreen.getValue() ? SDL_WINDOW_FULLSCREEN : 0);
        }
    );

    //Shadows
    yPos += addOptionsButton(xPos, yPos, 
        "Shadows", 
        
        //String description of current state
        []{ 
            if(!egoboo_config_t::get().graphic_shadows_enable.getValue()) return "Off";
            return egoboo_config_t::get().graphic_shadows_highQuality_enable.getValue() ? "High" : "Low";
        },

        //Change option effect
        []{
            if(!egoboo_config_t::get().graphic_shadows_enable.getValue()) {
                egoboo_config_t::get().graphic_shadows_enable.setValue(true);
                egoboo_config_t::get().graphic_shadows_highQuality_enable.setValue(false);
            }
            else if(!egoboo_config_t::get().graphic_shadows_highQuality_enable.getValue()) {
                egoboo_config_t::get().graphic_shadows_highQuality_enable.setValue(true);
            }
            else {
                egoboo_config_t::get().graphic_shadows_enable.setValue(false);
                egoboo_config_t::get().graphic_shadows_highQuality_enable.setValue(false);
            }
        }
    );

    //Texture Filtering
    yPos += addOptionsButton(xPos, yPos, 
        "Texture Quality", 
        
        //String description of current state
        []{ 
            if(egoboo_config_t::get().graphic_textureFilter_mipMapFilter.getValue() == Ego::TextureFilter::Linear) return "High";
            if(egoboo_config_t::get().graphic_textureFilter_minFilter.getValue() == Ego::TextureFilter::Linear) return "Medium";
            if(egoboo_config_t::get().graphic_textureFilter_minFilter.getValue() == Ego::TextureFilter::Nearest) return "Low";
            return "Unknown";
        },

        //Change option effect
        []{
            //Medium (Trilinear filtering)
            if(egoboo_config_t::get().graphic_textureFilter_minFilter.getValue() == Ego::TextureFilter::Nearest) {
                egoboo_config_t::get().graphic_textureFilter_minFilter.setValue(Ego::TextureFilter::Linear);
                egoboo_config_t::get().graphic_textureFilter_magFilter.setValue(Ego::TextureFilter::Linear);
                egoboo_config_t::get().graphic_textureFilter_mipMapFilter.setValue(Ego::TextureFilter::None);
            }

            //High (Trilinear mipmap filtering)
            else if(egoboo_config_t::get().graphic_textureFilter_mipMapFilter.getValue() == Ego::TextureFilter::None) {
                egoboo_config_t::get().graphic_textureFilter_minFilter.setValue(Ego::TextureFilter::Linear);
                egoboo_config_t::get().graphic_textureFilter_magFilter.setValue(Ego::TextureFilter::Linear);
                egoboo_config_t::get().graphic_textureFilter_mipMapFilter.setValue(Ego::TextureFilter::Linear);
            }

            //Low - linear filtering filtering
            else {
                egoboo_config_t::get().graphic_textureFilter_minFilter.setValue(Ego::TextureFilter::Nearest);
                egoboo_config_t::get().graphic_textureFilter_magFilter.setValue(Ego::TextureFilter::Nearest);
                egoboo_config_t::get().graphic_textureFilter_mipMapFilter.setValue(Ego::TextureFilter::None);
            }
        }
    );

    //Anisotropic Filtering
    yPos += addOptionsButton(xPos, yPos, 
        "Anisotropic Filtering", 
        
        //String description of current state
        []{ 
            if(!egoboo_config_t::get().graphic_anisotropy_enable.getValue() || 
                egoboo_config_t::get().graphic_anisotropy_levels.getValue() <= 0) {
                return std::string("Disabled");
            }
            
            return std::string("x") + std::to_string(static_cast<int>(egoboo_config_t::get().graphic_anisotropy_levels.getValue()));
        },

        //Change option effect
        []{
            if(!egoboo_config_t::get().graphic_anisotropy_enable.getValue()) {
                egoboo_config_t::get().graphic_anisotropy_enable.setValue(true);
                egoboo_config_t::get().graphic_anisotropy_levels.setValue(1.0f);
            }
            else {
                egoboo_config_t::get().graphic_anisotropy_levels.setValue((static_cast<int>(egoboo_config_t::get().graphic_anisotropy_levels.getValue()) << 1));

                if(egoboo_config_t::get().graphic_anisotropy_levels.getValue() > egoboo_config_t::get().graphic_anisotropy_levels.getMaxValue()) {
                    egoboo_config_t::get().graphic_anisotropy_levels.setValue(0.0f);
                    egoboo_config_t::get().graphic_anisotropy_enable.setValue(false);
                }
            }           
        },

        //Only enable button if option is supported by graphics card
        egoboo_config_t::get().graphic_anisotropy_levels.getMaxValue() > 0
    );

    //Anti-Aliasing
    yPos += addOptionsButton(xPos, yPos, 
        "Anti-Aliasing", 
        
        //String description of current state
        []{ 
            return egoboo_config_t::get().graphic_antialiasing.getValue() ? "Enabled" : "Disabled";
        },

        //Change option effect
        []{
            egoboo_config_t::get().graphic_antialiasing.setValue(!egoboo_config_t::get().graphic_antialiasing.getValue());
        }
    );

    //HD Textures
    yPos += addOptionsButton(xPos, yPos, 
        "Use HD Textures", 
        
        //String description of current state
        []{ 
            return egoboo_config_t::get().graphic_hd_textures_enable.getValue() ? "Enabled" : "Disabled";
        },

        //Change option effect
        []{
            egoboo_config_t::get().graphic_hd_textures_enable.setValue(!egoboo_config_t::get().graphic_hd_textures_enable.getValue());
        }
    );    

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
    std::shared_ptr<Label> welcomeLabel = std::make_shared<Label>("Change video settings here");
    welcomeLabel->setPosition(backButton->getX() + backButton->getWidth() + 40,
        SCREEN_HEIGHT - SCREEN_HEIGHT/60 - welcomeLabel->getHeight());
    addComponent(welcomeLabel);
}

int VideoOptionsScreen::addOptionsButton(int xPos, int yPos, const std::string &label, std::function<std::string()> labelFunction, std::function<void()> onClickFunction, bool enabled)
{
    std::shared_ptr<Label> optionLabel = std::make_shared<Label>(label + ": ");
    optionLabel->setPosition(xPos, yPos);
    addComponent(optionLabel);

    std::shared_ptr<Button> optionButton = std::make_shared<Button>(labelFunction());
    optionButton->setSize(150, 30);
    optionButton->setPosition(xPos+250, optionLabel->getY());
    optionButton->setOnClickFunction(
        [optionButton, onClickFunction, labelFunction]{
            onClickFunction();
            optionButton->setText(labelFunction());
        });
    optionButton->setEnabled(enabled);
    addComponent(optionButton); 

    return optionButton->getHeight() + 5;
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
    sdl_scr.window->setGrabEnabled(false);
    _gameEngine->enableMouseCursor();
}

void VideoOptionsScreen::addResolutionButton(int width, int height)
{
    std::shared_ptr<Button> resolutionButton = std::make_shared<Button>(std::to_string(width) + "x" + std::to_string(height));

    resolutionButton->setSize(200, 30);
    resolutionButton->setOnClickFunction(
        [width, height, resolutionButton, this]
        {

            // Set new resolution requested.
            egoboo_config_t::get().graphic_resolution_horizontal.setValue(width);
            egoboo_config_t::get().graphic_resolution_vertical.setValue(height);

            // Enable all resolution buttons except the one we just selected.
            for(const std::shared_ptr<GUIComponent> &button : _resolutionList->iterator())
            {
                button->setEnabled(true);
            }
            resolutionButton->setEnabled(false);
        }
    );
    _resolutionList->addComponent(resolutionButton);

    //If this is our current resolution then make it greyed out
    if (egoboo_config_t::get().graphic_resolution_horizontal.getValue() == width &&
        egoboo_config_t::get().graphic_resolution_vertical.getValue() == height)
    {
        resolutionButton->setEnabled(false);
    }
}

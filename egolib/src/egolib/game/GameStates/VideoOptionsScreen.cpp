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

/// @file egolib/game/GameStates/VideoOptionsScreen.cpp
/// @details Video settings
/// @author Johan Jansen

#include "egolib/game/GameStates/VideoOptionsScreen.hpp"
#include "egolib/game/GUI/Button.hpp"
#include "egolib/game/GUI/Image.hpp"
#include "egolib/game/GUI/Label.hpp"
#include "egolib/game/GUI/ScrollableList.hpp"

VideoOptionsScreen::VideoOptionsScreen() :
    _resolutionList(std::make_shared<Ego::GUI::ScrollableList>())
{
    auto background = std::make_shared<Ego::GUI::Image>("mp_data/menu/menu_video");

    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    // calculate the centered position of the background
    background->setSize(Vector2f(background->getTextureWidth() * 0.75f, background->getTextureHeight() * 0.75f));
    background->setPosition(Point2f(SCREEN_WIDTH- background->getWidth(), SCREEN_HEIGHT - background->getHeight()));
    addComponent(background);

    //Resolution
    auto resolutionLabel = std::make_shared<Ego::GUI::Label>("Resolution");
    resolutionLabel->setPosition(Point2f(20, 5));
    addComponent(resolutionLabel);

    _resolutionList->setSize(Vector2f(SCREEN_WIDTH/3, SCREEN_HEIGHT/2));
    _resolutionList->setPosition(resolutionLabel->getPosition() + Vector2f(0, resolutionLabel->getHeight()));
    addComponent(_resolutionList);

    //Build list of available resolutions
    std::unordered_set<uint32_t> resolutions;
    const auto& displays = Ego::GraphicsSystemNew::get().getDisplays();
    auto displayIt = std::find_if(displays.cbegin(), displays.cend(), [](const auto& display) { return display->isPrimaryDisplay(); });
    if (displayIt == displays.cend())
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "unable to get primary display");
    }
    for (const auto &displayMode : (*displayIt)->getDisplayModes())
    {
        //Skip duplicate resolutions (32-bit, 24-bit, 16-bit etc.)
        if(resolutions.find(displayMode->getHorizontalResolution() | displayMode->getVerticalResolution() << 16) != resolutions.end()) {
            continue;
        }

        addResolutionButton(displayMode->getHorizontalResolution(), displayMode->getVerticalResolution());
        resolutions.insert(displayMode->getHorizontalResolution() | displayMode->getVerticalResolution() << 16);
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
            SDL_SetWindowFullscreen(Ego::GraphicsSystem::get().window->get(), egoboo_config_t::get().graphic_fullscreen.getValue() ? SDL_WINDOW_FULLSCREEN : 0);
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
            if(egoboo_config_t::get().graphic_textureFilter_mipMapFilter.getValue() == idlib::texture_filter_method::linear) return "High";
            if(egoboo_config_t::get().graphic_textureFilter_minFilter.getValue() == idlib::texture_filter_method::linear) return "Medium";
            if(egoboo_config_t::get().graphic_textureFilter_minFilter.getValue() == idlib::texture_filter_method::nearest) return "Low";
            return "Unknown";
        },

        //Change option effect
        []{
            //Medium (Trilinear filtering)
            if(egoboo_config_t::get().graphic_textureFilter_minFilter.getValue() == idlib::texture_filter_method::nearest) {
                egoboo_config_t::get().graphic_textureFilter_minFilter.setValue(idlib::texture_filter_method::linear);
                egoboo_config_t::get().graphic_textureFilter_magFilter.setValue(idlib::texture_filter_method::linear);
                egoboo_config_t::get().graphic_textureFilter_mipMapFilter.setValue(idlib::texture_filter_method::none);
            }

            //High (Trilinear mipmap filtering)
            else if(egoboo_config_t::get().graphic_textureFilter_mipMapFilter.getValue() == idlib::texture_filter_method::none) {
                egoboo_config_t::get().graphic_textureFilter_minFilter.setValue(idlib::texture_filter_method::linear);
                egoboo_config_t::get().graphic_textureFilter_magFilter.setValue(idlib::texture_filter_method::linear);
                egoboo_config_t::get().graphic_textureFilter_mipMapFilter.setValue(idlib::texture_filter_method::linear);
            }

            //Low - linear filtering filtering
            else {
                egoboo_config_t::get().graphic_textureFilter_minFilter.setValue(idlib::texture_filter_method::nearest);
                egoboo_config_t::get().graphic_textureFilter_magFilter.setValue(idlib::texture_filter_method::nearest);
                egoboo_config_t::get().graphic_textureFilter_mipMapFilter.setValue(idlib::texture_filter_method::none);
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

    // Back button
    auto backButton = std::make_shared<Ego::GUI::Button>("Back", SDLK_ESCAPE);
    backButton->setPosition(Point2f(20, SCREEN_HEIGHT-80));
    backButton->setSize(Vector2f(200, 30));
    backButton->setOnClickFunction(
    [this]{
        endState();

        // save the setup file
        Ego::Setup::upload(egoboo_config_t::get());
    });
    addComponent(backButton);

    //Add version label and copyright text
    auto welcomeLabel = std::make_shared<Ego::GUI::Label>("Change video settings here");
    welcomeLabel->setPosition(Point2f(backButton->getX() + backButton->getWidth() + 40,
                              SCREEN_HEIGHT - SCREEN_HEIGHT/60 - welcomeLabel->getHeight()));
    addComponent(welcomeLabel);
}

int VideoOptionsScreen::addOptionsButton(int xPos, int yPos, const std::string &label, std::function<std::string()> labelFunction, std::function<void()> onClickFunction, bool enabled)
{
    auto optionLabel = std::make_shared<Ego::GUI::Label>(label + ": ");
    optionLabel->setPosition(Point2f(xPos, yPos));
    addComponent(optionLabel);

    auto optionButton = std::make_shared<Ego::GUI::Button>(labelFunction());
    optionButton->setSize(Vector2f(150, 30));
    optionButton->setPosition(Point2f(xPos + 250, optionLabel->getY()));
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

void VideoOptionsScreen::drawContainer(Ego::GUI::DrawingContext& drawingContext)
{

}

void VideoOptionsScreen::beginState()
{
    // menu settings
    Ego::GraphicsSystem::get().window->setGrabEnabled(false);
    _gameEngine->enableMouseCursor();
}

void VideoOptionsScreen::addResolutionButton(int width, int height)
{
    auto resolutionButton = std::make_shared<Ego::GUI::Button>(std::to_string(width) + "x" + std::to_string(height));

    resolutionButton->setSize(Vector2f(200, 30));
    resolutionButton->setOnClickFunction(
        [width, height, resolutionButton, this]
        {

            // Set new resolution requested.
            egoboo_config_t::get().graphic_resolution_horizontal.setValue(width);
            egoboo_config_t::get().graphic_resolution_vertical.setValue(height);

            // Enable all resolution buttons except the one we just selected.
            for(const auto& button : _resolutionList->iterator())
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

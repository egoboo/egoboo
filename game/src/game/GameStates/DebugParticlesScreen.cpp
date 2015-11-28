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

/// @file game/GameStates/DebugParticlesScreen.cpp
/// @details Options Screen menu
/// @author Johan Jansen

#include "game/GameStates/DebugParticlesScreen.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/Entities/_Include.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/ScrollableList.hpp"

DebugParticlesScreen::DebugParticlesScreen()
{
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    //Add the buttons
    std::shared_ptr<Button> backButton = std::make_shared<Button>("Back", SDLK_ESCAPE);
    backButton->setPosition(20, SCREEN_HEIGHT-80);
    backButton->setSize(200, 30);
    backButton->setOnClickFunction(
    [this]{
        endState();
    });
    addComponent(backButton);

    std::shared_ptr<Label> title = std::make_shared<Label>("==PARTICLE DEBUG SCREEN==");
    title->setPosition(10, 10);
    addComponent(title);

    std::shared_ptr<Label> usage = std::make_shared<Label>("Particle used: " + std::to_string(ParticleHandler::get().getCount()) + "/" + std::to_string(ParticleHandler::get().getDisplayLimit()));
    usage->setPosition(10, title->getY() + title->getHeight());
    addComponent(usage);

    std::shared_ptr<Label> invalid = std::make_shared<Label>("Invalid active particles: ");
    invalid->setPosition(10, usage->getY() + usage->getHeight());
    invalid->setColor(Ego::Math::Colour4f::red());
    addComponent(invalid);

    //Count who is using all the particles
    std::unordered_map<PIP_REF, size_t> usageCount;
    std::unordered_map<PIP_REF, size_t> terminatedCount;
    size_t invalidParticles = 0;
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        if(particle->getProfileID() == INVALID_PIP_REF || !ParticleProfileSystem::get().isLoaded(particle->getProfileID())) {
            invalidParticles++;
			Log::get().warn("Invalid particle with ID: %d (profile=%d)\n", particle->getProfileID(), particle->getSpawnerProfile());
            continue;
        }

        if(particle->isTerminated()) {
            terminatedCount[particle->getProfileID()] += 1;
        }
        else {
            usageCount[particle->getProfileID()] += 1;
        }
    }

    std::shared_ptr<ScrollableList> scrollableList = std::make_shared<ScrollableList>();
    scrollableList->setPosition(invalid->getX(), invalid->getY() + invalid->getHeight() + 20);
    scrollableList->setSize(SCREEN_WIDTH-50, SCREEN_HEIGHT*0.75f-scrollableList->getY());
    addComponent(scrollableList);

    for(const auto &element : usageCount)
    {
        const std::shared_ptr<pip_t> &particleProfile = ParticleProfileSystem::get().get_ptr(element.first);

        std::stringstream labelString;
        labelString << element.second << " particle" << ((element.second > 0) ? "s: " : ":");
        labelString << particleProfile->getName();

        std::shared_ptr<Label> label = std::make_shared<Label>(labelString.str());
        label->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_DEBUG));
        scrollableList->addComponent(label);
    }

    for(const auto &element : terminatedCount)
    {
        const std::shared_ptr<pip_t> &particleProfile = ParticleProfileSystem::get().get_ptr(element.first);

        std::stringstream labelString;
        labelString << element.second << " terminated particle" << ((element.second > 0) ? "s: " : ":");
        labelString << particleProfile->getName();

        std::shared_ptr<Label> label = std::make_shared<Label>(labelString.str());
        label->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_DEBUG));
        label->setColor(Ego::Math::Colour4f::red());
        scrollableList->addComponent(label);
    }


    invalid->setText("Invalid active particles: " + std::to_string(invalidParticles));
    scrollableList->forceUpdate();
}

void DebugParticlesScreen::update()
{
}

void DebugParticlesScreen::beginState()
{
    // menu settings
    SDL_SetWindowGrab(sdl_scr.window, SDL_FALSE);
    _gameEngine->enableMouseCursor();
}

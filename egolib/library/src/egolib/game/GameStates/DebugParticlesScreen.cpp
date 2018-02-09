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

/// @file egolib/game/GameStates/DebugParticlesScreen.cpp
/// @details Options Screen menu
/// @author Johan Jansen

#include "egolib/game/GameStates/DebugParticlesScreen.hpp"
#include "egolib/game/Core/GameEngine.hpp"
#include "egolib/Entities/_Include.hpp"
#include "egolib/game/GUI/Button.hpp"
#include "egolib/game/GUI/Label.hpp"
#include "egolib/game/GUI/ScrollableList.hpp"

DebugParticlesScreen::DebugParticlesScreen()
{
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    // Add the buttons.
    auto backButton = std::make_shared<Ego::GUI::Button>("Back", SDLK_ESCAPE);
    backButton->setPosition({ 20, SCREEN_HEIGHT - 80 });
    backButton->setSize({ 200, 30 });
    backButton->setOnClickFunction(
    [this]{
        endState();
    });
    addComponent(backButton);

    auto title = std::make_shared<Ego::GUI::Label>("==PARTICLE DEBUG SCREEN==");
    title->setPosition({ 10, 10 });
    addComponent(title);

    auto usage = std::make_shared<Ego::GUI::Label>("Particle used: " + std::to_string(ParticleHandler::get().getCount()) + "/" + std::to_string(ParticleHandler::get().getDisplayLimit()));
    usage->setPosition({ 10, title->getY() + title->getHeight() });
    addComponent(usage);

    auto invalid = std::make_shared<Ego::GUI::Label>("Invalid active particles: ");
    invalid->setPosition({ 10, usage->getY() + usage->getHeight() });
    invalid->setColour(Ego::Colour4f::red());
    addComponent(invalid);

    //Count who is using all the particles
    std::unordered_map<PIP_REF, size_t> usageCount;
    std::unordered_map<PIP_REF, size_t> terminatedCount;
    size_t invalidParticles = 0;
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        if(particle->getProfileID() == INVALID_PIP_REF || !ProfileSystem::get().ParticleProfileSystem.isLoaded(particle->getProfileID())) {
            invalidParticles++;
			Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "invalid particle with particle profile reference ", particle->getProfileID(), " and spawner object profile reference ", particle->getSpawnerProfile());
            continue;
        }

        if(particle->isTerminated()) {
            terminatedCount[particle->getProfileID()] += 1;
        }
        else {
            usageCount[particle->getProfileID()] += 1;
        }
    }

    auto scrollableList = std::make_shared<Ego::GUI::ScrollableList>();
    scrollableList->setPosition(invalid->getPosition() + Ego::Vector2f(0, invalid->getHeight() + 20));
    scrollableList->setSize({ SCREEN_WIDTH - 50, SCREEN_HEIGHT*0.75f - scrollableList->getY() });
    addComponent(scrollableList);

    for(const auto &element : usageCount)
    {
        const std::shared_ptr<ParticleProfile> &particleProfile = ProfileSystem::get().ParticleProfileSystem.get_ptr(element.first);

        std::stringstream labelString;
        labelString << element.second << " particle" << ((element.second > 0) ? "s: " : ":");
        labelString << particleProfile->getName();

        auto label = std::make_shared<Ego::GUI::Label>(labelString.str());
        label->setFont(_gameEngine->getUIManager()->getFont(Ego::GUI::UIManager::FONT_DEBUG));
        scrollableList->addComponent(label);
    }

    for(const auto &element : terminatedCount)
    {
        const std::shared_ptr<ParticleProfile> &particleProfile = ProfileSystem::get().ParticleProfileSystem.get_ptr(element.first);

        std::stringstream labelString;
        labelString << element.second << " terminated particle" << ((element.second > 0) ? "s: " : ":");
        labelString << particleProfile->getName();

        auto label = std::make_shared<Ego::GUI::Label>(labelString.str());
        label->setFont(_gameEngine->getUIManager()->getFont(Ego::GUI::UIManager::FONT_DEBUG));
        label->setColour(Ego::Colour4f::red());
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
    Ego::GraphicsSystem::get().window->grab_enabled(false);
    _gameEngine->enableMouseCursor();
}

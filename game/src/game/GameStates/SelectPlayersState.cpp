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

/// @file game/GameStates/SelectPlayersState.cpp
/// @details The Main Menu of the game, the first screen presented to the players
/// @author Johan Jansen

#include "game/GameStates/SelectPlayersState.hpp"
#include "game/GameStates/SelectModuleState.hpp"
#include "game/GameStates/SelectCharacterState.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/Label.hpp"
#include "game/game.h"	//only for MAX_IMPORTS_PER_OBJECT constant
#include "game/GameStates/LoadPlayerElement.hpp"

SelectPlayersState::SelectPlayersState()
    : _playerButtons(),
      _continueButton(std::make_shared<Ego::GUI::Button>("Select Module", SDLK_RETURN))
{
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    //Load background
    auto background = std::make_shared<Ego::GUI::Image>("mp_data/menu/menu_selectplayers");
    background->setSize(Vector2f(std::min(SCREEN_WIDTH, background->getTextureWidth()), std::min(SCREEN_HEIGHT, background->getTextureHeight())));
    background->setPosition(Point2f(SCREEN_WIDTH-background->getWidth(), SCREEN_HEIGHT-background->getHeight()));
    addComponent(background);

    //Add the buttons
    int yOffset = SCREEN_HEIGHT - 80;
    auto backButton = std::make_shared<Ego::GUI::Button>("Back", SDLK_ESCAPE);
    backButton->setPosition(Point2f(20, yOffset));
    backButton->setSize(Vector2f(200, 30));
    backButton->setOnClickFunction(
        [this]{
        endState();
    });
    addComponent(backButton);

    yOffset -= backButton->getHeight() + 10;

    _continueButton->setPosition(Point2f(20, yOffset));
    _continueButton->setSize(Vector2f(200, 30));
    _continueButton->setOnClickFunction(
        [this]{
        //Build list of all valid selected players
        std::list<std::string> selectedPlayersResult;
        for (const std::shared_ptr<LoadPlayerElement> &player : _selectedPlayers) {
            if (player != nullptr) {
                selectedPlayersResult.push_back(player->getProfile()->getPathname());
            }
        }

        //Do the select module screen next
        _gameEngine->pushGameState(std::make_shared<SelectModuleState>(selectedPlayersResult));
    });
    addComponent(_continueButton);
    _continueButton->setEnabled(false);

    //Tell them what this screen is all about
    auto infoText = std::make_shared<Ego::GUI::Label>("Select a character for each player that is going ot play.");
    infoText->setPosition(Point2f(150, SCREEN_HEIGHT - 40));
    addComponent(infoText);

    //Players Label
    auto playersLabel = std::make_shared<Ego::GUI::Label>("PLAYERS");
    playersLabel->setPosition(Point2f(20, 20));
    addComponent(playersLabel);

    auto characterLabel = std::make_shared<Ego::GUI::Label>("CHARACTER");
    characterLabel->setPosition(Point2f(SCREEN_WIDTH / 3, 20));
    addComponent(characterLabel);

    yOffset = playersLabel->getY() + playersLabel->getHeight() + 20;
    for (int i = 0; i < 4; ++i) {
        auto playerLabel = std::make_shared<Ego::GUI::Label>(std::string("Player ") + std::to_string(i + 1));
        playerLabel->setPosition(Point2f(40, yOffset));
        addComponent(playerLabel);

        auto playerButton = std::make_shared<Ego::GUI::Button>("Not playing");
        playerButton->setSize(Vector2f(200, 42));
        playerButton->setPosition(Point2f(SCREEN_WIDTH / 3, yOffset - 10));
        playerButton->setOnClickFunction(
            [this, i]{
            _gameEngine->pushGameState(std::make_shared<SelectCharacterState>(_selectedPlayers[i]));
        }
        );
        addComponent(playerButton);

        yOffset += playerLabel->getHeight() + 50;

        //Initially select no character for each player
        _selectedPlayers.push_back(nullptr);
        _playerButtons.push_back(playerButton);
    }

    //Mark all loadable characters initially as unselected
    for (const std::shared_ptr<LoadPlayerElement> &save : ProfileSystem::get().getSavedPlayers())
    {
        save->setSelected(false);
    }
}

void SelectPlayersState::update()
{
}

void SelectPlayersState::drawContainer(Ego::GUI::DrawingContext& drawingContext)
{

}

void SelectPlayersState::beginState()
{
    // menu settings
    Ego::GraphicsSystem::window->setGrabEnabled(false);

    // Begin the main menu song again (in case we just returned from winning a module)
    playMainMenuSong();

    // Update player selection and enable continue button if at least 
    // one player has selected a character
    _continueButton->setEnabled(false);
    for (size_t i = 0; i < _selectedPlayers.size(); ++i) {
        if (_selectedPlayers[i] != nullptr) {
            _continueButton->setEnabled(true);
            _playerButtons[i]->setText(_selectedPlayers[i]->getName());
        }
        else {
            _playerButtons[i]->setText("Not playing");
        }
    }
}

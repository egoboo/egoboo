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

/// @file game/GameStates/SelectCharacterState.cpp
/// @details Select which character a given player is playing
/// @author Johan Jansen

#include "game/GameStates/SelectCharacterState.hpp"
#include "game/GameStates/SelectModuleState.hpp"
#include "game/GameStates/LoadPlayerElement.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/IconButton.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/ScrollableList.hpp"

SelectCharacterState::SelectCharacterState(std::shared_ptr<LoadPlayerElement> &selectedCharacter)
{
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

	//Load background
	std::shared_ptr<Image> background = std::make_shared<Image>("mp_data/menu/menu_selectplayers");
    background->setSize(std::min(SCREEN_WIDTH, background->getTextureWidth()), std::min(SCREEN_HEIGHT, background->getTextureHeight()));
	background->setPosition(SCREEN_WIDTH-background->getWidth(), SCREEN_HEIGHT-background->getHeight());
	addComponent(background);

	//Add the buttons
	int yOffset = SCREEN_HEIGHT-80;
	std::shared_ptr<Button> backButton = std::make_shared<Button>("No character", SDLK_ESCAPE);
	backButton->setPosition(20, yOffset);
	backButton->setSize(200, 30);
	backButton->setOnClickFunction(
	[this, &selectedCharacter]() mutable {
		//Unselect any previous selection first
		if(selectedCharacter != nullptr) {
			selectedCharacter->setSelected(false);
		}

		//No character was selected and return to previous state
		selectedCharacter = nullptr;
		endState();
	});
	addComponent(backButton);

	yOffset -= backButton->getHeight() + 10;

	std::shared_ptr<Button> selectButton = std::make_shared<Button>("Select Character", SDLK_RETURN);
	selectButton->setPosition(20, yOffset);
	selectButton->setSize(200, 30);
	selectButton->setOnClickFunction(
	[this] {
		//Accept our selection and return to previous state
		endState();
	});
	selectButton->setVisible(false);
	addComponent(selectButton);

	//Tell them what this screen is all about
	std::shared_ptr<Label> infoText = std::make_shared<Label>("Select your character\nUse the mouse wheel to scroll.");
	infoText->setPosition(150, SCREEN_HEIGHT - 50);
	addComponent(infoText);

	//Players Label
	std::shared_ptr<Label> playersLabel = std::make_shared<Label>("CHARACTERS");
	playersLabel->setPosition(20, 20);
	addComponent(playersLabel);

	std::shared_ptr<ScrollableList> scrollableList = std::make_shared<ScrollableList>();
	scrollableList->setSize(300, _gameEngine->getUIManager()->getScreenHeight() - playersLabel->getY() - 150);
	scrollableList->setPosition(playersLabel->getX() + 20, playersLabel->getY() + playersLabel->getHeight() + 20);

	//Make a button for each loadable character
    for (const std::shared_ptr<LoadPlayerElement> &character : ProfileSystem::get().getSavedPlayers())
	{
		std::shared_ptr<Button> characterButton = std::make_shared<IconButton>(character->getName(), character->getIcon());
		characterButton->setSize(200, 40);

		//Check if this already has been selected by another player
		if(character->isSelected() && character != selectedCharacter) {
			characterButton->setEnabled(false);
		}

		else {
			characterButton->setOnClickFunction(
				[character, &selectedCharacter, selectButton]() mutable {
					//Unselect any previous selection first
					if(selectedCharacter != nullptr) {
						selectedCharacter->setSelected(false);
					}

					//This is the new character selected by this player
					selectedCharacter = character;
					selectedCharacter->setSelected(true);

					//Show the button that lets them accept their selection now
					selectButton->setVisible(true);
				});
		}

		scrollableList->addComponent(characterButton);
	}
	
	scrollableList->forceUpdate();
	addComponent(scrollableList);
}

void SelectCharacterState::update()
{
}

void SelectCharacterState::drawContainer()
{

}

void SelectCharacterState::beginState()
{
    // menu settings
    SDL_SetWindowGrab(sdl_scr.window, SDL_FALSE);
}

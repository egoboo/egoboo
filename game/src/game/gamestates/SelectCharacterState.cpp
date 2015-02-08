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

/// @file game/gamestates/SelectCharacterState.cpp
/// @details Select which character a given player is playing
/// @author Johan Jansen

#include "game/gamestates/SelectCharacterState.hpp"
#include "game/gamestates/SelectModuleState.hpp"
#include "game/gamestates/LoadPlayerElement.hpp"
#include "game/core/GameEngine.hpp"
#include "game/ui.h"
#include "game/gui/Button.hpp"
#include "game/gui/IconButton.hpp"
#include "game/gui/Image.hpp"
#include "game/gui/Label.hpp"

SelectCharacterState::SelectCharacterState(const std::vector<std::shared_ptr<LoadPlayerElement>> &loadPlayerList, std::shared_ptr<LoadPlayerElement> &selectedCharacter) :
	_loadPlayerList(loadPlayerList)
{
	//Load background
	std::shared_ptr<Image> background = std::make_shared<Image>("mp_data/menu/menu_selectplayers");
	background->setPosition(0, 0);
	background->setSize(GFX_WIDTH, GFX_HEIGHT);
	addComponent(background);

	//Add the buttons
	int yOffset = GFX_HEIGHT-80;
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
	infoText->setPosition(150, GFX_HEIGHT - 50);
	addComponent(infoText);

	//Players Label
	std::shared_ptr<Label> playersLabel = std::make_shared<Label>("CHARACTERS");
	playersLabel->setPosition(20, 20);
	addComponent(playersLabel);

	yOffset = playersLabel->getY() + playersLabel->getHeight() + 20;

	//Make a button for each loadable character
	for(const std::shared_ptr<LoadPlayerElement> &character : loadPlayerList)
	{
		std::shared_ptr<Button> characterButton = std::make_shared<IconButton>(character->getName(), character->getIcon());
		characterButton->setSize(200, 40);
		characterButton->setPosition(playersLabel->getX(), yOffset);

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

		addComponent(characterButton);
		yOffset += characterButton->getHeight() + 20;
	}
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
    SDL_WM_GrabInput(SDL_GRAB_OFF);
}

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

/// @file game/GameStates/SelectModuleState.cpp
/// @details The Main Menu of the game, the first screen presented to the players
/// @author Johan Jansen

#include "game/Core/GameEngine.hpp"
#include "egolib/Audio/AudioSystem.hpp"
#include "game/GameStates/SelectModuleState.hpp"
#include "game/GameStates/LoadingState.hpp"
#include "game/game.h"
#include "game/GUI/Button.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/ModuleSelector.hpp"
#include "game/Profiles/_Include.hpp"

SelectModuleState::SelectModuleState() : SelectModuleState( std::list<std::string>() )
{
	//ctor
}

SelectModuleState::SelectModuleState(const std::list<std::string> &playersToLoad) :
	_onlyStarterModules(playersToLoad.empty()),
	_validModules(),
	_background(std::make_shared<Image>()),
	_filterButton(std::make_shared<Button>("All Modules", SDLK_TAB)),
	_chooseModule(std::make_shared<Button>("Choose Module", SDLK_RETURN)),
	_moduleSelector(std::make_shared<ModuleSelector>(_validModules)),
	_moduleFilter(FILTER_OFF),
	_selectedPlayerList(playersToLoad)
{
	const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
	const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

	//Set default filter
	if(_onlyStarterModules)
	{
		setModuleFilter(FILTER_STARTER);
	}
	else
	{
		setModuleFilter(FILTER_OFF);
	}
	addComponent(_background);

	//Add the buttons
	_chooseModule = std::make_shared<Button>("Choose Module", SDLK_RETURN);
	_chooseModule->setPosition(_moduleSelector->getX() + _moduleSelector->getWidth() + 20, SCREEN_HEIGHT / 2 + 20);
	_chooseModule->setSize(200, 30);
	_chooseModule->setEnabled(false);
	_chooseModule->setOnClickFunction(
	[this]{
		if(_moduleSelector->getSelectedModule())
		{
			_gameEngine->setGameState(std::make_shared<LoadingState>(_moduleSelector->getSelectedModule(), _selectedPlayerList));
		}
	});
	addComponent(_chooseModule);

	std::shared_ptr<Button> backButton = std::make_shared<Button>("Back", SDLK_ESCAPE);
	backButton->setPosition(_moduleSelector->getX() + _moduleSelector->getWidth() + 20, _chooseModule->getY() + 50);
	backButton->setSize(200, 30);
	backButton->setOnClickFunction(
		[this]{
			this->endState();
		});
	addComponent(backButton);

	//Add the module selector
	addComponent(_moduleSelector);

	//Only draw module filter for non-starter games
	if(!_onlyStarterModules)
	{
		_filterButton->setPosition(10 + SCREEN_WIDTH/2, 30);
		_filterButton->setSize(200, 30);
		_filterButton->setOnClickFunction(
			[this]{
				//Set next module filter (wrap around using modulo)
				setModuleFilter(static_cast<ModuleFilter>( (_moduleFilter+1) % NR_OF_MODULE_FILTERS) );
			});
		addComponent(_filterButton);		
	}

	//Help text
	std::shared_ptr<Label> infoText = std::make_shared<Label>("Press an icon to select a game.\n"
															  "Use the mouse wheel or the <- and -> buttons to scroll.");
	infoText->setPosition(200, SCREEN_HEIGHT - 70);
	addComponent(infoText);

}

void SelectModuleState::setModuleFilter(const ModuleFilter filter)
{
	_moduleFilter = filter;

	//Load list of valid modules
	_validModules.clear();

	//Build list of valid modules
	for(const std::shared_ptr<ModuleProfile> &module : _profileSystem.getModuleProfiles())
	{
		// if this module is not valid given the game options and the
		// selected players, skip it
		if (!module->isModuleUnlocked()) continue;

		if (_onlyStarterModules && module->isStarterModule())
		{
		    // starter module
		    _validModules.push_back(module);
		}
		else
		{
		    if (FILTER_OFF != _moduleFilter && static_cast<ModuleFilter>(module->getModuleType()) != _moduleFilter ) continue;
		    if ( _selectedPlayerList.size() > module->getImportAmount() ) continue;
		    if ( _selectedPlayerList.size() < module->getMinPlayers() ) continue;
		    if ( _selectedPlayerList.size() > module->getMaxPlayers() ) continue;

		    // regular module
		    _validModules.push_back(module);
		}
	}

	//Sort modules by difficulity
	std::sort(_validModules.begin(), _validModules.end(), [](const std::shared_ptr<ModuleProfile> &a, const std::shared_ptr<ModuleProfile> &b) {
		return a->getRank() < b->getRank();
	});

	//Notify the module selector that the list of available modules has changed
	_moduleSelector->notifyModuleListUpdated();

	// load background depending on current filter
	switch (_moduleFilter)
	{
		case FILTER_STARTER:	
			_background->setImage("mp_data/menu/menu_advent");
			_filterButton->setText("Starter Modules");
		break;
	    
	    case FILTER_MAIN: 		
	    	_background->setImage("mp_data/menu/menu_draco");
			_filterButton->setText("Main Quest");
		break;
	    
	    case FILTER_SIDE_QUEST: 		
	    	_background->setImage("mp_data/menu/menu_sidequest");
			_filterButton->setText("Side Quest");
		break;
	    
	    case FILTER_TOWN: 		
	    	_background->setImage("mp_data/menu/menu_town");
			_filterButton->setText("Towns and Cities");
		break;
	    
	    case FILTER_FUN:  		
	    	_background->setImage("mp_data/menu/menu_funquest");
			_filterButton->setText("Fun Modules");
		break;

	    default:
	    case FILTER_OFF: 		
	    	_background->setImage("mp_data/menu/menu_allquest");  
			_filterButton->setText("All Modules");
	    break;
	}

	//Place background in bottom right corner
	_background->setSize(_background->getTextureWidth(), _background->getTextureHeight());
	_background->setPosition((GFX_WIDTH/2) - (_background->getWidth()/2), GFX_HEIGHT - _background->getHeight());
}

void SelectModuleState::update()
{
	//Only enable choose module button once we have a valid module selection
	_chooseModule->setEnabled(_moduleSelector->getSelectedModule() != nullptr);
}

void SelectModuleState::drawContainer()
{

}

void SelectModuleState::beginState()
{
	// menu settings
    SDL_WM_GrabInput( SDL_GRAB_OFF );
}

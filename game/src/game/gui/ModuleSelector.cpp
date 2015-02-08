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

/// @file game/gui/ModuleSelector.cpp
/// @details GUI widget to select which module to play
/// @author Johan Jansen

#include "game/gui/ModuleSelector.hpp"
#include "game/gui/Button.hpp"
#include "game/profiles/ModuleProfile.hpp"
#include "game/audio/AudioSystem.hpp"
#include "game/ui.h"

ModuleSelector::ModuleSelector(const std::vector<std::shared_ptr<ModuleProfile>> &modules) :
	_startIndex(0),
	_modules(modules),
	_nextModuleButton(std::make_shared<Button>("->", SDLK_RIGHT)),
	_previousModuleButton(std::make_shared<Button>("<-", SDLK_LEFT)),
	_selectedModule(nullptr)
{
    // Figure out at what offset we want to draw the module menu.
    int moduleMenuOffsetX = ( GFX_WIDTH  - 640 ) / 2;
    moduleMenuOffsetX = std::max(0, moduleMenuOffsetX);

    int moduleMenuOffsetY = ( GFX_HEIGHT - 480 ) / 2;
    moduleMenuOffsetY = std::max(0, moduleMenuOffsetY);

	//Set backdrop size and position
	setSize(330, 250);
	setPosition(moduleMenuOffsetX + 21, moduleMenuOffsetY + 173);

	//Next and previous buttons
	_nextModuleButton->setPosition(moduleMenuOffsetX + 590, moduleMenuOffsetY + 74);
	_nextModuleButton->setSize(30, 30);
	_nextModuleButton->setOnClickFunction(
	[this]{
		_startIndex++;
		_nextModuleButton->setEnabled(_modules.size() > _startIndex + 3);
		_previousModuleButton->setEnabled(true);
	});
	addComponent(_nextModuleButton);

	_previousModuleButton->setPosition(moduleMenuOffsetX + 20, moduleMenuOffsetY + 74);
	_previousModuleButton->setSize(30, 30);
	_previousModuleButton->setOnClickFunction(
	[this]{
		_startIndex--;
		_previousModuleButton->setEnabled(_startIndex > 0);
		_nextModuleButton->setEnabled(true);
	});
	addComponent(_previousModuleButton);

	//The three module buttons
	std::shared_ptr<ModuleButton> moduleButtonOne = std::make_shared<ModuleButton>(this, 0);
	moduleButtonOne->setSize(138, 138);
	moduleButtonOne->setPosition(moduleMenuOffsetX + 93, moduleMenuOffsetY + 20);
	moduleButtonOne->setOnClickFunction(
	[this]{
		if(_startIndex + 0 >= _modules.size()) return;
		_selectedModule = _modules[_startIndex + 0];
	});
	addComponent(moduleButtonOne);

	std::shared_ptr<ModuleButton> moduleButtonTwo = std::make_shared<ModuleButton>(this, 1);
	moduleButtonTwo->setSize(138, 138);
	moduleButtonTwo->setPosition(moduleButtonOne->getX() + 20 + moduleButtonOne->getWidth(), moduleButtonOne->getY());
	moduleButtonTwo->setOnClickFunction(
	[this]{
		if(_startIndex + 1 >= _modules.size()) return;
		_selectedModule = _modules[_startIndex + 1];
	});
	addComponent(moduleButtonTwo);

	std::shared_ptr<ModuleButton> moduleButtonThree = std::make_shared<ModuleButton>(this, 2);
	moduleButtonThree->setSize(138, 138);
	moduleButtonThree->setPosition(moduleButtonTwo->getX() + 20 + moduleButtonTwo->getWidth(), moduleButtonTwo->getY());
	moduleButtonThree->setOnClickFunction(
	[this]{
		if(_startIndex + 2 >= _modules.size()) return;
		_selectedModule = _modules[_startIndex + 2];
	});
	addComponent(moduleButtonThree);
}

void ModuleSelector::drawContainer()
{
	const GLXvector4f backDrop = {0.66f, 0.0f, 0.0f, 0.6f};

    //Draw backdrop
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );
    
    GL_DEBUG( glColor4fv )( backDrop );
    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glVertex2f )( getX(), getY() );
        GL_DEBUG( glVertex2f )( getX(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY() );
    }
    GL_DEBUG_END();

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );

    //Module description
    if(_selectedModule != nullptr)
    {
    	std::stringstream buffer;

    	buffer << "Name: " << _selectedModule->getName() << '\n';
    	buffer << "Difficulty: " << _selectedModule->getRank() << '\n';

    	if(_selectedModule->getBase().maxplayers > 1)
    	{
    		if(_selectedModule->getBase().maxplayers == _selectedModule->getBase().minplayers)
    		{
    			buffer << _selectedModule->getBase().minplayers << " Players" << '\n';
    		}
    		else
    		{
    			buffer << std::to_string(_selectedModule->getBase().minplayers) << '-' << std::to_string(_selectedModule->getBase().maxplayers) << " Players" << '\n';
    		}
    	}
    	else if(_selectedModule->isStarterModule())
    	{
    		buffer << "Starter Module" << '\n';
    	}
    	else
    	{
    		buffer << "Single Player" << '\n';
    	}

	    for (size_t i = 0; i < SUMMARYLINES; i++ )
		{
			buffer << _selectedModule->getBase().summary[i] << '\n';
		}    	

    	GL_DEBUG( glColor4fv )( Ego::white_vec );
    	ui_drawTextBox(ui_getFont(), buffer.str().c_str(), getX() + 5, getY() + 5, getWidth() - 10, getHeight() - 10, 20);
    }
}


void ModuleSelector::notifyModuleListUpdated()
{
	_startIndex = 0;
	_nextModuleButton->setEnabled(_modules.size() > 3);
	_previousModuleButton->setEnabled(false);
}

ModuleSelector::ModuleButton::ModuleButton(ModuleSelector *selector, const uint8_t offset) : Button(),
	_moduleSelector(selector),
	_offset(offset)
{
	//ctor
}

void ModuleSelector::ModuleButton::draw()
{
	//Don't do "out of bounds" modules
	if(_moduleSelector->_startIndex + _offset >= _moduleSelector->_modules.size()) {
		return;
	}

    //Draw backdrop
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );
    
    //Determine button color
    if(!isEnabled())
    {
        GL_DEBUG( glColor4fv )( DISABLED_BUTTON_COLOUR );
    }
    else if(_mouseOver)
    {
        GL_DEBUG( glColor4fv )( HOVER_BUTTON_COLOUR );
    }
    else
    {
        GL_DEBUG( glColor4fv )( DEFAULT_BUTTON_COLOUR );
    }

    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glVertex2f )( getX(), getY() );
        GL_DEBUG( glVertex2f )( getX(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY() );
    }
    GL_DEBUG_END();

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );

    //Draw module title image
    ui_drawImage(0, &_moduleSelector->_modules[_moduleSelector->_startIndex + _offset]->getIcon(), getX() + 5, getY() + 5, getWidth()-10, getHeight()-10, nullptr);
}

bool ModuleSelector::notifyMouseScrolled(const int amount)
{
	if(amount < 0 && _startIndex == 0) {
		return false;
	}
	if(amount > 0 && _startIndex >= _modules.size()-3) {
		return false;
	}
	_startIndex = Math::constrain<int>(_startIndex + amount, 0, _modules.size()-3);
	_audioSystem.playSoundFull(_audioSystem.getGlobalSound(GSND_BUTTON_CLICK));
	_nextModuleButton->setEnabled(_startIndex < _modules.size()-3);
	_previousModuleButton->setEnabled(_startIndex > 0);
	return true;
}

const std::shared_ptr<ModuleProfile>& ModuleSelector::getSelectedModule() const
{
	return _selectedModule;	
}

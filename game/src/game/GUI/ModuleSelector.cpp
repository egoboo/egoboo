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

/// @file game/GUI/ModuleSelector.cpp
/// @details GUI widget to select which module to play
/// @author Johan Jansen

#include "game/GUI/ModuleSelector.hpp"
#include "game/GUI/Button.hpp"
#include "game/Profiles/_Include.hpp"

ModuleSelector::ModuleSelector(const std::vector<std::shared_ptr<ModuleProfile>> &modules) :
    _startIndex(0),
    _modules(modules),
    _nextModuleButton(std::make_shared<Button>("->", SDLK_RIGHT)),
    _previousModuleButton(std::make_shared<Button>("<-", SDLK_LEFT)),
    _selectedModule(nullptr)
{
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    // Figure out at what offset we want to draw the module menu.
    int moduleMenuOffsetX = (800 - 640) / 2;
    moduleMenuOffsetX = std::max(0, moduleMenuOffsetX);

    int moduleMenuOffsetY = (600 - 480) / 2;
    moduleMenuOffsetY = std::max(0, moduleMenuOffsetY);

    //Set backdrop size and position
    setSize(30 + SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    setPosition(moduleMenuOffsetX + 21, moduleMenuOffsetY + 173);

    //Next and previous buttons
    _nextModuleButton->setPosition(SCREEN_WIDTH - 50, moduleMenuOffsetY + 74);
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

    const int numberOfModuleButtons = ((_nextModuleButton->getX() - _previousModuleButton->getX() - _previousModuleButton->getWidth()) / 158);

    //Add as many modules as we can fit with current screen width
    for (int i = 0; i < numberOfModuleButtons; ++i) {
        std::shared_ptr<ModuleButton> moduleButton = std::make_shared<ModuleButton>(this, i);
        moduleButton->setSize(138, 138);
        moduleButton->setPosition(moduleMenuOffsetX + 93, moduleMenuOffsetY + 20);
        moduleButton->setOnClickFunction(
            [this, i]{
            if (_startIndex + i >= _modules.size()) return;
            _selectedModule = _modules[_startIndex + i];
        });
        addComponent(moduleButton);

        moduleMenuOffsetX += moduleButton->getWidth() + 20;
    }
}

void ModuleSelector::drawContainer()
{
    const GLXvector4f backDrop = { 0.66f, 0.0f, 0.0f, 0.6f };

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

    // Module description
    if(_selectedModule != nullptr)
    {

        // Draw module Name first
        Ego::Renderer::get().setColour(Ego::Colour4f::white());
        _gameEngine->getUIManager()->getDefaultFont()->drawTextBox(_selectedModule->getName(), getX() + 5, getY() + 5, getWidth() - 10, 20, 25);
        

        // Now difficulty
        if (_selectedModule->getRank() > 0)
        {
            int textWidth, textHeight;
            _gameEngine->getUIManager()->getDefaultFont()->getTextSize("Difficulty: ", &textWidth, &textHeight);
            _gameEngine->getUIManager()->getDefaultFont()->drawTextBox("Difficulty: ", getX() + 5, getY() + 25, getWidth() - 10, textHeight, 25);

            // Draw one skull per rated difficulty
            for (int i = 0; i < _selectedModule->getRank(); ++i)
            {
                draw_icon_texture(TextureManager::get().get_valid_ptr(TX_SKULL), getX() + 5 + textWidth + i*textHeight, getY() + 28, 0xFF, 0, textHeight - 4, true);
            }
        }

        // Module description
        std::stringstream buffer;
        if (_selectedModule->getMaxPlayers() > 1)
        {
            if (_selectedModule->getMaxPlayers() == _selectedModule->getMinPlayers())
            {
                buffer << _selectedModule->getMinPlayers() << " Players" << '\n';
            }
            else
            {
                buffer << std::to_string(_selectedModule->getMinPlayers()) << '-' << std::to_string(_selectedModule->getMaxPlayers()) << " Players" << '\n';
            }
        }
        else if (_selectedModule->isStarterModule())
        {
            buffer << "Starter Module" << '\n';
        }
        else
        {
            buffer << "Single Player" << '\n';
        }

        for (const std::string &line : _selectedModule->getSummary())
        {
            buffer << line << '\n';;
        }

        _gameEngine->getUIManager()->getDefaultFont()->drawTextBox(buffer.str(), getX() + 5, getY() + 45, getWidth() - 10, getHeight() - 50, 25);
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
    // ctor
}

void ModuleSelector::ModuleButton::draw()
{
    // Don't do "out of bounds" modules
    if (_moduleSelector->_startIndex + _offset >= _moduleSelector->_modules.size()) {
        return;
    }

    // Draw backdrop
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );
    
    // Determine button color
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
    _gameEngine->getUIManager()->drawImage(_moduleSelector->_modules[_moduleSelector->_startIndex + _offset]->getIcon(), getX() + 5, getY() + 5, getWidth()-10, getHeight()-10);
}

bool ModuleSelector::notifyMouseScrolled(const int amount)
{
    if (amount < 0 && _startIndex == 0)
    {
        return false;
    }
    if (amount > 0 && _startIndex >= _modules.size() - 3)
    {
        return false;
    }
    _startIndex = Math::constrain<int>(_startIndex + amount, 0, _modules.size() - 3);
    AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_BUTTON_CLICK));
    _nextModuleButton->setEnabled(_startIndex < _modules.size() - 3);
    _previousModuleButton->setEnabled(_startIndex > 0);
    return true;
}

const std::shared_ptr<ModuleProfile>& ModuleSelector::getSelectedModule() const
{
    return _selectedModule;	
}

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

namespace Ego {
namespace GUI {

ModuleSelector::ModuleSelector(const std::vector<std::shared_ptr<ModuleProfile>> &modules) :
    _startIndex(0),
    _modules(modules),
    _nextModuleButton(std::make_shared<Button>("->", SDLK_RIGHT)),
    _previousModuleButton(std::make_shared<Button>("<-", SDLK_LEFT)),
    _selectedModule(nullptr) {
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    const int MODULE_BUTTON_SIZE = Math::constrain((SCREEN_WIDTH) / 6, 138, 256);

    // Figure out at what offset we want to draw the module menu.
    int moduleMenuOffsetX = (800 - 640) / 2;
    moduleMenuOffsetX = std::max(0, moduleMenuOffsetX);

    int moduleMenuOffsetY = (600 - 480) / 2;
    moduleMenuOffsetY = std::max(0, moduleMenuOffsetY);

    //Set backdrop size and position
    setSize(30 + SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    setPosition(moduleMenuOffsetX + 21, moduleMenuOffsetY + MODULE_BUTTON_SIZE + 40);

    //Next and previous buttons
    _nextModuleButton->setPosition(SCREEN_WIDTH - 50, moduleMenuOffsetY + 74);
    _nextModuleButton->setSize(30, 30);
    _nextModuleButton->setOnClickFunction(
        [this] {
        _startIndex++;
        _nextModuleButton->setEnabled(_modules.size() > _startIndex + 3);
        _previousModuleButton->setEnabled(true);
    });
    addComponent(_nextModuleButton);

    _previousModuleButton->setPosition(moduleMenuOffsetX + 20, moduleMenuOffsetY + 74);
    _previousModuleButton->setSize(30, 30);
    _previousModuleButton->setOnClickFunction(
        [this] {
        _startIndex--;
        _previousModuleButton->setEnabled(_startIndex > 0);
        _nextModuleButton->setEnabled(true);
    });
    addComponent(_previousModuleButton);

    const int numberOfModuleButtons = ((_nextModuleButton->getX() - _previousModuleButton->getX() - _previousModuleButton->getWidth() - _nextModuleButton->getWidth()) / (MODULE_BUTTON_SIZE + 20));

    //Add as many modules as we can fit with current screen width
    for (int i = 0; i < numberOfModuleButtons; ++i) {
        std::shared_ptr<ModuleButton> moduleButton = std::make_shared<ModuleButton>(this, i);
        moduleButton->setSize(MODULE_BUTTON_SIZE, MODULE_BUTTON_SIZE);
        moduleButton->setPosition(moduleMenuOffsetX + 93, moduleMenuOffsetY + 20);
        moduleButton->setOnClickFunction(
            [this, i] {
            if (_startIndex + i >= _modules.size()) return;
            _selectedModule = _modules[_startIndex + i];
        });
        addComponent(moduleButton);

        moduleMenuOffsetX += moduleButton->getWidth() + 20;
    }
}

void ModuleSelector::drawContainer() {
    const Math::Colour4f backDrop = {0.66f, 0.0f, 0.0f, 0.6f};

    auto &renderer = Renderer::get();

    //Draw backdrop
    renderer.getTextureUnit().setActivated(nullptr);

    renderer.setColour(backDrop);
    VertexBuffer vb(4, GraphicsUtilities::get<VertexFormat::P2F>());
    {
        struct Vertex {
            float x, y;
        };
        VertexBufferScopedLock vblck(vb);
        Vertex *vertices = vblck.get<Vertex>();
        vertices[0].x = getX(); vertices[0].y = getY();
        vertices[1].x = getX(); vertices[1].y = getY() + getHeight();
        vertices[2].x = getX() + getWidth(); vertices[2].y = getY() + getHeight();
        vertices[3].x = getX() + getWidth(); vertices[3].y = getY();
    }
    renderer.render(vb, PrimitiveType::Quadriliterals, 0, 4);

    // Module description
    if (_selectedModule != nullptr) {

        // Draw module Name first
        renderer.setColour(Colour4f::white());
        _gameEngine->getUIManager()->getDefaultFont()->drawTextBox(_selectedModule->getName(), getX() + 5, getY() + 5, getWidth() - 10, 26, 25);


        // Now difficulty
        if (_selectedModule->getRank() > 0) {
            int textWidth, textHeight;
            _gameEngine->getUIManager()->getDefaultFont()->getTextSize("Difficulty: ", &textWidth, &textHeight);
            _gameEngine->getUIManager()->getDefaultFont()->drawTextBox("Difficulty: ", getX() + 5, getY() + 30, getWidth() - 10, textHeight, 25);

            // Draw one skull per rated difficulty
            const std::shared_ptr<Texture> &skullTexture = TextureManager::get().getTexture("mp_data/skull");
            for (int i = 0; i < _selectedModule->getRank(); ++i) {
                draw_icon_texture(skullTexture, getX() + 5 + textWidth + i*textHeight, getY() + 33, 0xFF, 0, textHeight - 4, true);
            }
        }

        // Module description
        std::stringstream buffer;
        if (_selectedModule->getMaxPlayers() > 1) {
            if (_selectedModule->getMaxPlayers() == _selectedModule->getMinPlayers()) {
                buffer << _selectedModule->getMinPlayers() << " Players" << '\n';
            } else {
                buffer << std::to_string(_selectedModule->getMinPlayers()) << '-' << std::to_string(_selectedModule->getMaxPlayers()) << " Players" << '\n';
            }
        } else if (_selectedModule->isStarterModule()) {
            buffer << "Starter Module" << '\n';
        } else {
            buffer << "Single Player" << '\n';
        }

        for (const std::string &line : _selectedModule->getSummary()) {
            buffer << line << '\n';;
        }

        _gameEngine->getUIManager()->getDefaultFont()->drawTextBox(buffer.str(), getX() + 5, getY() + 55, getWidth() - 10, getHeight() - 60, 25);
    }
}


void ModuleSelector::notifyModuleListUpdated() {
    _startIndex = 0;
    _nextModuleButton->setEnabled(_modules.size() > 3);
    _previousModuleButton->setEnabled(false);
}

ModuleSelector::ModuleButton::ModuleButton(ModuleSelector *selector, const uint8_t offset) : Button(),
_moduleSelector(selector),
_offset(offset) {
    // ctor
}

void ModuleSelector::ModuleButton::draw() {
    // Don't do "out of bounds" modules
    if (_moduleSelector->_startIndex + _offset >= _moduleSelector->_modules.size()) {
        return;
    }

    auto &renderer = Renderer::get();

    // Draw backdrop
    renderer.getTextureUnit().setActivated(nullptr);

    // Determine button color
    if (!isEnabled()) {
        renderer.setColour(DISABLED_BUTTON_COLOUR);
    } else if (_mouseOver) {
        renderer.setColour(HOVER_BUTTON_COLOUR);
    } else {
        renderer.setColour(DEFAULT_BUTTON_COLOUR);
    }

    VertexBuffer vb(4, GraphicsUtilities::get<VertexFormat::P2F>());
    {
        struct Vertex {
            float x, y;
        };
        VertexBufferScopedLock vblck(vb);
        Vertex *vertices = vblck.get<Vertex>();
        vertices[0].x = getX(); vertices[0].y = getY();
        vertices[1].x = getX(); vertices[1].y = getY() + getHeight();
        vertices[2].x = getX() + getWidth(); vertices[2].y = getY() + getHeight();
        vertices[3].x = getX() + getWidth(); vertices[3].y = getY();
    }
    renderer.render(vb, PrimitiveType::Quadriliterals, 0, 4);

    //Draw module title image
    _gameEngine->getUIManager()->drawImage(_moduleSelector->_modules[_moduleSelector->_startIndex + _offset]->getIcon().get(), Point2f(getX() + 5, getY() + 5), Vector2f(getWidth() - 10, getHeight() - 10));
}

bool ModuleSelector::notifyMouseScrolled(const int amount) {
    if (amount < 0 && _startIndex == 0) {
        return false;
    }
    if (amount > 0 && _startIndex >= _modules.size() - 3) {
        return false;
    }
    _startIndex = Math::constrain<int>(_startIndex + amount, 0, _modules.size() - 3);
    AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_BUTTON_CLICK));
    _nextModuleButton->setEnabled(_startIndex < _modules.size() - 3);
    _previousModuleButton->setEnabled(_startIndex > 0);
    return true;
}

const std::shared_ptr<ModuleProfile>& ModuleSelector::getSelectedModule() const {
    return _selectedModule;
}

} // namespace GUI
} // namespace Ego

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
#include "game/GUI/Material.hpp"

namespace Ego {
namespace GUI {

ModuleSelector::ModuleSelector(const std::vector<std::shared_ptr<ModuleProfile>> &modules) :
    _startIndex(0),
    _modules(modules),
    _nextModuleButton(std::make_shared<Button>("->", SDLK_RIGHT)),
    _previousModuleButton(std::make_shared<Button>("<-", SDLK_LEFT)),
    _selectedModule(nullptr) {
    const Vector2f SCREEN_SIZE = Vector2f(_gameEngine->getUIManager()->getScreenWidth(),
        _gameEngine->getUIManager()->getScreenHeight());
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    const Vector2f NAVIGATION_BUTTON_SIZE = Vector2f(30.0f, 30.0f);
    const int MODULE_BUTTON_SIZE = Math::constrain((SCREEN_WIDTH) / 6, 138, 256);

    // Figure out at what offset we want to draw the module menu.
    int moduleMenuOffsetX = (800 - 640) / 2;
    moduleMenuOffsetX = std::max(0, moduleMenuOffsetX);

    int moduleMenuOffsetY = (600 - 480) / 2;
    moduleMenuOffsetY = std::max(0, moduleMenuOffsetY);

    //Set backdrop size and position
    setSize(Vector2f(NAVIGATION_BUTTON_SIZE.x(), 0.0f) + SCREEN_SIZE * 0.5f);
    setPosition(Point2f(moduleMenuOffsetX, moduleMenuOffsetY) + Vector2f(21, MODULE_BUTTON_SIZE + 40));

    //Next and previous buttons
    _nextModuleButton->setPosition(Point2f(SCREEN_WIDTH - 50 - moduleMenuOffsetX - 21, 74 - MODULE_BUTTON_SIZE - 40));
    _nextModuleButton->setSize(NAVIGATION_BUTTON_SIZE);
    _nextModuleButton->setOnClickFunction(
        [this] {
        _startIndex++;
        _nextModuleButton->setEnabled(_modules.size() > _startIndex + 3);
        _previousModuleButton->setEnabled(true);
    });
    addComponent(_nextModuleButton);

    _previousModuleButton->setPosition(Point2f(moduleMenuOffsetX - moduleMenuOffsetX - 21, - MODULE_BUTTON_SIZE - 40) + Vector2f(20, 74));
    _previousModuleButton->setSize(NAVIGATION_BUTTON_SIZE);
    _previousModuleButton->setOnClickFunction(
        [this] {
        _startIndex--;
        _previousModuleButton->setEnabled(_startIndex > 0);
        _nextModuleButton->setEnabled(true);
    });
    addComponent(_previousModuleButton);

    static const int GAP = 20; // The gap between two module buttons.
    const int numberOfModuleButtons = ((_nextModuleButton->getX() - _previousModuleButton->getX() - _previousModuleButton->getWidth() - _nextModuleButton->getWidth()) / (MODULE_BUTTON_SIZE + 20));

    //Add as many modules as we can fit with current screen width
    float offsetx = -21,
        offsety = -MODULE_BUTTON_SIZE - 40;
    for (int i = 0; i < numberOfModuleButtons; ++i) {
        std::shared_ptr<ModuleButton> moduleButton = std::make_shared<ModuleButton>(this, i);
        moduleButton->setSize(Vector2f(MODULE_BUTTON_SIZE, MODULE_BUTTON_SIZE));
        moduleButton->setPosition(Point2f(offsetx, offsety) + Vector2f(93, GAP));
        moduleButton->setOnClickFunction(
            [this, i] {
            if (_startIndex + i >= _modules.size()) return;
            _selectedModule = _modules[_startIndex + i];
        });
        addComponent(moduleButton);

        offsetx += moduleButton->getWidth() + GAP;
    }
}

void ModuleSelector::drawContainer(DrawingContext& drawingContext) {
    const Math::Colour4f backDrop = {0.66f, 0.0f, 0.0f, 0.6f};

    auto &renderer = Renderer::get();

    //Draw backdrop
    std::shared_ptr<Material> material = nullptr;

    material = std::make_shared<Material>(nullptr, backDrop, true);
    material->apply();
    _gameEngine->getUIManager()->drawQuad2d(getBounds());

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

void ModuleSelector::ModuleButton::draw(DrawingContext& drawingContext) {
    // Don't do "out of bounds" modules
    if (_moduleSelector->_startIndex + _offset >= _moduleSelector->_modules.size()) {
        return;
    }

    auto &renderer = Renderer::get();

    // Draw backdrop
    std::shared_ptr<Material> material = nullptr;

    // Determine button color
    if (!isEnabled()) {
        material = std::make_shared<Material>(nullptr, DISABLED_BUTTON_COLOUR, true);
    } else if (_mouseOver) {
        material = std::make_shared<Material>(nullptr, HOVER_BUTTON_COLOUR, true);
    } else {
        material = std::make_shared<Material>(nullptr, DEFAULT_BUTTON_COLOUR, true);
    }
    material->apply();
    _gameEngine->getUIManager()->drawQuad2d(getDerivedBounds());

    //Draw module title image
    material = std::make_shared<Material>(_moduleSelector->_modules[_moduleSelector->_startIndex + _offset]->getIcon().get(), Math::Colour4f::white(), true);
    _gameEngine->getUIManager()->drawImage(getDerivedPosition() + Vector2f(5,5), getSize() - Vector2f(10,10), material);
}

bool ModuleSelector::notifyMouseWheelTurned(const Events::MouseWheelTurnedEventArgs& e) {
    if (e.getDelta().y() < 0 && _startIndex == 0) {
        return false;
    }
    if (e.getDelta().y() > 0 && _startIndex >= _modules.size() - 3) {
        return false;
    }
    _startIndex = Math::constrain<int>(_startIndex + e.getDelta().y(), 0, _modules.size() - 3);
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

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
#pragma once

#include "game/GUI/Button.hpp"
#include "game/GUI/Container.hpp"

// Forward declarations.
class ModuleProfile;

namespace Ego {
namespace GUI {
class Button;
}
}

namespace Ego {
namespace GUI {

class ModuleSelector : public Container {
public:
    ModuleSelector(const std::vector<std::shared_ptr<ModuleProfile>> &modules);
    ~ModuleSelector();

    void draw(DrawingContext& drawingContext) override {
        drawAll(drawingContext);
    }

    void notifyModuleListUpdated();

    bool notifyMouseWheelTurned(const Events::MouseWheelTurnedEventArgs& e) override;

    const std::shared_ptr<ModuleProfile>& getSelectedModule() const;

protected:
    void drawContainer(DrawingContext& drawingContext) override;

    //Local class
    class ModuleButton : public Button {
    public:
        ModuleButton(ModuleSelector* selector, const uint8_t offset);

        void draw(DrawingContext& drawingContext) override;

    private:
        ModuleSelector *_moduleSelector;
        uint8_t _offset;
    };

private:
    size_t _startIndex;
    const std::vector<std::shared_ptr<ModuleProfile>> &_modules;
    std::vector<Id::Connection> _onModuleSelected;

    Id::Connection _onNextModuleButtonClicked;
    std::shared_ptr<Button> _nextModuleButton;
    
    Id::Connection _onPreviousModuleButtonClicked;
    std::shared_ptr<Button> _previousModuleButton;

    std::shared_ptr<ModuleProfile> _selectedModule;
};

} // namespace GUI
} // namespace Ego

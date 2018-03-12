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

/// @file egolib/game/GameStates/OptionsScreen.hpp
/// @details Selection of the different option/settings screens
/// @author Johan Jansen

#pragma once

#include "egolib/game/GameStates/GameState.hpp"

//Forward declarations
namespace Ego::GUI {
class Button;
} // namespace Ego::GUI

class OptionsScreen : public GameState
{
public:
    OptionsScreen();

    void update() override;

    void beginState() override;

    void draw(Ego::GUI::DrawingContext& drawingContext) override {
        drawAll(drawingContext);
    }
protected:
    void drawContainer(Ego::GUI::DrawingContext& drawingContext) override;

private:
    std::forward_list<std::shared_ptr<Ego::GUI::Button>> _slidyButtons;
};

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

/// @file game/gamestates/DebugModuleLoadingState.hpp
/// @details Debugging state where one can debug font layout and rendering
/// @author Johan Jansen, penguinflyer5234

#pragma once

#include "game/GameStates/GameState.hpp"
#include "game/GUI/UIManager.hpp"

namespace Ego {
namespace GUI {
class Button;
} // namespace GUI
} // namespace Ego

class DebugFontRenderingState : public GameState
{
public:
	DebugFontRenderingState();
    
    void update() override {}
    
    void draw(Ego::GUI::DrawingContext& drawingContext) override { drawContainer(drawingContext); }
protected:
    void drawContainer(Ego::GUI::DrawingContext& drawingContext) override {}
    
private:
    void setFont(Ego::GUI::UIManager::UIFontType font);
    void setMaxWidth(int maxWidth);
    void setMaxHeight(int maxHeight);
    
    class DebugLabel;
    
    std::shared_ptr<DebugLabel> _textLabel;
    std::shared_ptr<Ego::GUI::Button> _widthButton;
    std::shared_ptr<Ego::GUI::Button> _heightButton;
    std::shared_ptr<Ego::GUI::Button> _newlineButton;
    
    int _missingSpace;
    int _maxWidth, _maxHeight;
};

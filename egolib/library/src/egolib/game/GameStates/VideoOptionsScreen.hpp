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

/// @file egolib/game/GameStates/VideoOptionsScreen.hpp
/// @details Video settings
/// @author Johan Jansen

#pragma once

#include "egolib/game/GameStates/GameState.hpp"

// Forward declarations.
namespace Ego::GUI {
class ScrollableList;
} // namespace Ego::GUI

class VideoOptionsScreen : public GameState
{
public:
	VideoOptionsScreen();

	void update() override;

	void beginState() override;

    void draw(Ego::GUI::DrawingContext& drawingContext) override {
        drawContainer(drawingContext);
    }
protected:
	void drawContainer(Ego::GUI::DrawingContext& drawingContext) override;

private:
	/**
	* @brief
	*	Add a selectable resolution to the scrollable resolution list
	**/
	void addResolutionButton(int width, int height);

	/**
	* @brief
	*	Add a new graphics options button.
	* @param xPos
	*	x position of button and label
	* @param yPos
	*	y position of button and label
	* @param label
	*	Descriptive string of what this option does (label)
	* @param labelFuction
	*	Function that generates description of current option state
	* @param onClickFunction
	*	Function that changes the current state of this option to a different one
	**/
	int addOptionsButton(int xPos, int yPos, const std::string &label, std::function<std::string()> labelFunction, std::function<void()> onClickFunction, bool enabled = true);

private:
	std::shared_ptr<Ego::GUI::ScrollableList> _resolutionList;
};

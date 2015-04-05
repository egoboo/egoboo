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

/// @file game/GUI/ScrollableList.hpp
/// @details GUI widget that can contain any number of GUIComponents and that can scroll through
///			 each component contained within this list
/// @author Johan Jansen

#pragma once

#include "game/GUI/ComponentContainer.hpp"
#include "game/GUI/GUIComponent.hpp"

//Forward declaration
class Button;

class ScrollableList : public GUIComponent, public ComponentContainer
{
public:
	static const size_t COMPONENT_LINE_SPACING;

	ScrollableList();

	void draw() override;

    /**
    * Ensure that this class inherits defaults for these methods from ComponentContainer and not GUIComponent
    **/
    bool notifyKeyDown(const int keyCode) override 								 {return ComponentContainer::notifyKeyDown(keyCode);}
    bool notifyMouseClicked(const int button, const int x, const int y) override;
   	bool notifyMouseScrolled(const int amount) override;
	bool notifyMouseMoved(const int x, const int y) override;

    void setWidth(const int width) override;
    void setHeight(const int height) override;
    void setX(const int x) override;
    void setY(const int y) override;

protected:
	void drawContainer() override;
	void updateScrollButtons();
	void setScrollPosition(int position);

private:
	size_t _currentIndex;
	bool _mouseOver;
	std::shared_ptr<Button> _downButton;
	std::shared_ptr<Button> _upButton;
};

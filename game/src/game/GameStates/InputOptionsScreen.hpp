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

/// @file game/GameStates/InputOptionsScreen.hpp
/// @details Selection input settings
/// @author Johan Jansen

#pragma once

#include "game/GameStates/GameState.hpp"
#include "egolib/InputControl/InputDevice.hpp"

//Forward declarations
namespace Ego { namespace GUI { class Button; } }

namespace Ego
{
namespace GameStates
{

class InputOptionsScreen : public GameState
{
public:
    InputOptionsScreen();

    void update() override;

    void beginState() override;

	virtual bool notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEventArgs& e) override;

protected:
    void drawContainer() override;

private:
	void addInputOption(const std::string& label, const Ego::Input::InputDevice::InputButton binding);

	Ego::Input::InputDevice& getActiveInputDevice() const;

private:
	int _maxLabelWidth;
	int _bindingButtonPosX;
	int _bindingButtonPosY;

	std::shared_ptr<Ego::GUI::Button> _activeButton;
	Ego::Input::InputDevice::InputButton _activeBinding;
};


} //GameStates
} //Ego

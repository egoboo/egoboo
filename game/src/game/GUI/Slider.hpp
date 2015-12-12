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

/// @file game/GUI/Slider.hpp
/// @details GUI widget of a moveable slider ranging between minimum and maximum value
/// @author Johan Jansen

#include "game/GUI/GUIComponent.hpp"

#pragma once

namespace Ego
{
namespace GUI
{

class Slider : public GUIComponent
{
public:
    Slider(int minValue, int maxValue);

    virtual void draw() override;

    void setOnChangeFunction(const std::function<void(int)> onChange);

    void setValue(const int value);
    int getValue() const;

    bool isEnabled() const override;

    bool notifyMouseMoved(const int x, const int y) override;
    bool notifyMouseClicked(const int button, const int x, const int y) override;
    bool notifyMouseReleased(const int button, const int x, const int y) override;

private:
    Ego::DeferredTexture _sliderBarTexture;
    Ego::DeferredTexture _sliderTexture;
    std::function<void(int)> _onChangeFunction;
    int _minValue;
    int _maxValue;
    float _sliderPosition;  //from 0.0f to 1.0f
    bool _isDragging;
};

} //GUI
} //Ego

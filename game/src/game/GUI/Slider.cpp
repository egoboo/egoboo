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

/// @file game/GUI/Slider.cpp
/// @details GUI widget of a moveable slider ranging between minimum and maximum value
/// @author Johan Jansen

#include "Slider.hpp"

namespace Ego
{
namespace GUI
{

Slider::Slider(int minValue, int maxValue) :
    _sliderBarTexture("mp_data/gui-slider_bar"),
    _sliderTexture("mp_data/gui-slider"),
    _onChangeFunction(),
    _minValue(minValue),
    _maxValue(maxValue),
    _sliderPosition(0.5f),
    _isDragging(false)
{
    if (_minValue >= _maxValue)
    {
        throw std::domain_error("min cannot be equal or more than max");
    }
}

void Slider::setOnChangeFunction(const std::function<void(int)> onChange)
{
    _onChangeFunction = onChange;
}

void Slider::draw()
{

}

void Slider::setValue(const int value)
{
    int constrainedValue = Ego::Math::constrain(value, _minValue, _maxValue);
    _sliderPosition = (1.0f / (_maxValue-_minValue)) * (constrainedValue-_minValue);
}

bool Slider::notifyMouseMoved(const int x, const int y)
{
    if(_isDragging) {
        _sliderPosition = (1.0f / getWidth()) * (x-getX());
        _sliderPosition = Ego::Math::constrain(_sliderPosition, 0.0f, 1.0f);
        return true;
    }

    return false;
}

int Slider::getValue() const
{
    return _minValue + (_maxValue-_minValue) * _sliderPosition;
}

bool Slider::notifyMouseClicked(const int button, const int x, const int y)
{
    if(button == SDL_BUTTON_LEFT && contains(x, y)) {
        _isDragging = true;
    }
    else {
        _isDragging = false;
    }

    return _isDragging;
}

bool Slider::notifyMouseReleased(const int button, const int x, const int y)
{
    if(_isDragging && button == SDL_BUTTON_LEFT) {
        _isDragging = false;
        _onChangeFunction(getValue());
        return true;        
    }
    return false;
}

} //GUI
} //Ego

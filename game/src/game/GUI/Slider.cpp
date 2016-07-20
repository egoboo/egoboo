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

namespace Ego {
namespace GUI {

Slider::Slider(int minValue, int maxValue) :
    _sliderBarTexture("mp_data/gui-slider_bar"),
    _sliderTexture("mp_data/gui_slider"),
    _onChangeFunction(),
    _minValue(minValue),
    _maxValue(maxValue),
    _sliderPosition(0.5f),
    _isDragging(false) {
    if (_minValue >= _maxValue) {
        throw std::domain_error("min cannot be equal or more than max");
    }
}

void Slider::setOnChangeFunction(const std::function<void(int)> onChange) {
    _onChangeFunction = onChange;
}

void Slider::draw(DrawingContext& drawingContext) {
    //Draw the bar
    _gameEngine->getUIManager()->drawImage(_sliderBarTexture.get_ptr(), Point2f(getX(), getY()), Vector2f(getWidth(), getHeight()), isEnabled() ? Math::Colour4f::white() : Math::Colour4f::grey());

    //Draw the moveable slider on top
    const int SLIDER_WIDTH = getWidth() / 10;
    _gameEngine->getUIManager()->drawImage(_sliderTexture.get_ptr(), Point2f(getX() + SLIDER_WIDTH + (getWidth() - SLIDER_WIDTH * 2)*_sliderPosition - SLIDER_WIDTH / 2, getY()), Vector2f(SLIDER_WIDTH, getHeight()), isEnabled() ? Math::Colour4f::white() : Math::Colour4f::grey());
}

void Slider::setValue(const int value) {
    int constrainedValue = Math::constrain(value, _minValue, _maxValue);
    _sliderPosition = (1.0f / (_maxValue - _minValue)) * (constrainedValue - _minValue);
}

bool Slider::notifyMouseMoved(const Events::MouseMovedEventArgs& e) {
    if (_isDragging) {
        _sliderPosition = (1.0f / getWidth()) * (e.getPosition().x() - getX());
        _sliderPosition = Math::constrain(_sliderPosition, 0.0f, 1.0f);
        return true;
    }

    return false;
}

int Slider::getValue() const {
    return _minValue + (_maxValue - _minValue) * _sliderPosition;
}

bool Slider::notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e) {
    if (e.getButton() == SDL_BUTTON_LEFT && contains(e.getPosition())) {
        _isDragging = true;
        notifyMouseMoved(Events::MouseMovedEventArgs(e.getPosition()));
    } else {
        _isDragging = false;
    }

    return _isDragging;
}

bool Slider::notifyMouseButtonReleased(const Events::MouseButtonReleasedEventArgs& e) {
    if (_isDragging && e.getButton() == SDL_BUTTON_LEFT) {
        _isDragging = false;
        if (_onChangeFunction) {
            _onChangeFunction(getValue());
        }
        return true;
    }
    return false;
}

bool Slider::isEnabled() const {
    return _onChangeFunction != nullptr && Component::isEnabled();
}

} //GUI
} //Ego

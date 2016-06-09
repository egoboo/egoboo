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

/// @file egolib/Events/MouseClickedEventArgs.hpp
/// @brief Arguments of a mouse clicked event.

#pragma once

#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace Events {

struct MouseClickedEventArgs {
private:
    Point2f position;
    int button;
public:
    /**
     * @brief Construct these mouse moved event arguments with the specified values.
     * @param position the position of the mouse
     * @param button the button
     */
    MouseClickedEventArgs(const Point2f& position, int button)
        : position(position), button(button) {}
    /**
     * @brief Construct these mouse clicked event arguments with the values of other mouse clicked event arguments.
     * @param other the other mouse clicked event arguments
     */
    MouseClickedEventArgs(const MouseClickedEventArgs& other)
        : position(other.position), button(other.button) {}
    /**
     * @brief Construct these mouse clicked event arguments with the values of other mouse clicked event arguments.
     * @param other the other mouse clicked event arguments
     */
    MouseClickedEventArgs(MouseClickedEventArgs&& other)
        : position(other.position), button(other.button) {}
    /**
     * @brief Assign these mouse clicked event arguments with the values of other mouse clicked event arguments.
     * @param other the other mouse clicked event arguments
     * @return these mouse clicked event arguments
     */
    MouseClickedEventArgs operator=(const MouseClickedEventArgs& other) {
        MouseClickedEventArgs temporary(other);
        std::swap(*this, temporary);
        return *this;
    }
    /**
     * @brief Get the mouse position.
     * @return the mouse position
     */
    const Point2f& getPosition() const {
        return position;
    }
    /**
     * @brief Get the mouse button.
     * @return the mouse button
     */
    int getButton() const {
        return button;
    }
};

} // namespace Events
} // namespace Ego

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

/// @file egolib/Events/MouseMovedEventArgs.hpp
/// @brief Arguments of a mouse moved event.

#pragma once

#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace Events {

struct MouseMovedEventArgs {
private:
    Point2f position;
public:
    /**
     * @brief Construct these mouse moved event arguments with the specified values.
     * @param position the position of the mouse
     */
    MouseMovedEventArgs(const Point2f& position)
        : position(position) {
    }
    /**
     * @brief Construct these mouse moved event arguments with the values of other mouse moved event arguments.
     * @param other the other mouse moved event arguments
     */
    MouseMovedEventArgs(const MouseMovedEventArgs& other) 
        : position(other.position) {
    }
    /**
     * @brief Construct these mouse moved event arguments with the values of other mouse moved event arguments.
     * @param other the other mouse moved event arguments
     */
    MouseMovedEventArgs(MouseMovedEventArgs&& other)
        : position(other.position) {
    }
    /**
     * @brief Assign these mouse moved event arguments with the values of other mouse moved event arguments.
     * @param other the other mouse moved event arguments
     * @return these mouse moved event arguments
     */
    MouseMovedEventArgs operator=(const MouseMovedEventArgs& other) {
        MouseMovedEventArgs temporary(other);
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
};

} // namespace Events
} // namespace Ego

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

/// @file egolib/Events/MouseWheelEventArgs.hpp
/// @brief Arguments of a mouse wheel event.

#pragma once

#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace Events {

/**
 * @brief Event arguments of a mouse wheel event.
 */
struct MouseWheelEventArgs {
private:
    Vector2f delta;
public:
    /**
     * @brief Construct these mouse wheel event arguments with the specified values.
     * @param delta scroll direction
     */
    MouseWheelEventArgs(Vector2f& delta)
        : delta(delta) {}
    /**
     * @brief Construct these mouse wheel event arguments with the values of other mouse wheel event arguments.
     * @param other the other mouse wheel event arguments
     */
    MouseWheelEventArgs(const MouseWheelEventArgs& other)
        : delta(other.delta) {}
    /**
     * @brief Assign these mouse wheel event arguments with the values of other mouse wheel event arguments.
     * @param other the other mouse wheel event arguments
     * @return these mouse wheel event arguments
     */
    MouseWheelEventArgs operator=(const MouseWheelEventArgs& other) {
        MouseWheelEventArgs temporary(other);
        std::swap(*this, temporary);
        return *this;
    }
    /**
     * @brief Get the delta.
     * @return the delta
     */
    const Vector2f& getDelta() const {
        return delta;
    }
};

} // namespace Events
} // namespace Ego

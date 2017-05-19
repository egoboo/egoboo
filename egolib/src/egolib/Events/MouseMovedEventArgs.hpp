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

/// @brief The event arguments of a mouse pointer moved event.
class MouseMovedEventArgs
{
private:
    /// @brief The position of the mouse pointer.
    Point2f m_position;

public:
    /// @brief Construct these mouse pointer moved event arguments with the specified values.
    /// @param position the position of the mouse pointer
    MouseMovedEventArgs(const Point2f& position)
        : m_position(position)
    {}

    MouseMovedEventArgs(const MouseMovedEventArgs&) = default;
    MouseMovedEventArgs(MouseMovedEventArgs&&) = default;
    MouseMovedEventArgs& operator=(const MouseMovedEventArgs&) = default;
    MouseMovedEventArgs& operator=(MouseMovedEventArgs&&) = default;

    /// @brief Get the mouse pointer position.
    /// @return the mouse pointer position
    const Point2f& position() const
    {
        return m_position;
    }

}; // class MouseMovedEventArgs

} // namespace Events
} // namespace Ego

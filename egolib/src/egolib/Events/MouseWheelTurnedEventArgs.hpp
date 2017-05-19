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

/// @brief Event arguments of a mouse wheel turned event.
class MouseWheelTurnedEventArgs
{
private:
    Vector2f m_delta;

public:
    /// @brief Construct these mouse wheel turned event arguments with the specified values.
    /// @param delta turn direction and amount
    MouseWheelTurnedEventArgs(const Vector2f& delta)
        : m_delta(delta)
    {}

    MouseWheelTurnedEventArgs(const MouseWheelTurnedEventArgs&) = default;
    MouseWheelTurnedEventArgs(MouseWheelTurnedEventArgs&&) = default;
    MouseWheelTurnedEventArgs& operator=(const MouseWheelTurnedEventArgs&) = default;
    MouseWheelTurnedEventArgs& operator=(MouseWheelTurnedEventArgs&&) = default;

    /// @brief Get the delta.
    /// @return the delta
    const Vector2f& delta() const
    {
        return m_delta;
    }

}; // class MouseWheelTurnedEventArgs

} // namespace Events
} // namespace Ego

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

/// @file egolib/Events/WindowEventArgs.hpp
/// @brief Arguments of a mouse released event.

#pragma once

#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace Events {

template <id::event::window_event_kind Kind>
class WindowEventArgs
{
public:
    /// @brief Construct these window event arguments.
    WindowEventArgs()
    {}

    WindowEventArgs(const WindowEventArgs&) = default;
    WindowEventArgs(WindowEventArgs&&) = default;
    WindowEventArgs& operator=(const WindowEventArgs&) = default;
    WindowEventArgs& operator=(WindowEventArgs&&) = default;

    /// @brief Get the kind of the window event.
    /// @return the kind of the window event
    id::event::window_event_kind kind() const
    {
        return Kind;
    }

}; // class WindowEventArgs

} // namespace Events
} // namespace Ego

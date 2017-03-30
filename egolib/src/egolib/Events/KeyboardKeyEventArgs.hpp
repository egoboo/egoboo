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

/// @file egolib/Events/KeyboardKeyEventArgs.hpp
/// @brief Arguments of a keyboard key event.

#pragma once

#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace Events {

/// @brief The event arguments for a keyboard key events.
/// @remark KeyboardKeyEventArgs::key() returns the key which was pressed/released/typed.
template <id::event::keyboard_key_event_kind Kind>
struct KeyboardKeyEventArgs
{
private:
    int m_key;

public:
    /// @brief Construct these keyboard key event arguments with the specified values.
    /// @param key the keyboard key
    explicit KeyboardKeyEventArgs(int key)
        : m_key(key)
    {}

    KeyboardKeyEventArgs(const KeyboardKeyEventArgs&) = default;
    KeyboardKeyEventArgs(KeyboardKeyEventArgs&&) = default;
    KeyboardKeyEventArgs& operator=(const KeyboardKeyEventArgs&) = default;
    KeyboardKeyEventArgs& operator=(KeyboardKeyEventArgs&&) = default;

    /// @brief Get the keyboard key.
    /// @return the keyboard key
    int key() const
    {
        return m_key;
    }

    /// @brief Get the kind of the keyboard key event.
    /// @return the kind of the keyboard key event
    id::event::keyboard_key_event_kind kind() const
    {
        return Kind;
    }

}; // class KeyboardKeyEventArgs

} // namespace Events
} // namespace Ego

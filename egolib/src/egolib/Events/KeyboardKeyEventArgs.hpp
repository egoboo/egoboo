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

/**
 * @ingroup ego-events
 * @brief Enumeration of the different kinds of keyboard key events.
 */
enum class KeyboardKeyEventKind {
    /**
     * @brief Kind of a keyboard key pressed event.
     */
    Pressed,
    /**
     * @brief Kind of a keyboard key released event.
     */
    Released,
    /**
     * @brief Kind of a keyboard key typed event.
     */
    Typed,
};


/**
 * @brief The event arguments for all kinds of keyboard key events.
 * @remark KeyboardKeyEventArgs::getKey returns the key which was pressed/released/typed.
 * In addition, the modifier keys which were active when the key was pressed can be obtained using KeyboardKeyEventArgs::getModifiers().
 */
template <KeyboardKeyEventKind _KeyboardKeyEventKind>
struct KeyboardKeyEventArgs {
private:
    int key;

public:
    /**
     * @brief Construct these keyboard key event arguments with the specified values.
     * @param key the keyboard key
     */
    KeyboardKeyEventArgs(int key)
        : key(key) {}

    /**
     * @brief Construct these keyboard key event arguments with the values of other keyboard key event arguments.
     * @param other the other keyboard key event arguments
     */
    KeyboardKeyEventArgs(const KeyboardKeyEventArgs& other)
        : key(other.key) {}

    /**
     * @brief Construct these keyboard key event arguments with the values of other keyboard key event arguments.
     * @param other the other keyboard key event arguments
     */
    KeyboardKeyEventArgs(KeyboardKeyEventArgs&& other)
        : key(other.key) {}

    /**
     * @brief Assign these keybaord key event arguments with the values of other keyboard key event arguments.
     * @param other the other keyboard key event arguments
     * @return these keyboard key event arguments
     */
    KeyboardKeyEventArgs operator=(const KeyboardKeyEventArgs& other) {
        KeyboardKeyEventArgs temporary(other);
        std::swap(*this, temporary);
        return *this;
    }

    /**
     * @brief Get the keyboard key.
     * @return the keyboard key
     */
    int getKey() const {
        return key;
    }

    /**
     * @brief Get the kind of the keyboard key event.
     * @return the kind of the keyboard key event
     */
    KeyboardKeyEventKind getKind() const {
        return _KeyboardKeyEventKind;
    }

};

} // namespace Events
} // namespace Ego

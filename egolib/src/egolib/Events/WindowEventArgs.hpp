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

struct WindowEventArgs {
public:
	/**
	 * @brief The kinds of window events.
	 */
	enum class Kind {
		/**
		 * @brief A window was moved.
		 */
		Moved,
		/**
		 * @brief A window was resized.
		 */
		Resized,
		/**
		 * @brief A window received keyboard input focus.
		 */
		KeyboardFocusReceived,
		/**
		 * @brief A window lost keyboard input focus.
		 */
		KeyboardFocusLost,
        /**
         * @brief A mouse entered a window.
         */
        MouseEntered,
        /**
         * @brief A mouse left a window.
         */
        MouseLeft,
        /**
         * @brief A window was shown.
         */
        Shown,
        /**
         * @brief A window was hidden.
         */
        Hidden,
	};

private:
    Kind kind;

public:
    /**
     * @brief Construct these window event arguments with the specified values.
     * @param kind the kind of the window event
     */
    WindowEventArgs(Kind kind)
        : kind(kind) {}
    /**
     * @brief Construct these window event arguments with the values of other window event arguments.
     * @param other the other window event arguments
     */
    WindowEventArgs(const WindowEventArgs& other)
        : kind(other.kind) {}
    /**
     * @brief Construct these window event arguments with the values of other window event arguments.
     * @param other the other window event arguments
     */
    WindowEventArgs(WindowEventArgs&& other)
        : kind(other.kind) {}
    /**
     * @brief Assign these window event arguments with the values of other window event arguments.
     * @param other the other window event arguments
     * @return these window event arguments
     */
    WindowEventArgs operator=(const WindowEventArgs& other) {
        WindowEventArgs temporary(other);
        std::swap(*this, temporary);
        return *this;
    }
    /**
     * @brief Get the kind of the window event.
     * @return the kind of the window event
     */
    Kind getKind() const {
        return kind;
    }
};

} // namespace Events
} // namespace Ego

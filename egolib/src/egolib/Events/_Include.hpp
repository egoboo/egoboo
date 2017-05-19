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

#include "egolib/Events/MouseButtonEventArgs.hpp"
#include "egolib/Events/KeyboardKeyEventArgs.hpp"
#include "egolib/Events/MouseWheelTurnedEventArgs.hpp"
#include "egolib/Events/MouseMovedEventArgs.hpp"
#include "egolib/Events/WindowEventArgs.hpp"

namespace Ego {
namespace Events {


using MouseButtonClickedEventArgs = MouseButtonEventArgs<id::event::mouse_button_event_kind::clicked>;
using MouseButtonPressedEventArgs = MouseButtonEventArgs<id::event::mouse_button_event_kind::pressed>;
using MouseButtonReleasedEventArgs = MouseButtonEventArgs<id::event::mouse_button_event_kind::released>;


using KeyboardKeyTypedEventArgs = KeyboardKeyEventArgs<id::event::keyboard_key_event_kind::typed>;
using KeyboardKeyPressedEventArgs = KeyboardKeyEventArgs<id::event::keyboard_key_event_kind::pressed>;
using KeyboardKeyReleasedEventArgs = KeyboardKeyEventArgs<id::event::keyboard_key_event_kind::released>;


using WindowMousePointerEnteredEventArgs = WindowEventArgs<id::event::window_event_kind::mouse_pointer_entered>;
using WindowMousePointerLeftEventArgs = WindowEventArgs<id::event::window_event_kind::mouse_pointer_left>;

using WindowResizedEventArgs = WindowEventArgs<id::event::window_event_kind::resized>;

using WindowShownEventArgs = WindowEventArgs<id::event::window_event_kind::shown>;
using WindowHiddenEventArgs = WindowEventArgs<id::event::window_event_kind::hidden>;

using WindowReceivedKeyboardInputFocusEventArgs = WindowEventArgs<id::event::window_event_kind::keyboard_input_focus_received>;
using WindowLostKeyboardInputFocusEventArgs = WindowEventArgs<id::event::window_event_kind::keyboard_input_focus_lost>;

} // namespace Events
} // namespace Ego

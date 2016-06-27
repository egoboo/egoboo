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
#include "egolib/Events/MouseWheelEventArgs.hpp"
#include "egolib/Events/MouseMovedEventArgs.hpp"
#include "egolib/Events/WindowEventArgs.hpp"

namespace Ego {
namespace Events {

using MouseButtonClickedEventArgs = MouseButtonEventArgs<MouseButtonEventKind::Clicked>;
using MouseButtonPressedEventArgs = MouseButtonEventArgs<MouseButtonEventKind::Pressed>;
using MouseButtonReleasedEventArgs = MouseButtonEventArgs<MouseButtonEventKind::Released>;

using KeyboardKeyTypedEventArgs = KeyboardKeyEventArgs<KeyboardKeyEventKind::Typed>;
using KeyboardKeyPressedEventArgs = KeyboardKeyEventArgs<KeyboardKeyEventKind::Pressed>;
using KeyboardKeyReleasedEventArgs = KeyboardKeyEventArgs<KeyboardKeyEventKind::Released>;

} // namespace Events
} // namespace Ego

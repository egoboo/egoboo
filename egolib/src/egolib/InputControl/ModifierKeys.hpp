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

/// @file egolib/Input/ModifierKeys.hpp
/// @brief Modifier keys.

#include "egolib/platform.h"

#pragma once

namespace Ego {

/**
 * @brief An flag enum class of modifier keys.
 * @remark An flag enum class has the bitwise operators |, &, ^, ~, |=, &=, ^=, and ~= enabled.
 * See Id::EnableBitmaskOperators.
 */
enum class ModifierKeys : int {
    /// The left Shift key.
    LeftShift = 1 << 1,
    /// The right Shift key.
    RightShift = 1 << 2,
    /// The left Control key,
    LeftControl = 1 << 3,
    /// The right Control key.
    RightControl = 1 << 4,
    /// The left Alt key.
    LeftAlt = 1 << 5,
    /// The right Alt key.
    /// Does not exist on every keyboard.
    RightAlt = 1 << 6,
    /// The left GUI key (on Windows: left Windows key).
    LeftGui = 1 << 7,
    /// The right GUI key (on Windows: right Windows key).
    RightGui = 1 << 8,
    /// The Num Lock key.
    Num = 1 << 9,
    /// The Caps Lock key.
    Caps = 1 << 10,
    /// The Mode key
    /// . aka AltGr key
    /// - aka Alt Graph key
    /// . aka Alt Car key
    /// - aka Alternate Graphic key
    /// - aka Alt Car key.
    /// Does not exist on every keyboard,
    Mode = 1 << 11,
};

} // namespace Ego

namespace Id {

/// 17.5.2.1.3 [bitmask.types] of the C++ Standard: 
/// Bitmask types shall provide definitions for the operators |, &, ^, ~, |=, &= and ^= with the expected semantics.
/// For scoped enums to be used as bitmasks, simply define
/// @code
//// struct enable_bitmask_operators<T> { static constexpr bool enable = true; }
/// @endcode
template <>
struct EnableBitmaskOperators<Ego::ModifierKeys> {
	static constexpr bool enable=true;
};

} // namespace Id

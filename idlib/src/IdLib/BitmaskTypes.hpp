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

/// @file  IdLib/BitmaskTypes.hpp
/// @brief Enum class bitmask types.
/// @author Michael Heilmann

#pragma once

#include <type_traits>

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif


namespace Id {

/// 17.5.2.1.3 [bitmask.types] of the C++ Standard: 
/// Bitmask types shall provide definitions for the operators |, &, ^, ~, |=, &= and ^= with the expected semantics.
/// For scoped enums to be used as bitmasks, simply define
/// @code
//// struct enable_bitmask_operators<T> { static constexpr bool enable = true; }
/// @endcode
template<typename E>
struct EnableBitmaskOperators {
	static constexpr bool enable=false;
};

} // namespace Id

/// Bitwise |.
template<typename E>
std::enable_if_t<Id::EnableBitmaskOperators<E>::enable,E>
operator|(E lhs,E rhs){
	typedef std::underlying_type_t<E> underlying;
	return static_cast<E>(
		static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

// Bitwise &.
template<typename E>
std::enable_if_t<Id::EnableBitmaskOperators<E>::enable,E>
operator&(E lhs,E rhs){
	typedef std::underlying_type_t<E> underlying;
	return static_cast<E>(
		static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

// Bitwise ^.
template<typename E>
std::enable_if_t<Id::EnableBitmaskOperators<E>::enable,E>
operator^(E lhs,E rhs){
    typedef std::underlying_type_t<E> underlying;
    return static_cast<E>(
        static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

// Bitwise ~.
template<typename E>
std::enable_if_t<Id::EnableBitmaskOperators<E>::enable,E>
operator~(E lhs){
    typedef std::underlying_type_t<E> underlying;
    return static_cast<E>(
        ~static_cast<underlying>(lhs));
}

// Bitwise |=.
template<typename E>
std::enable_if_t<Id::EnableBitmaskOperators<E>::enable,E&>
operator|=(E& lhs,E rhs){
    typedef std::underlying_type_t<E> underlying;
    lhs=static_cast<E>(
        static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
    return lhs;
}

// Bitwise &=.
template<typename E>
std::enable_if_t<Id::EnableBitmaskOperators<E>::enable,E&>
operator&=(E& lhs,E rhs){
    typedef std::underlying_type_t<E> underlying;
    lhs=static_cast<E>(
        static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
    return lhs;
}

// Bitwise ^=.
template<typename E>
std::enable_if_t<Id::EnableBitmaskOperators<E>::enable,E&>
operator^=(E& lhs,E rhs){
    typedef std::underlying_type_t<E> underlying;
    lhs=static_cast<E>(
        static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
    return lhs;
}

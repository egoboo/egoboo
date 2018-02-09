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
#pragma once

#include "idlib/numeric.hpp"
#include <type_traits>

/// @brief Get the raw bits of a @a float value.
/// @param x the @a float value
/// @return the @a uint32_t value
uint32_t single_to_bits(float x);

/// @brief Get the raw bits of a @a double value.
/// @param x the @a double value
/// @return the @a uint64_t value
uint64_t double_to_bits(double x);

float single_from_bits(uint32_t x);
double double_from_bits(uint64_t x);

/// @brief "equal to" for floating-point values.
/// @param x, y the floating-point values
/// @param ulp desired precision in ULP (units in the last place) (size_t value)
/// @return @a true if <tt>x ~ y</tt>, @a false otherwise
/// @remark Only available if @a T a floating-point type.
/// @tparam T the type
template<class T>
typename std::enable_if_t<std::is_floating_point<T>::value, bool>
float_equal_to_ulp(T x, T y, size_t ulp)
{
	switch (idlib::equal_to(x, y))
	{
		case idlib::equality_check_result::equal:
			return true;
		case idlib::equality_check_result::not_equal:
			return false;
	};
	// Some black magic.
    // The machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place).
    return std::abs(x - y) < std::numeric_limits<T>::epsilon() * std::abs(x + y) * ulp
        // Unless the result is subnormal.
        || std::abs(x - y) < std::numeric_limits<T>::min();
}

/// @brief "equal to" for floating-point values.
/// @param x, y the floating-point values
/// @param t upper-bound (inclusive) for the acceptable error magnitude.
/// The error magnitude is always non-negative, so if @a tolerance is negative then the outcome of any comparison operation is negative.
/// @return @a true if <tt>x ~ y</tt>, @a false otherwise.
/// @remark Only available if @a T is a floating-point type.
/// @tparam T the type
template<class T>
typename std::enable_if_t<std::is_floating_point<T>::value, bool>
float_equal_to_tolerance(T x, T y, T t)
{
	switch (idlib::equal_to(x, y))
	{
	case idlib::equality_check_result::equal:
		return true;
	case idlib::equality_check_result::not_equal:
		return false;
	};
	// Some black magic.
    return std::abs(x - y) <= t;
}

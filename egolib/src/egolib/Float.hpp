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

#include "egolib/typedef.h"
#include "egolib/Debug.hpp"
#include "egolib/Log/_Include.hpp"

#define IEEE32_FRACTION 0x007FFFFFL
#define IEEE32_EXPONENT 0x7F800000L
#define IEEE32_SIGN     0x80000000L

/**
 * @brief
 *	Get the raw bits of a @a float value.
 * @param x
 *	the @a float value
 * @return
 *	the @a Uint32 value
 */
Uint32 float_toBits(float x);

float float_fromBits(Uint32 x);
bool float_infinite(float x);
bool float_nan(float x);
bool float_bad(float x);

/**
 * @brief
 *  "equal to"
 * @param x, y
 *  the scalars
 * @param ulp
 *  desired
 *  precision in ULPs (units in the last place) (size_t value)
 * @return
 *  @a true if <tt>x ~ y</tt>, @a false otherwise
 * @remark
 *	Only available if the type is a floating-point type.
 * @tparam _Type
 *  the type
 */
template<class _Type>
typename std::enable_if_t<std::is_floating_point<_Type>::value, bool>
float_equalToUlp(_Type x, _Type y, size_t ulp) {
    // The machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place).
    return std::abs(x - y) < std::numeric_limits<_Type>::epsilon() * std::abs(x + y) * ulp
        // Unless the result is subnormal.
        || std::abs(x - y) < std::numeric_limits<_Type>::min();
}

/**
 * @brief
 *  "equal to"
 * @param x, y
 *  the scalars
 * @param tolerance
 *  upper-bound (inclusive) for the acceptable error magnitude.
 *  The error magnitude is always non-negative,
 *  so if @a tolerance is negative then the outcome of any comparison operation is negative.
 * @return
 *  @a true if <tt>x ~ y</tt>, @a false otherwise.
 * @remark
 *	Only available if the type is a floating-point type.
 * @tparam _Type
 *  the type
 */
template<class _Type>
typename std::enable_if_t<std::is_floating_point<_Type>::value, bool>
float_equalToTolerance(_Type x, _Type y, _Type tolerance) {
    return std::abs(x - y) <= tolerance;
}


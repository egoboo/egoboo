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

/// @file  egolib/math/Math.hpp
/// @brief Math utility functions
/// @author Johan Jansen
#pragma once

namespace Math
{
    /**
     * @brief
     *  Constrain a value within a specified range (same as clamping or clipping).
     * @param n
     *  the value
     * @param lower
     *  the minimum
     * @param upper
     *  the maximum
     * @return
     *  the constrained value
     */
    template <typename T>
    T constrain(const T& n, const T& lower, const T& upper) {
        return std::max(lower, std::min(n, upper));
    }

    /**
     * @brief
     *  Calculates the square of a number, this is same as X^2.
     *  This is much faster than using pow(val, 2)
     * @param val
     *  the number to square
     **/
    template <typename T>
    T sq(const T &value) {
        return value * value;
    }
}

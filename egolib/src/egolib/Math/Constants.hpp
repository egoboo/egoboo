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

/// @file egolib/Math/Constants.hpp
/// @brief Relevant mathematical constants

#pragma once

#include "egolib/platform.h"

/**
 * @brief \f$\frac{1}{255}\f$.
 * @return \f$\frac{1}{255}\f$ as a value of type @a T.
 * @remark Specializations for single- and double-precision floating-point types are provided.
 */
template <typename T>
T INV_FF();

template <>
inline float INV_FF<float>() {
    return 0.003921568627450980392156862745098f;
}

template <>
inline double INV_FF<double>() {
    return 0.003921568627450980392156862745098;
}


/**
 * @brief \f$\frac{1}{65535}\f$.
 * @return \f$\frac{1}{65535}\f$ as a value of type @a T.
 * @remark Specializations for single- and double-precision floating-point types are provided.
 */
template <typename T>
T INV_FFFF();

template <>
inline float INV_FFFF<float>() {
    return 0.000015259021896696421759365224689097f;
}

template <>
inline double INV_FFFF<double>() {
    return 0.000015259021896696421759365224689097;
}

/**
 * @brief \f$\frac{1}{256}\f$.
 * @return \f$\frac{1}{256}\f$ as a value of type @a T.
 * @remark Specializations for single- and double-precision floating-point types are provided.
 */
template <typename T>
T INV_0100();

template <>
inline float INV_0100<float>() {
    return 0.00390625f;
}

template <>
inline double INV_0100<double>() {
    return 0.00390625;
}

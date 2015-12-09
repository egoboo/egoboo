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

/// @file   egolib/Math/Angle.hpp
/// @brief  Angles.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Math.hpp"

namespace Ego {
namespace Math {

/// The type of an angle in degrees.
/// The canonical range of an angle is degrees is \f$[0,360]\f$.
/// If \f$x\f$ is an angle in degrees then
/// - \f$x \cdot \frac{1}{360}\f$ is the equivalent angle in turns and
/// - \f$x \cdot \frac{2\pi}{360}\f$ is the equivalent angle in radians.
typedef float Degrees;

/// The type of an angle in radians.
/// The canonical range of an angle in radians is \f$[0,2\pi]\f$.
/// If \f$x\f$ is an angle in radians then
/// - \f$x \cdot \frac{1}{2\pi}\f$ is the equivalent angle in turns and
/// - \f$x \cdot \frac{360}{2\pi}\f$ is the equivalent angle in degrees.
typedef float Radians;

/// The type of an angle in turns.
/// The canonical range of an angle in turns is \f$[0,1]\f$.
/// Let \f$x\f$ be an angle in turns then
/// - \f$x \cdot 360\f$   is the equivalent angle in degrees and
/// - \f$x \cdot 2\pi\f$  is the equivalent angle in radians.
/// To convert an angle in turns \f$x\f$ to an angle in degrees 
typedef float Turns;

/**
 * @brief
 *  Convert an angle from radians (\f$[0,2\pi]\f$) to turns (\f$[0,1]\f$).
 * @param x
 *  the angle in radians
 * @return
 *  the angle in turns
 */
inline Turns RadiansToTurns(Radians x) {
    return x * invTwoPi<float>();
}

/**
 * @brief
 *  Convert an angle from turns to radians.
 * @param x
 *  the angle in turns
 * @return
 *  the angle in radians
 */
inline Radians TurnsToRadians(Turns x) {
    return x * twoPi<float>();
}

/**
 * @brief
 *  Convert an angle from radians to degrees.
 * @param xalloc
 *  the angle in radians
 * @return
 *  the angle in degrees
 */
inline Degrees RadiansToDegrees(Radians x) {
    return x * (360.0f / twoPi<float>());
}

/**
 * @brief
 *  Convert an angle from degrees to radians.
 * @param x
 *  the angle in degrees
 * @return
 *  the angle in radians
 */
inline Radians DegreesToRadians(Degrees x) {
    return x * (twoPi<float>() / 360.0f);
}

} // namespace Math
} // namespace Ego

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

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief Enumeration of units in which angles are measured.
 */
enum class AngleUnit {
    /** @brief The unit degrees with its canonical range of \f$[0,360)\f$. */
    Degrees,
    /** @brief The unit radians with its canonical range of \f$[0,2\pi)\f$. */
    Radians,
    /** @brief The unit turns with its canonical range of \f$[0,1)\f$. */
    Turns,
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

namespace Internal {

/// Convert an angle in a unit into an equivalent angle in another unit.
template <AngleUnit _TargetAngleUnitType, AngleUnit _SourceAngleUnitType>
inline float convert(float x);

// Conversion between Degrees and Degrees (identity).

template <>
inline float convert<AngleUnit::Degrees, AngleUnit::Degrees>(float x) {
    return x;
}

// Conversion between Radians and Radians (identity).

template <>
inline float convert<AngleUnit::Radians, AngleUnit::Radians>(float x) {
    return x;
}

// Conversion between Turns and Turns (identity).

template <>
inline float convert<AngleUnit::Turns, AngleUnit::Turns>(float x) {
    return x;
}

// Conversion between Degrees and Radians.

/// The conversion of an angle \f$\alpha\f$ in degrees into an equivalent angle
/// \f$\beta\f$ in radians is defined by \f$\beta = \alpha \frac{2\pi}{360}\f$.
template <>
inline float convert<AngleUnit::Radians, AngleUnit::Degrees>(float x) {
    return x * (pi<float>() / 180.0f);
}

/// The conversion of an angle \f$\alpha\f$ in radians into an equivalent angle
/// \f$\beta\f$ in degrees is defined by \f$\beta = \alpha \frac{360}{2\pi}\f$.
template <>
inline float convert<AngleUnit::Degrees, AngleUnit::Radians>(float x) {
    return x * (180.0f * invPi<float>());
}

// Conversion between Degrees and Turns.

/// The conversion of an angle \f$\alpha\f$ in degrees into an equivalent angle
/// \f$\beta\f$ in turns is defined as \f$\beta = \alpha \frac{1}{360}\f$.
template <>
inline float convert<AngleUnit::Turns, AngleUnit::Degrees>(float x) {
    return x * (1.0f / 360.0f);
}

/// The conversion of an angle \f$\alpha\f$ in turns into an equivalent angle
/// \f$\beta\f$ in degrees is defined as \f$\beta = \alpha 360\f$.
template <>
inline float convert<AngleUnit::Degrees, AngleUnit::Turns>(float x) {
    return x * 360.0f;
}

// Conversion between Radians and Turns.

/// The conversion of an angle \f$\alpha\f$ in radians into an equivalent angle
/// \f$\beta\f$ in turns is defined as \$\beta = \alpha \frac{1}{2 \pi}\f$.
template <>
inline float convert<AngleUnit::Turns, AngleUnit::Radians>(float x) {
    return x * invTwoPi<float>();
}

/// The conversion of an angle \f$\alpha\f$ in turns into an equivalent angle
/// \f$\beta\f$ in radians is defined as \$\beta = \alpha 2 \pi\f$.
template <>
inline float convert<AngleUnit::Radians, AngleUnit::Turns>(float x) {
    return x * twoPi<float>();
}
}

template <AngleUnit _AngleUnitType>
struct Angle {
private:
    float angle;

public:
    Angle(const Angle& other) :
        angle(other.angle) {
    }

public:
    template <AngleUnit _AngleUnitTargetType = _AngleUnitType, AngleUnit _AngleUnitSourceType>
    Angle(const Angle<_AngleUnitSourceType>& other) :
        angle(Internal::convert<_AngleUnitTargetType, _AngleUnitSourceType>((float)other)) {
    }

public:
    /// Construct this angle with its default values.
    Angle() : angle(0.0f) {}
    /// Construct this angle with the specified values.
    explicit Angle(float angle) : angle(angle) {}

public:
    const Angle& operator=(const Angle& other) {
        angle = other.angle;
        return *this;
    }

public:
    bool operator==(const Angle& other) const {
        return angle == other.angle;
    }

    bool operator!=(const Angle& other) const {
        return angle != other.angle;
    }

public:
    bool operator<(const Angle& other) const {
        return angle < other.angle;
    }

    bool operator<=(const Angle& other) const {
        return angle <= other.angle;
    }

public:
    bool operator>(const Angle& other) const {
        return angle > other.angle;
    }

    bool operator>=(const Angle& other) const {
        return angle >= other.angle;
    }

public:
    Angle operator+(const Angle& other) const {
        Angle temporary(*this);
        temporary += other;
        return temporary;
    }

    Angle operator-(const Angle& other) const {
        Angle temporary(*this);
        return temporary - other;
    }

public:
    Angle operator+=(const Angle& other) {
        angle += other.angle;
        return *this;
    }

    Angle operator-=(const Angle& other) {
        angle -= other.angle;
        return *this;
    }

public:
    Angle operator*(float other) const {
        return Angle(angle * other);
    }

    Angle operator/(float other) const {
        return Angle(angle * other);
    }

public:
    const Angle& operator*=(float other) {
        angle *= other;
        return *this;
    }

    const Angle& operator/=(float other) {
        angle /= other;
        return *this;
    }

public:
    /// Explicit cast operator: Cast this angle into an equivalent angle of another unit.
    /// @tparam _AngleUnitTargetType the type of the target unit
    /// @tparam _AngleUnitSourceType the type of the source unit.
    ///                              Must be equal to _AngleUnitType.
    template <AngleUnit _AngleUnitTargetType, AngleUnit _AngleUnitSourceType = _AngleUnitType>
    explicit operator std::enable_if_t<_AngleUnitType == _AngleUnitSourceType,
                                       Angle<_AngleUnitTargetType>>() const {
        return Angle<_AngleUnitTargetType>(Internal::convert<_AngleUnitTargetType, _AngleUnitSourceType>(angle));
    }

public:
    explicit operator float() const {
        return angle;
    }

public:
    /**
     * @brief Get if an angle, in degrees, is an acute angle.
     * @param x the angle, in degrees
     * @return @a true if the angle is an acute angle, @a false otherwise
     * @remark An angle \f$\alpha\f$ in degrees is an acute angle if \f$0 < a < 90\f$.
     */
    template <AngleUnit _LocalAngleUnitType = _AngleUnitType>
    std::enable_if_t<AngleUnit::Degrees == _LocalAngleUnitType && _LocalAngleUnitType == _AngleUnitType, bool>
    isAcute() const {
        return 0.0f < angle && angle < 90.0f;
    }

    /**
     * @brief Get if an angle, in radians, is an acute angle.
     * @param x the angle, in radians
     * @return @a true if the angle is an acute angle, @a false otherwise
     * @remark An acute angle is an angle which has a measure between that of a right angle and that of a zero angle.
     * In other words: An angle \f$\alpha\f$ in radians is an acute angle if \f$0 < a < \frac{\pi}{2}\f$.
     */
    template <AngleUnit _LocalAngleUnitType = _AngleUnitType>
    std::enable_if_t<AngleUnit::Radians == _LocalAngleUnitType && _LocalAngleUnitType == _AngleUnitType, bool>
    isAcute() const {
        return 0.0f < angle && angle < twoPi<float>();
    }

    /**
     * @brief Get if this angle, is an acute angle.
     * @return @a true if the angle is an acute angle, @a false otherwise
     * @remark An acute angle is an angle which has a measure between that of a right angle and that of a zero angle.
     * In other words: An angle \f$\alpha\f$ in turns is an acute angle if \f$0 < a < \frac{1}{4}\f$.
     */
    template <AngleUnit _LocalAngleUnitType = _AngleUnitType>
    std::enable_if_t<AngleUnit::Turns == _LocalAngleUnitType && _LocalAngleUnitType == _AngleUnitType, bool>
    isAcute() const {
        return 0.0f < angle && angle < 0.25f;
    }

};

/// The type of an angle in degrees.
/// The canonical range of an angle is degrees is \f$[0,360]\f$.
/// If \f$x\f$ is an angle in degrees then
/// - \f$x \cdot \frac{1}{360}\f$ is the equivalent angle in turns and
/// - \f$x \cdot \frac{2\pi}{360}\f$ is the equivalent angle in radians.
typedef Angle<AngleUnit::Degrees> Degrees;

/// The type of an angle in radians.
/// The canonical range of an angle in radians is \f$[0,2\pi]\f$.
/// If \f$x\f$ is an angle in radians then
/// - \f$x \cdot \frac{1}{2\pi}\f$ is the equivalent angle in turns and
/// - \f$x \cdot \frac{360}{2\pi}\f$ is the equivalent angle in degrees.
typedef Angle<AngleUnit::Radians> Radians;

/// The type of an angle in turns.
/// The canonical range of an angle in turns is \f$[0,1]\f$.
/// Let \f$x\f$ be an angle in turns then
/// - \f$x \cdot 360\f$   is the equivalent angle in degrees and
/// - \f$x \cdot 2\pi\f$  is the equivalent angle in radians.
/// To convert an angle in turns \f$x\f$ to an angle in degrees 
typedef Angle<AngleUnit::Turns> Turns;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <AngleUnit _AngleUnitType>
inline Angle<_AngleUnitType> operator * (float x, const Angle<_AngleUnitType>& y) {
    return Angle<_AngleUnitType>(x * (float)y);
}

template <AngleUnit _AngleUnitType>
inline Angle<_AngleUnitType> operator / (float x, const Angle<_AngleUnitType>& y) {
    return Angle<_AngleUnitType>(x / (float)y);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace Math
} // namespace Ego

namespace std {

template <Ego::Math::AngleUnit _AngleUnitType>
float sin(const Ego::Math::Angle<_AngleUnitType>& x) {
    using TargetAngleType = Ego::Math::Angle<Ego::Math::AngleUnit::Radians>;
    return sin(float(TargetAngleType(x)));
}

template <Ego::Math::AngleUnit _AngleUnitType>
float cos(const Ego::Math::Angle<_AngleUnitType>& x) {
    using TargetAngleType = Ego::Math::Angle<Ego::Math::AngleUnit::Radians>;
    return cos(float(TargetAngleType(x)));
}

template <Ego::Math::AngleUnit _AngleUnitType>
float tan(const Ego::Math::Angle<_AngleUnitType>& x) {
    using TargetAngleType = Ego::Math::Angle<Ego::Math::AngleUnit::Radians>;
    return tan(float(TargetAngleType(x)));
}

}
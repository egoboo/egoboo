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

/// @file egolib/_math.h
/// @details The name's pretty self explanatory, doncha think?

#pragma once

#include "egolib/Math/euler_angle.hpp"
#include "egolib/Math/Random.hpp"
#include "egolib/Log/_Include.hpp"

//--------------------------------------------------------------------------------------------
// IEEE 32-BIT FLOATING POINT NUMBER FUNCTIONS
//--------------------------------------------------------------------------------------------

template <typename Type>
int sgn(const Type& x) {
    return LAMBDA(0 == x, 0, LAMBDA(x > 0, 1, -1));
}

#if defined(TEST_NAN_RESULT)
#    define LOG_NAN(XX)      if( ieee32_bad(XX) ) log_error( "**** A math operation resulted in an invalid result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#else
#    define LOG_NAN(XX)
#endif

typedef uint16_t FACING_T;

namespace id {

// An angle with syntax uint16_t and semantics facings.
// n := std::numeric_limits<uint16_t>::min() corresponds to 0 degrees,
// m := std::numeric_limits<uint16_t>::max() corresponds to 360 degrees.
// The canonical range is [n, m).
template <>
struct angle<uint16_t, facings> {
private:
    int32_t m_angle;

    static void constrain(int32_t& angle) {
        while (angle < -static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) {
            angle = angle + static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
        }
        while (angle > +static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) {
            angle = angle - static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
        }
    }

public:
    explicit angle(const angle<float, turns>& other) : angle(0) {
        static const float s = static_cast<float>(static_cast<int32_t>(std::numeric_limits<uint16_t>::max()) + 1);
        this->m_angle = int32_t(float(other) * s);
        constrain(this->m_angle);
    }
    explicit angle(const angle<float, id::degrees>& x) : angle(semantic_cast<angle<float, id::turns>>(x)) {
        /* Intentionally left empty. */
    }
    explicit angle(const angle<float, id::radians>& x) : angle(semantic_cast<angle<float, id::turns>>(x)) {
        /* Intentionally left empty. */
    }
    // int32_t always fits into int32_t.
    explicit angle(int32_t angle) : m_angle(0) {
        // Do *not* normalize the angle.
        this->m_angle = angle;
        //constrain(this->angle);
    }
    // uint16_t always fits into int32_t.
    // uint16_t is always in the correct range of 0 and 2^16-1.
    explicit angle(uint16_t angle) : angle(static_cast<int32_t>(angle)) {
        /* Intentionally left empty. */
    }
    angle() : angle(0) {
        /* Intentionally left empty. */
    }
    angle(const angle& other) : angle(other.m_angle) {
        /* Intentionally left empty. */
    }
    // Explicit cast. Canonicalizes angles. 
    explicit operator uint16_t() const {
        int32_t x = m_angle;
        while (x < 0) {
            x = x + static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
        }
        while (x > +static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) {
            x = x - static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
        }
        return x;
    }

	int32_t get_value() const
	{ return m_angle; }

    // Explicit cast.
    explicit operator int32_t() const {
        return m_angle;
    }
    explicit operator angle<float, turns>() const {
        static const int32_t s = static_cast<float>(static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) + 1;
        return angle<float, turns>(float(m_angle) * s);
    }
    explicit operator angle<float, radians>() const {
        return semantic_cast<angle<float, radians>>((angle<float, turns>)(*this));
    }
    explicit operator angle<float, degrees>() const {
        return semantic_cast<angle<float, degrees>>((angle<float, turns>)(*this));
    }

public:
	angle operator+() const {
        return *this;
    }
	angle operator-() const {
        return angle(-m_angle);
    }

public:
	angle operator*(float other) const {
        return angle(int32_t(float(m_angle) * other));
    }

	angle operator+(const angle& other) const {
        // this.angle and other.angle are in the canonical range of 0 and 2^16-1.
        // (2^16 - 1) + (2^16-1) is always smaller than the maximum value of int32_t.
        return angle(m_angle + other.m_angle);
    }

	angle operator-(const angle& other) const {
        // this angle and other.angle are in the canonical range of 0 and 2^16-1.
        // 0         -    2^16-1 is always greater than the minimum value of int32_t.
        return angle(m_angle - other.m_angle);
    }

    const angle& operator+=(const angle& other) {
        (*this) = (*this) + other;
        return *this;
    }

    const angle& operator-=(const angle& other) {
        (*this) = (*this) - other;
        return *this;
    }

    bool operator<(const angle& other) const
	{ return m_angle < other.m_angle; }

    bool operator<=(const angle& other) const
	{ return m_angle <= other.m_angle; }

    bool operator>(const angle& other) const
	{ return m_angle > other.m_angle; }

    bool operator>=(const angle& other) const
	{ return m_angle >= other.m_angle; }

    bool operator==(const angle& other) const
	{ return m_angle == other.m_angle; }

    bool operator!=(const angle& other) const
	{ return m_angle != other.m_angle; }

public:
    static angle random(bool negative = false)
	{
        int32_t x = static_cast<int32_t>(Random::next<uint16_t>(std::numeric_limits<uint16_t>::max()));
        if (negative)
		{
            x = Random::nextBool() ? x : -x;
        }
        return angle(x);
    }
};

template <typename T>
struct canonicalize_functor;

template <typename T>
auto canonicalize(const T& v) -> decltype(canonicalize_functor<T>()(v))
{ return canonicalize_functor<T>()(v); }

/// @brief Specialization of id::canonicalize for id::angle<uint16_t, id::facings>.
/// Maps an angle x to the canonical range [n, m) where
/// n := std::numeric_limits<uint16_t>::min() and
/// m := std::numeric_limits<uint16_t>::max().
template <>
struct canonicalize_functor<angle<uint16_t, facings>>
{
	auto operator()(const angle<uint16_t, facings>& x) const
	{
		static const int32_t c = static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
		int32_t v = x.get_value();
		while (v < 0)
		{ v += c; }
		while (v >= c)
		{ v -= c; }
		return angle<uint16_t, facings>(v);
	}
}; // struct canonicalize_functor

} // namespace id

using Facing = id::angle<uint16_t, id::facings>;

/// Directional alias for "an attack from front".
extern const Facing ATK_FRONT;
/// Directional alias for "an attack from right".
extern const Facing ATK_RIGHT;
/// Directional alias for "an attack from behind".
extern const Facing ATK_BEHIND;
/// Directional alias for "an attack from left".
extern const Facing ATK_LEFT;

/// Facing alias for "west".
extern const Facing FACE_WEST;
/// Facing alias for "north".
extern const Facing FACE_NORTH;
/// Facing alias for "east".
extern const Facing FACE_EAST;
/// Facing alias for "south".
extern const Facing FACE_SOUTH;

using EulerFacing = id::euler_angle<id::angle<uint16_t, id::facings>>;

template <>
inline int sgn<Facing>(const Facing& x) {
    return sgn(x.get_value());
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// basic constants

/**
 * @brief
 *  Convert an angle from turns to "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$).
 * @param x
 *  the angle in turns
 * @return
 *  the angle in "facing"
 */
inline Facing TurnToFacing(const id::angle<float, id::turns>& x) {
    return Facing(x);
}

/**
 * @brief
 *  Convert an angle from "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$)) to turns.
 * @param x
 *	the angle in "facing"
 * @return
 *  the angle in turns
 */
inline id::angle<float, id::turns> FacingToTurn(const Facing& x) {
    return static_cast<id::angle<float, id::turns>>(x);
}


/**
 * @brief
 *  Convert an angle "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$) to radians.
 * @param x
 *  the angle in facing
 * @return
 *  the angle in radians
 */
inline id::angle<float, id::radians> FacingToRadian(const Facing& x) {
    return static_cast<id::angle<float, id::radians>>(x);
}

/**
 * @brief
 *  Convert an angle from radians to "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$).
 * @param x
 *  the angle in radians
 * @return
 *  the angle in "facing"
 */
inline Facing RadianToFacing(const id::angle<float, id::radians>& x) {
    return TurnToFacing(id::semantic_cast<id::angle<float, id::turns>>(x));
}

// conversion functions
Facing vec_to_facing(const float dx, const float dy);
void facing_to_vec(const Facing& facing, float * dx, float * dy);

// rotation functions

/**
 * @brief Rotate from a source angle into a target angle.
 * @param source the source angle
 * @param target the target angle
 * @param weight the weight, must be positive
 * @return the value <tt>source + (target-source)/weight</tt>
 * @pre <tt>weight</tt> must be positive.
 */
Facing rotate(const Facing& source, const Facing& target, const float weight);

//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// FAST CONVERSIONS

#define FF_TO_FLOAT( V1 )  ( (float)(V1) * id::fraction<float,1,255>() )

#define FFFF_TO_FLOAT( V1 )  ( (float)(V1) * id::fraction<float, 1, 65535>() )
#define FLOAT_TO_FFFF( V1 )  ( (int)((V1) * 0xFFFF) )

//--------------------------------------------------------------------------------------------

// limiting functions
    void getadd_int( const int min, const int value, const int max, int* valuetoadd );
    void getadd_flt( const float min, const float value, const float max, float* valuetoadd );

// random functions
    int generate_irand_pair( const IPair num );
    int generate_irand_range( const id::interval<float> num );

//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif

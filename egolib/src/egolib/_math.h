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

#include "egolib/Math/Angle.hpp"
#include "egolib/Math/Random.hpp"
#include "egolib/Log/_Include.hpp"

//--------------------------------------------------------------------------------------------
// IEEE 32-BIT FLOATING POINT NUMBER FUNCTIONS
//--------------------------------------------------------------------------------------------


#if defined(TEST_NAN_RESULT)
#    define LOG_NAN(XX)      if( ieee32_bad(XX) ) log_error( "**** A math operation resulted in an invalid result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#else
#    define LOG_NAN(XX)
#endif

/// the type for the 16-bit value used to store angles
/// Some custom Egoboo unit within the bounds of \f$[0,2^16-1]\f$.
struct Facing {
private:
    // The canonical range of unit facing is 0 = UINT16_MIN, 360 = 2^16-1 = UINT16_MAX.
    // for the
    uint16_t angle;
public:
    explicit Facing(const Ego::Math::Turns& x) : angle(0) {
        static const float s = (float)0x00010000; // UINT16_MAX + 1.
        this->angle = Ego::Math::clipBits<16>(int(float(x) * s));
    }
    explicit Facing(const Ego::Math::Degrees& x) : Facing(Ego::Math::Turns(x)) {
        /* Intentionally left empty. */
    }
    explicit Facing(const Ego::Math::Radians& x) : Facing(Ego::Math::Turns(x)) {
        /* Intentionally left empty. */
    }
    // int32_t always fits into int32_t.
    explicit Facing(int32_t angle) : angle(0) {
        // Important: Normalize the angle.
        static constexpr int32_t min = static_cast<int32_t>(std::numeric_limits<uint16_t>::min()),
            max = static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
        while (angle < min) { angle += max; }
        while (angle > max) { angle -= max; }
        this->angle = angle;
    }
    // uint16_t always fits into int32_t.
    // uint16_t is always in the correct range of 0 and 2^16-1.
    explicit Facing(uint16_t angle) : angle(static_cast<int32_t>(angle)) {
        /* Intentionally left empty. */
    }
    Facing() : angle(0) {
        /* Intentionally left empty. */
    }
    Facing(const Facing& other) : angle(other.angle) {
        /* Intentionally left empty. */
    }
    explicit operator int32_t() const {
        // angle is always in the canonical range of 0 and 2^16-1.
        return angle;
    }
    explicit operator uint16_t() const {
        // angle is always in the canonical range of 0 and 2^16-1.
        return angle;
    }
    explicit operator Ego::Math::Turns() const {
        static const int32_t m = static_cast<int32_t>(std::numeric_limits<uint16_t>::max()) + 1;
        static const float s = 1.0f / float(m);
        return Ego::Math::Turns(float(angle) * s);
    }
    explicit operator Ego::Math::Radians() const {
        return Ego::Math::Radians((Ego::Math::Turns)(*this));
    }
    explicit operator Ego::Math::Degrees() const {
        return Ego::Math::Degrees((Ego::Math::Turns)(*this));
    }

public:
    Facing operator+(const Facing& other) const {
        // this.angle and other.angle are in the canonical range of 0 and 2^16-1.
        // (2^16 - 1) + (2^16-1) is always smaller than the maximum value of int32_t.
        return Facing(int32_t(angle) + int32_t(other.angle));
    }
    Facing operator-(const Facing& other) const {
        // this angle and other.angle are in the canonical range of 0 and 2^16-1.
        // 0         -    2^16-1 is always greater than the minimum value of int32_t.
        return Facing(int32_t(angle) - int32_t(other.angle));
    }
    const Facing& operator+=(const Facing& other) {
        (*this) = (*this) + other;
        return *this;
    }
    const Facing& operator-=(const Facing& other) {
        (*this) = (*this) - other;
        return *this;
    }

public:
    bool operator<(const Facing& other) const {
        return angle < other.angle;
    }
    bool operator<=(const Facing& other) const {
        return angle <= other.angle;
    }
    bool operator>(const Facing& other) const {
        return angle > other.angle;
    }
    bool operator>=(const Facing& other) const {
        return angle >= other.angle;
    }
    bool operator==(const Facing& other) const {
        return angle == other.angle;
    }
    bool operator!=(const Facing& other) const {
        return angle != other.angle;
    }
    static Facing random() {
        const uint16_t angle = Random::next<uint16_t>(std::numeric_limits<uint16_t>::max());
        return Facing(angle);
    }
};

typedef uint16_t FACING_T;

#define FACE_RANDOM  Random::next<FACING_T>(std::numeric_limits<FACING_T>::max())

/// pre defined directions
static constexpr FACING_T FACE_WEST = 0x0000; ///< Character facings
static constexpr FACING_T FACE_NORTH = 0x4000;
static constexpr FACING_T FACE_EAST = 0x8000;
static constexpr FACING_T FACE_SOUTH = 0xC000;

//Directional aliases
static constexpr FACING_T ATK_FRONT = FACE_WEST;
static constexpr FACING_T ATK_RIGHT = FACE_NORTH;
static constexpr FACING_T ATK_BEHIND = FACE_EAST;
static constexpr FACING_T ATK_LEFT = FACE_SOUTH;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// basic constants

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
* @brief \f$\frac{1}{255^2}\f$.
* @return \f$\frac{1}{255^2}\f$ as a value of type @a T.
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
 * @brief
 *  Convert an angle from turns to "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$).
 * @param x
 *  the angle in turns
 * @return
 *  the angle in "facing"
 */
inline FACING_T TurnToFacing(const Ego::Math::Turns& x) {
    // 0x00010000 = UINT16_MAX + 1.
    // s := UINT16_MAX + 1.
    // TODO: Why is +1 added?
    static const int32_t m = static_cast<int32_t>(std::numeric_limits<uint16_t>::max()) + 1;
    static const float s = (float)m;
	return FACING_T(Ego::Math::clipBits<16>(int(float(x) * s)));
}

/**
 * @brief
 *  Convert an angle from "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$)) to turns.
 * @param x
 *	the angle in "facing"
 * @return
 *  the angle in turns
 */
inline Ego::Math::Turns FacingToTurn(const FACING_T& x) {
    // 0x00010000 = UINT16_MAX + 1.
    // s := 1 / (UINT16_T + 1).
    // TODO: why is +1 added?
    static const int32_t m = static_cast<int32_t>(std::numeric_limits<uint16_t>::max()) + 1;
    static const float s = 1.0f / (float)m;
	return Ego::Math::Turns(float(x) * s);
}
inline Ego::Math::Turns FacingToTurn(const Facing& x) {
    // 0x00010000 = UINT16_MAX + 1.
    // s := 1 / (UINT16_T + 1).
    // TODO: why is +1 added?
    static const int32_t m = static_cast<int32_t>(std::numeric_limits<uint16_t>::max()) + 1;
    static const float s = 1.0f / (float)m;
    return Ego::Math::Turns(float(uint16_t(x)) * s);
}


/**
 * @brief
 *  Convert an angle "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$) to radians.
 * @param x
 *  the angle in facing
 * @return
 *  the angle in radians
 */
inline Ego::Math::Radians FacingToRadian(const FACING_T& x) {
    return Ego::Math::Radians(FacingToTurn(x));
}
inline Ego::Math::Radians FacingToRadian(const Facing& x) {
    return Ego::Math::Radians(FacingToTurn(x));
}

/**
 * @brief
 *  Convert an angle from radians to "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$).
 * @param x
 *  the angle in radians
 * @return
 *  the angle in "facing"
 */
inline FACING_T RadianToFacing(const Ego::Math::Radians& x) {
    return TurnToFacing(Ego::Math::Turns(x));
}

// conversion functions
FACING_T vec_to_facing(const float dx, const float dy);
void     facing_to_vec(const FACING_T& facing, float * dx, float * dy);

// rotation functions
int terp_dir(const FACING_T& majordir, const FACING_T& minordir, const int weight);

//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if !defined(SGN)
#    define SGN(X)  LAMBDA( 0 == (X), 0, LAMBDA( (X) > 0, 1, -1) )
#endif

#if !defined(SQR)
#    define SQR(A) ((A)*(A))
#endif

//--------------------------------------------------------------------------------------------
// FAST CONVERSIONS


#if !defined(INV_0100)
#   define INV_0100            0.00390625f
#endif


#define FF_TO_FLOAT( V1 )  ( (float)(V1) * INV_FF<float>() )

#define FFFF_TO_FLOAT( V1 )  ( (float)(V1) * INV_FFFF<float>() )
#define FLOAT_TO_FFFF( V1 )  ( (int)((V1) * 0xFFFF) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------



// limiting functions
    void getadd_int( const int min, const int value, const int max, int* valuetoadd );
    void getadd_flt( const float min, const float value, const float max, float* valuetoadd );

// random functions
    int generate_irand_pair( const IPair num );
    int generate_irand_range( const FRange num );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif

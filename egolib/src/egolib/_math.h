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

template <typename Type>
int sgn(const Type& x) {
    return LAMBDA(0 == x, 0, LAMBDA(x > 0, 1, -1));
}

#if defined(TEST_NAN_RESULT)
#    define LOG_NAN(XX)      if( ieee32_bad(XX) ) log_error( "**** A math operation resulted in an invalid result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#else
#    define LOG_NAN(XX)
#endif

struct Facing {
private:
    // The canonical range of unit facing is 0 = UINT16_MIN, 360 = 2^16-1 = UINT16_MAX.
    int32_t angle;

    static void constrain(int32_t& angle) {
        while (angle < -static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) {
            angle = angle + static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
        }
        while (angle > +static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) {
            angle = angle - static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
        }
    }

public:
    explicit Facing(const Ego::Math::Turns& other) : angle(0) {
        static const float s = static_cast<float>(static_cast<int32_t>(std::numeric_limits<uint16_t>::max()) + 1);
        this->angle = int32_t(float(other) * s);
        constrain(this->angle);
    }
    explicit Facing(const Ego::Math::Degrees& x) : Facing(Ego::Math::Turns(x)) {
        /* Intentionally left empty. */
    }
    explicit Facing(const Ego::Math::Radians& x) : Facing(Ego::Math::Turns(x)) {
        /* Intentionally left empty. */
    }
    // int32_t always fits into int32_t.
    explicit Facing(int32_t angle) : angle(0) {
        // Do *not* normalize the angle.
        this->angle = angle;
        constrain(this->angle);
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
    explicit operator uint16_t() const {
        int32_t x = angle;
        while (x < -static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) {
            x = x + static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
        }
        while (x > +static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) {
            x = x - static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
        }
        return x;
    }
    explicit operator int32_t() const {
        return angle;
    }
    explicit operator Ego::Math::Turns() const {
        static const int32_t s = static_cast<float>(static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) + 1;
        static const float inv_s = 1.0f / s;
        return Ego::Math::Turns(float(angle) * s);
    }
    explicit operator Ego::Math::Radians() const {
        return Ego::Math::Radians((Ego::Math::Turns)(*this));
    }
    explicit operator Ego::Math::Degrees() const {
        return Ego::Math::Degrees((Ego::Math::Turns)(*this));
    }

public:
    Facing operator+() const {
        return *this;
    }
    Facing operator-() const {
        return Facing(-angle);
    }

public:
    Facing operator*(float other) const {
        return Facing(int32_t(float(angle) * other));
    }

    Facing operator+(const Facing& other) const {
        // this.angle and other.angle are in the canonical range of 0 and 2^16-1.
        // (2^16 - 1) + (2^16-1) is always smaller than the maximum value of int32_t.
        return Facing(angle + other.angle);
    }

    Facing operator-(const Facing& other) const {
        // this angle and other.angle are in the canonical range of 0 and 2^16-1.
        // 0         -    2^16-1 is always greater than the minimum value of int32_t.
        return Facing(angle - other.angle);
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

public:
    static Facing random(bool negative = false) {
        int32_t x = static_cast<int32_t>(Random::next<uint16_t>(std::numeric_limits<uint16_t>::max()));
        if (negative) {
            x = Random::nextBool() ? -x : -x;
        }
        return Facing(x);
    }
};

struct EulerFacing {
    Facing x, y, z;
    EulerFacing() : x(), y(), z() {}
    EulerFacing(const Facing& x, const Facing& y, const Facing& z) : x(x), y(y), z(z) {}
    EulerFacing(const EulerFacing& other) : x(other.x), y(other.y), z(other.z) {}
    const Facing& operator[](size_t index) const {
        switch (index) {
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            default:
                throw Id::OutOfBoundsException(__FILE__, __LINE__, "index out of range");
        };
    }
    Facing& operator[](size_t index) {
        switch (index) {
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            default:
                throw Id::OutOfBoundsException(__FILE__, __LINE__, "index out of range");
        };
    }
};

template <>
inline int sgn<Facing>(const Facing& x) {
    return sgn(int32_t(x));
}

namespace std {
inline float sin(const Facing& x) {
    return sin((Ego::Math::Radians)x);
}

inline float cos(const Facing& x) {
    return cos((Ego::Math::Radians)x);
}
}

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
inline Facing TurnToFacing(const Ego::Math::Turns& x) {
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
inline Ego::Math::Turns FacingToTurn(const Facing& x) {
    return static_cast<Ego::Math::Turns>(x);
}


/**
 * @brief
 *  Convert an angle "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$) to radians.
 * @param x
 *  the angle in facing
 * @return
 *  the angle in radians
 */
inline Ego::Math::Radians FacingToRadian(const Facing& x) {
    return static_cast<Ego::Math::Radians>(x);
}

/**
 * @brief
 *  Convert an angle from radians to "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$).
 * @param x
 *  the angle in radians
 * @return
 *  the angle in "facing"
 */
inline Facing RadianToFacing(const Ego::Math::Radians& x) {
    return TurnToFacing(Ego::Math::Turns(x));
}

// conversion functions
Facing vec_to_facing(const float dx, const float dy);
void facing_to_vec(const Facing& facing, float * dx, float * dy);

// rotation functions
int terp_dir(const FACING_T& majordir, const FACING_T& minordir, const int weight);

//--------------------------------------------------------------------------------------------

#if !defined(SGN)
#    define SGN(X) sgn(X)
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


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

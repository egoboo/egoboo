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

/// @file   egolib/math/Math.hpp
/// @brief  Math utility functions
/// @author Johan Jansen
/// @author Michael Heilmann
#pragma once

namespace Ego
{
namespace Math
{

/**
 * @brief
 *  Get \f$\pi\f$.
 * @return
 *  \f$\pi\f$
 * @remark
 *  Overloads for @a float and @a double are provided.
 */
template <typename Type>
Type pi();

template <>
inline float pi<float>()
{
    return 3.1415926535897932384626433832795f;
}

template <>
inline double pi<double>()
{
    return 3.1415926535897932384626433832795;
}

/**
 * @brief
 *  Get \f$\frac{1}{\pi}\f$.
 * @return
 *  \f$\frac{1}{\pi}\f$
 * @remark
 *  Specializations for @a float and @a double are provided.
 */
template <typename Type>
Type invPi();

template <>
inline float invPi<float>()
{
    return 0.31830988618379067153776752674503f;
}

template <>
inline double invPi<double>()
{
    return 0.31830988618379067153776752674503;
}

/**
 * @brief
 *  Get \f$2 \cdot \pi\f$.
 * @return
 *  \f$2 \cdot \pi\f$
 * @remark
 *  Specializations for @a float and @a double are provided.
 */
template <typename Type>
Type twoPi();

template <>
inline float twoPi<float>()
{
    return 6.283185307179586476925286766559f;
}

template <>
inline double twoPi<double>()
{
    return 6.283185307179586476925286766559;
}

/**
 * @brief
 *  Get \f$\frac{1}{2 \cdot \pi}\f$.
 * @return
 *  \f$\frac{1}{2 \cdot \pi}\f$
 * @remark
 *  Specializations for @a float and @a double are provided.
 */
template <typename Type>
Type invTwoPi();

template <>
inline float invTwoPi<float>()
{
    return 0.15915494309189533576888376337251f;
}

template <>
inline double invTwoPi<double>()
{
    return 0.15915494309189533576888376337251;
}

/**
 * @brief
 *  Get \f$\frac{\pi}{2}\f$.
 * @return
 *  \f$\frac{\pi}{2}\f$
 * @remark
 *  Specializations for @a float and @a double are provided.
 */
template <typename Type>
Type piOverTwo();

template <>
inline float piOverTwo<float>()
{
    return 1.5707963267948966192313216916398f;
}

template <>
inline double piOverTwo<double>()
{
    return 1.5707963267948966192313216916398;
}

/**
 * @brief
 *  Get \f$\frac{\pi}{4}\f$.
 * @return
 *  \f$\frac{\pi}{4}\f$
 * @remark
 *  Specializations for @a float and @a double are provided.
 */
template <typename Type>
Type piOverFour();

template <>
inline float piOverFour<float>()
{
    return 0.78539816339744830961566084581988f;
}

template <>
inline double piOverFour<double>()
{
    return 0.78539816339744830961566084581988;
}

} // namespace Math
} // namespace Ego

namespace Ego
{
namespace Math
{

/**
 * @brief
 *  Get the smallest positive power of two greater than or equal to a given value.
 * @param x
 *  the value
 * @return
 *  the smallest positive power of two greater than or equal to @a x
 * @throw std::domain_error
 *  if the smallest positive power of two greater than or equal to @a x
 *  is not represantable as a value of type @a T
 * @remark
 *  The only specialization currently provided is for the type @a int.
 * @remark
 *  MH: The template/its specialization is not used in high-performance
 *  code, so I do think exhaustive error checking is approriate because
 *  we simply have the time to do so.
 */
template <typename T>
T powerOfTwo(T x);

template <>
inline int powerOfTwo<int>(int x)
{
    // std::numeric_limits<int>::max() returns something like 0 111 ... 111; shifting this to the right by
    // one digit gives 0 011 ... 111 and adding 1 it becomes 0 100 ... 000 which is greatest power of two
    // represantable as an int.
    static const int HIGH = (std::numeric_limits<int>::max() >> 1) + 1;
    if (x >= HIGH)
    {
        if (x > HIGH)
        {
            throw std::domain_error("x is too big");
        }
        else
        {
            return x;
        }
    }
    // The following can not overflow now.
    int y = 1;
    while (y < x)
    {
        y <<= 1;
    }
    return y;
}

} // namespace Math
} // namespace Ego

namespace Ego
{
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
    const T& constrain(const T& n, const T& lower, const T& upper)
    {
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
    T sq(const T &value)
    {
        return value * value;
    }

    /**
    * @brief
    *   Clamps the N first bits of an integral number.
    * @param value
    *   The number which should be bit clamped
    * @return
    *   An integer where the N first bits have been clamped
    **/
    template<size_t BITS, typename T>
    int clipBits(const T &value)
    {
        static_assert(BITS > 0, "Cannot clamp zero bits");
        return static_cast<int>(value) & (static_cast<int>(1 << BITS)-1);
    }

    /**
     * @brief
     *  Convert an angle from degree to radian.
     * @param x
     *  the angle in degree
     * @return
     *  the angle in radian
     */
    template<typename T>
    inline T degToRad(const T &x)
    {
        static_assert(std::is_floating_point<T>::value, "T must be of floating point type");
        return (x * Ego::Math::pi<T>()) / static_cast<T>(180);
    }

    /**
     * @brief
     *  Convert an angle from degree to radian.
     * @param x
     *  the angle in degree
     * @return
     *  the angle in radian
     */
    template<typename T>
    inline T radToDeg(const T &x)
    {
        static_assert(std::is_floating_point<T>::value, "T must be of floating point type");
        return (x * static_cast<T>(180)) / Ego::Math::pi<T>();
    }

} // namespace Math
} // namespace Ego

namespace Ego
{
namespace Math
{
/**
 * @brief
 *  Get \f$\sqrt{2}\f$.
 * @return
 *  \f$\sqrt{2}\f$
 * @remark
 *  Specializations for @a float and @a double are provided.
 */
template <typename Type>
Type sqrtTwo();

template <>
inline float sqrtTwo<float>()
{
    return 1.4142135623730950488016887242097f;
}

template <>
inline double sqrtTwo<double>()
{
    return 1.4142135623730950488016887242097;
}

/**
 * @brief
 *  Get \f$\frac{1}{\sqrt{2}}\f$.
 * @return
 *  \f$\frac{1}{\sqrt{2}}\f$
 * @remark
 *  Specializations for @a float and @a double are provided.
 */
template <typename Type>
Type invSqrtTwo();

template <>
inline float invSqrtTwo()
{
    return 0.70710678118654752440084436210485f;
}

template <>
inline double invSqrtTwo()
{
    //      .70710678118654757
    //      .70710678118654752440
    return 0.70710678118654752440084436210485;
}

} // namespace Math
} // namespace Ego

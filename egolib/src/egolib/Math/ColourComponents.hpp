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

/// @file   egolib/Math/ColourComponents.hpp
/// @brief  Colours components of misc. colour spaces.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/ColourSpace.hpp"

/// If defined and @a 1, then colour components allow for setting the component values.
#define EGO_MATH_COLOURCOMPONENTS_MUTABLE (1)

namespace Ego {
namespace Math {
    
namespace Internal {
    
inline float b2f(uint8_t x) {
    return std::max(std::min(float(x) * 255.0f, 1.0f), 0.0f);
}

inline uint8_t d2b(double x) {
    // (1) Ensure that y is within the bounds of 0 and 1.
    double y = std::max(0.0, std::min(1.0, x));
    // (2) Multiply y by 256.0. This ensures that numbers very
    //     close to 1.0 (e.g. 0.999) are not mapped to 254 but
    //     to 255 as desired. As a consequence, however, the case
    //     in which y is really 1.0f must be handled separatedly.
    uint8_t b = std::floor(y == 1.0 ? 255.0 : y * 256.0);
    // (3) Return the result.
    return b;
}

inline uint8_t f2b(float x) {
    // (1) Ensure that y is within the bounds of 0 and 1.
    float y = std::max(0.0f, std::min(1.0f, x));
    // (2) Multiply y by 256.0f. This ensures that numbers very
    //     close to 1.0f (e.g. 0.999f) are not mapped to 254 but
    //     to 255 as desired. As a consequence, however, the case
    //     in which y is really 1.0f must be handled separatedly.
    uint8_t b = std::floor(y == 1.0f ? 255.0f : y * 256.0f);
    // (3) Return the result.
    return b;
}

} // Internal

template <typename _ColourSpaceType, typename _Enabled = void>
struct ColourComponents;


template <typename _ColourSpaceType>
struct ColourComponents<_ColourSpaceType, std::enable_if_t<Internal::IsL<_ColourSpaceType>::value>> {
protected:
    using ColourSpaceType = _ColourSpaceType;
    using ComponentType = typename ColourSpaceType::ComponentType;

private:
    /**
     * @brief The luminance component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType l;

protected:
    /**
    * @brief
    *  Create a color.
    * @param l
    *  the component value of the luminance component
    * @throws Id::OutOfBoundsException
    *  if @a l is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
    */
    ColourComponents(ComponentType l)
        : l(l) {
        if (l < ColourSpaceType::min() || l > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "luminance component out of bounds");
        }
    }

protected:
    void assign(const ColourComponents& other) {
        l = other.l;
    }

protected:
    // If colour spaces are the same.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_OtherColourSpaceType, _ColourSpaceType>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : l(other.Luminance()) {
        // Intentionally empty.
    }

    // If Lf to Lb.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, RGBb>::value &&
                         std::is_same<_OtherColourSpaceType, Lf>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : l(Internal::f2b(other.getLuminance())) {
        // Intentionally empty.
    }

    // If Lb to Lf.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, Lf>::value &&
                         std::is_same<_OtherColourSpaceType, Lb>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : l(Internal::b2f(other.getLuminance())) {
        // Intentionally empty.
    }

public:
    /**
     * @brief
     *  Get the value of the luminance component.
     * @return
     *  the value of the luminance component
     */
    ComponentType getLuminance() const {
        return l;
    }

#if defined(EGO_MATH_COLOURCOMPONENTS_MUTABLE) && 1 == EGO_MATH_COLOURCOMPONENTS_MUTABLE
    /**
     * @brief
     *  Set the value of the luminance component.
     * @param l
     *  the value of the luminance component
     * @throws Id::OutOfBoundsException
     *  if @a l is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    void setLuminance(ComponentType l) {
        if (l < ColourSpaceType::min() || l > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "luminance component out of bounds");
        }
        this->l = l;
    }
#endif

};

template <typename _ColourSpaceType>
struct ColourComponents<_ColourSpaceType, std::enable_if_t<Internal::IsLA<_ColourSpaceType>::value>> {
protected:
    using ColourSpaceType = _ColourSpaceType;
    using ComponentType = typename ColourSpaceType::ComponentType;

private:
    /**
     * @brief The luminance component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType l;

    /**
     * @brief The alpha component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType a;

protected:
    void assign(const ColourComponents& other) {
        l = other.l;
        a = other.a;
    }

protected:
    // If colour spaces are the same.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_OtherColourSpaceType, _ColourSpaceType>::value, int *> = 0
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : l(other.getLuminance()), a(other.getAlpha()) {
        // Intentionally empty.
    }

    // If LAf to LAb.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, LAb>::value &&
                         std::is_same<_OtherColourSpaceType, LAf>::value, int *> = 0
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : l(Internal::b2f(other.getLuminance())), a(f2b(other.getAlpha())) {
        // Intentionally empty.
    }

    // If LAb to LAf.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, LAf>::value &&
                         std::is_same<_OtherColourSpaceType, LAb>::value, int *> = 0
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : l(b2f(other.getLuminance())), a(b2f(other.getAlpha())) {
        // Intentionally empty.
    }
    
protected:
    /**
     * @brief
     *  Create a color.
     * @param l
     *  the component value of the luminance component
     * @param a
     *  the component value of the alpha component
     * @throws Id::OutOfBoundsException
     *  if @a l or @a a are not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    ColourComponents(ComponentType l, ComponentType a)
        : l(l), a(a) {
        if (l < ColourSpaceType::min() || l > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "luminance component out of bounds");
        }
        if (a < ColourSpaceType::min() || a > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "alpha component out of bounds");
        }
    }

    /**
     * @brief
     *  Create a colour.
     * @param l
     *  the l component of the colour as a colour in L colour space
     * @param a
     *  the alpha component
     * @throws Id::OutOfBoundsException
     *  if @a a is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    ColourComponents(const ColourComponents<Opaque<ColourSpaceType>>& other, ComponentType a)
        : l(other.getLuminance()), a(a) {
        /* l is known to be within bounds. */
        if (a < ColourSpaceType::min() || a > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "alpha component out of bounds");
        }
    }

public:
    /**
     * @brief
     *  Get the value of the luminance component.
     * @return
     *  the value of the luminance component
     */
    ComponentType getLuminance() const {
        return l;
    }

#if defined(EGO_MATH_COLOURCOMPONENTS_MUTABLE) && 1 == EGO_MATH_COLOURCOMPONENTS_MUTABLE
    /**
     * @brief
     *  Set the value of the luminance component.
     * @param l
     *  the value of the luminance component
     * @throws Id::OutOfBoundsException
     *  if @a l is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    void setLuminance(ComponentType l) {
        if (l < ColourSpaceType::min() || l > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "luminance component out of bounds");
        }
        this->l = l;
    }
#endif

    /**
     * @brief
     *  Get the value of the alpha component.
     * @return
     *  the value of the alpha component
     */
    ComponentType getAlpha() const {
        return a;
    }

#if defined(EGO_MATH_COLOURCOMPONENTS_MUTABLE) && 1 == EGO_MATH_COLOURCOMPONENTS_MUTABLE
    /**
     * @brief
     *  Set the value of the alpha component.
     * @param a
     *  the value of the alpha component
     * @throws Id::OutOfBoundsException
     *  if @a a is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    void setAlpha(const ComponentType a) {
        if (a < ColourSpaceType::min() || a > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "alpha component out of bounds");
        }
        this->a = a;
    }
#endif
};

template <typename _ColourSpaceType>
struct ColourComponents<_ColourSpaceType, std::enable_if_t<Internal::IsRgb<_ColourSpaceType>::value>> {
protected:
    using ColourSpaceType = _ColourSpaceType;
    using ComponentType = typename ColourSpaceType::ComponentType;

private:
    static_assert(Internal::IsRgb<_ColourSpaceType>::value, "not an RGB colour space type");
    static_assert(_ColourSpaceType::hasRgb() && !_ColourSpaceType::hasA() && !_ColourSpaceType::hasL(), "not an RGB colour space type");

    /**
     * @brief The red component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType r;

    /**
     * @brief The green component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType g;

    /**
     * @brief The blue component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType b;

protected:
    // If colour spaces are the same.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_OtherColourSpaceType, _ColourSpaceType>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : r(other.getRed()), g(other.getGreen()), b(other.getBlue()) {
        // Intentionally empty.
    }

    // If RGBf to RGBb.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, RGBb>::value &&
                         std::is_same<_OtherColourSpaceType, RGBf>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : r(Internal::f2b(other.getRed())), g(Internal::f2b(other.getGreen())), b(Internal::f2b(other.getBlue())) {
        // Intentionally empty.
    }

    // If RGBb to RGBf.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, RGBf>::value &&
                         std::is_same<_OtherColourSpaceType, RGBb>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
      : r(Internal::b2f(other.getRed())), g(Internal::b2f(other.getGreen())), b(Internal::b2f(other.getBlue())) {
        // Intentionally empty.
    }
    

protected:
    // If Lb to RGBb.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, RGBb>::value &&
                         std::is_same<_OtherColourSpaceType, Lb>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : r(other.getLuminance()), g(other.getLuminance()), b(other.getLuminance()) {
        // Intentionally empty.
    }

    // If Lf to RGBf.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, RGBf>::value &&
                         std::is_same<_OtherColourSpaceType, Lf>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : r(other.getLuminance()), g(other.getLuminance()), b(other.getLuminance()) {
        // Intentionally empty.
    }

protected:
    /**
     * @brief
     *  Construct this color.
     * @param r
     *  the component value of the red component
     * @param g
     *  the component value of the green component
     * @param b
     *  the component value of the blue component
     * @throws Id::OutOfBoundsException
     *  if @a r, @a g, or @a b are not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    ColourComponents(ComponentType r, ComponentType g, ComponentType b)
        : r(r), g(g), b(b) {
        if (r < ColourSpaceType::min() || r > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "red component out of bounds");
        }
        if (g < ColourSpaceType::min() || g > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "green component out of bounds");
        }
        if (b < ColourSpaceType::min() || b > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "blue component out of bounds");
        }
    }

protected:
    void assign(const ColourComponents& other) {
        r = other.r;
        g = other.g;
        b = other.b;
    }

public:
    /**
     * @brief
     *  Get the value of the red component.
     * @return
     *  the value of the red component
     */
    ComponentType getRed() const {
        return r;
    }

#if defined(EGO_MATH_COLOURCOMPONENTS_MUTABLE) && 1 == EGO_MATH_COLOURCOMPONENTS_MUTABLE
    /**
     * @brief
     *  Set the value of the red component.
     * @param r
     *  the value of the red component
     * @throws Id::OutOfBoundsException
     *  if @a r is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    void setRed(ComponentType r) {
        if (r < ColourSpaceType::min() || r > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "red component out of bounds");
        }
        this->r = r;
    }
#endif

    /**
     * @brief
     *  Get the value of the green component.
     * @return
     *  the value of the green component
     */
    ComponentType getGreen() const {
        return g;
    }

#if defined(EGO_MATH_COLOURCOMPONENTS_MUTABLE) && 1 == EGO_MATH_COLOURCOMPONENTS_MUTABLE
    /**
     * @brief
     *  Set the value of the green component.
     * @param g
     *  the value of the green component
     * @throws Id::OutOfBoundsException
     *  if @a g is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    void setGreen(ComponentType g) {
        if (g < ColourSpaceType::min() || g > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "green component out of bounds");
        }
        this->g = g;
    }
#endif

    /**
     * @brief
     *  Get the value of the blue component.
     * @return
     *  the value of the blue component
     */
    ComponentType getBlue() const {
        return b;
    }

#if defined(EGO_MATH_COLOURCOMPONENTS_MUTABLE) && 1 == EGO_MATH_COLOURCOMPONENTS_MUTABLE
    /**
     * @brief
     *  Set the value of the blue component.
     * @param b
     *  the value of the blue component
     * @throws Id::OutOfBoundsException
     *  if @a b is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    void setBlue(ComponentType b) {
        if (b < ColourSpaceType::min() || b > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "blue component out of bounds");
        }
        this->b = b;
    }
#endif
};

template <typename _ColourSpaceType>
struct ColourComponents<_ColourSpaceType, std::enable_if_t<Internal::IsRgba<_ColourSpaceType>::value>> {
protected:
    using ColourSpaceType = _ColourSpaceType;
    using ComponentType = typename ColourSpaceType::ComponentType;

private:
    static_assert(Internal::IsRgba<_ColourSpaceType>::value, "not an RGBA colour space type");
    static_assert(_ColourSpaceType::hasRgb() && _ColourSpaceType::hasA() && !_ColourSpaceType::hasL(), "not an RGBA colour space type");

    /**
     * @brief The red component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType r;

    /**
     * @brief The green component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType g;

    /**
     * @brief The blue component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType b;

    /**
     * @brief The blue component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType a;

protected:
    // If colour spaces are the same.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_OtherColourSpaceType, _ColourSpaceType>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : r(other.getRed()), g(other.getGreen()), b(other.getBlue()), a(other.getAlpha()) {
        // Intentionally empty.
    }

    // If RGBAf to RGBAb.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, RGBAb>::value &&
                         std::is_same<_OtherColourSpaceType, RGBAf>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : r(Internal::f2b(other.getRed())),
          g(Internal::f2b(other.getGreen())), 
          b(Internal::f2b(other.getBlue())), 
          a(Internal::f2b(other.getAlpha())) {
        // Intentionally empty.
    }

    // If RGBAb to RGBAf.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, RGBAf>::value &&
                         std::is_same<_OtherColourSpaceType, RGBAb>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : r(Internal::b2f(other.getRed())),
          g(Internal::b2f(other.getGreen())),
          b(Internal::b2f(other.getBlue())),
          a(Internal::b2f(other.getAlpha())) {
        // Intentionally empty.
    }

protected:
    // If LAb to RGBAb.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, RGBAb>::value &&
                         std::is_same<_OtherColourSpaceType, LAb>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : r(other.getLuminance()), g(other.getLuminance()), b(other.getLuminance()), a(other.getAlpha()) {
        // Intentionally empty.
    }

    // If LAf to RGBAf.
    template <
        typename _OtherColourSpaceType,
        std::enable_if_t<std::is_same<_ColourSpaceType, RGBAf>::value &&
                         std::is_same<_OtherColourSpaceType, LAf>::value, int *> = nullptr
    >
    ColourComponents(const ColourComponents<_OtherColourSpaceType>& other)
        : r(other.getLuminance()), g(other.getLuminance()), b(other.getLuminance()), a(other.getAlpha()) {
        // Intentionally empty.
    }

protected:
    /**
     * @brief
     *  Create a color.
     * @param r
     *  the component value of the red component
     * @param g
     *  the component value of the green component
     * @param b
     *  the component value of the blue component
     * @param a
     *  the component value of the alpha component
     * @throws Id::OutOfBoundsException
     *  if @a r, @a g, @a b, or @a a are not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    ColourComponents(ComponentType r, ComponentType g, ComponentType b, ComponentType a)
        : r(r), g(g), b(b), a(a) {
        if (r < ColourSpaceType::min() || r > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "red component out of bounds");
        }
        if (g < ColourSpaceType::min() || g > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "green component out of bounds");
        }
        if (b < ColourSpaceType::min() || b > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "blue component out of bounds");
        }
        if (a < ColourSpaceType::min() || a > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "alpha component out of bounds");
        }
    }

    /**
     * @brief
     *  Create a colour.
     * @param other
     *  the red, green and blue components of the colour as a colour in RGB colour space
     * @param a
     *  the alpha component
     * @throws Id::OutOfBoundsException
     *  if @a a is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    ColourComponents(const ColourComponents<Opaque<ColourSpaceType>>& other, ComponentType a)
        : r(other.getRed()), g(other.getGreen()), b(other.getBlue()), a(a) {
        /* r, g, b are known to be within in bounds. */
        if (a < ColourSpaceType::min() || a > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "alpha component out of bounds");
        }
    }

protected:
    void assign(const ColourComponents& other) {
        r = other.r;
        g = other.g;
        b = other.b;
        a = other.a;
    }

public:
    /**
     * @brief
     *  Get the value of the red component.
     * @return
     *  the value of the red component
     */
    ComponentType getRed() const {
        return r;
    }

#if defined(EGO_MATH_COLOURCOMPONENTS_MUTABLE) && 1 == EGO_MATH_COLOURCOMPONENTS_MUTABLE
    /**
     * @brief
     *  Set the value of the red component.
     * @param r
     *  the value of the red component
     * @throws Id::OutOfBoundsException
     *  if @a r is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    void setRed(ComponentType r) {
        if (r < ColourSpaceType::min() || r > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "red component out of bounds");
        }
        this->r = r;
    }
#endif

    /**
     * @brief
     *  Get the value of the green component.
     * @return
     *  the value of the green component
     */
    ComponentType getGreen() const {
        return g;
    }

#if defined(EGO_MATH_COLOURCOMPONENTS_MUTABLE) && 1 == EGO_MATH_COLOURCOMPONENTS_MUTABLE
    /**
     * @brief
     *  Set the value of the green component.
     * @param g
     *  the value of the green component
     * @throws Id::OutOfBoundsException
     *  if @a g is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    void setGreen(ComponentType g) {
        if (g < ColourSpaceType::min() || g > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "green component out of bounds");
        }
        this->g = g;
    }
#endif

    /**
     * @brief
     *  Get the value of the blue component.
     * @return
     *  the value of the blue component
     */
    ComponentType getBlue() const {
        return b;
    }

#if defined(EGO_MATH_COLOURCOMPONENTS_MUTABLE) && 1 == EGO_MATH_COLOURCOMPONENTS_MUTABLE
    /**
     * @brief
     *  Set the value of the blue component.
     * @param b
     *  the value of the blue component
     * @throws Id::OutOfBoundsException
     *  if @a b is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    void setBlue(ComponentType b) {
        if (b < ColourSpaceType::min() || b > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "blue component out of bounds");
        }
        this->b = b;
    }
#endif

    /**
     * @brief
     *  Get the value of the alpha component.
     * @return
     *  the value of the alpha component
     */
    ComponentType getAlpha() const {
        return a;
    }

#if defined(EGO_MATH_COLOURCOMPONENTS_MUTABLE) && 1 == EGO_MATH_COLOURCOMPONENTS_MUTABLE
    /**
     * @brief
     *  Set the value of the alpha component.
     * @param a
     *  the value of the alpha component
     * @throws Id::OutOfBoundsException
     *  if @a a is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    void setAlpha(const ComponentType a) {
        if (a < ColourSpaceType::min() || a > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "alpha component out of bounds");
        }
        this->a = a;
    }
#endif

};

} // namespace Math
} // namespace Ego

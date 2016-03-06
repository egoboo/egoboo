#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {

/// @brief The RGB colour space type.
struct RGBf {
    typedef float ComponentType;
    static constexpr bool isRgb() { return true; }
    static constexpr bool hasA() { return false; }

    /// @brief The number of components of a colour in this colour space.
    /// @return the number of components of a colour in this colour space
    static constexpr size_t count() { return 3; }
    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0.0f; }
    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 1.0f; }

};

/// @brief The RGBA colour space type.
struct RGBAf {
    typedef float ComponentType;
    static constexpr bool isRgb() { return true; }
    static constexpr bool hasA() { return true; }

    /// @brief The number of components of a colour in this colour space.
    /// @return the number of components of a colour in this colour space
    static constexpr size_t count() { return 4; }
    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0.0f; }
    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 1.0f; }

};

/// @brief The RGB colour space type.
struct RGBb {
    typedef uint8_t ComponentType;
    static constexpr bool isRgb() { return true; }
    static constexpr bool hasA() { return false; }

    /// @brief The number of components of a colour in this colour space.
    /// @return the number of components of a colour in this colour space
    static constexpr size_t count() { return 3; }
    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0; }
    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 255; }
};

/// @brief The RGBA colour space type.
struct RGBAb {
    typedef uint8_t ComponentType;
    static constexpr bool isRgb() { return true; }
    static constexpr bool hasA() { return true; }

    /// @brief The number of components of a colour in this colour space.
    /// @return the number of components of a colour in this colour space
    static constexpr size_t count() { return 4; }
    /**
    * @brief Get the minimum component value.
    * @return the minimum component value
    */
    static constexpr ComponentType min() { return 0; }
    /**
    * @brief Get the maximum component value.
    *  @return the maximum component value
    */
    static constexpr ComponentType max() { return 255; }
};

/**
 * @brief Get the type of the opaque color space of a specified color space.
 * @remark The opaque color space consists of all opaque colors of the specified color space.
 */
template <typename _ColourSpaceType>
struct _Opaque;

template <>
struct _Opaque<RGBf> { typedef RGBf Type; };
template <>
struct _Opaque<RGBAf> { typedef RGBf Type; };

template <typename _ColourSpaceType>
using Opaque = typename _Opaque<_ColourSpaceType>::Type;

template <typename _ColourSpaceType, typename _Enabled = void>
struct ColourComponents;

namespace Internal {

template <typename _ColourSpaceType>
struct IsRgb {
    static constexpr bool value = _ColourSpaceType::isRgb() && !_ColourSpaceType::hasA();
};

}

template <typename _ColourSpaceType>
struct ColourComponents<_ColourSpaceType, std::enable_if_t<Internal::IsRgb<_ColourSpaceType>::value>> {
protected:
    typedef _ColourSpaceType ColourSpaceType;
    typedef typename ColourSpaceType::ComponentType ComponentType;

private:
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
    ColourComponents(const ColourComponents& other)
        : r(other.r), g(other.g), b(other.b) {
        // Intentionally empty.
    }

    /**
     * @brief
     *  Create a color.
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

    /**
     * @brief
     *  Get the value of the green component.
     * @return
     *  the value of the green component
     */
    ComponentType getGreen() const {
        return g;
    }

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

    /**
     * @brief
     *  Get the value of the blue component.
     * @return
     *  the value of the blue component
     */
    ComponentType getBlue() const {
        return b;
    }

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
};

namespace Internal {

template <typename _ColourSpaceType>
struct IsRgba {
    static constexpr bool value = _ColourSpaceType::isRgb() && _ColourSpaceType::hasA();
};

}

template <typename _ColourSpaceType>
struct ColourComponents<_ColourSpaceType, std::enable_if_t<Internal::IsRgba<_ColourSpaceType>::value>> :
    public ColourComponents<Opaque<_ColourSpaceType>> {
protected:
    typedef _ColourSpaceType ColourSpaceType;
    typedef typename ColourSpaceType::ComponentType ComponentType;

private:
    /**
     * @brief The blue component value.
     * @invariant Within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive).
     */
    ComponentType a;
protected:
    ColourComponents(const ColourComponents& other)
        : ColourComponents<Opaque<ColourSpaceType>>(other), a(other.a) {
        // Intentionally empty.
    }
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
        : ColourComponents<Opaque<ColourSpaceType>>(r, g, b), a(a) {
        if (a < ColourSpaceType::min() || a > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "alpha component out of range");
        }
    }
    /**
     * @brief
     *  Create a colour.
     * @param rgb
     *  the red, green and blue components of the colour as a colour in RGB colour space
     *  the colour in RGB space
     * @param a
     *  the alpha component
     * @throws Id::OutOfBoundsException
     *  if @a a is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    ColourComponents(const ColourComponents<Opaque<ColourSpaceType>>& other, ComponentType a) :
        ColourComponents<Opaque<ColourSpaceType>>(other),
        a(a) {
        if (a < ColourSpaceType::min() || a > ColourSpaceType::max()) {
            throw Id::OutOfBoundsException(__FILE__, __LINE__, "alpha component out of bounds");
        }
    }

    void assign(const ColourComponents& other) {
        ColourComponents<Opaque<ColourSpaceType>>::assign(other);
        a = other.a;
    }
public:
    /**
     * @brief
     *  Get the value of the alpha component.
     * @return
     *  the value of the alpha component
     */
    ComponentType getAlpha() const {
        return a;
    }

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
};

template <typename _ColourSpaceType, typename _Enabled = void>
struct Colour;

} // namespace Math
} // namespace Ego

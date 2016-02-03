#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {

/// @brief The RGB colour space type.
struct RGB {
    /// @brief The number of components of a colour in this colour space.
    /// @return the number of components of a colour in this colour space
    static constexpr size_t components() { return 3; }
};

/// @brief The RGBA colour space type.
struct RGBA {
    /// @brief The number of components of a colour in this colour space.
    /// @return the number of components of a colour in this colour space
    static constexpr size_t components() { return 4; }
};

/// @brief The HSV colour space type.
struct HSV {
    /// @brief The number of components of a colour in this colour space.
    /// @return the number of components of a colour in this colour space
    static constexpr size_t components() { return 3; }
};

/// @brief The HSVA colour space type.
struct HSVA {
    /// @brief The number of components of a colour in this colour space.
    /// @return the number of components of a colour in this colour space
    static constexpr size_t components() { return 4; }
};

template <typename _ColourSpaceType, typename _ComponentType, typename _Enabled = void>
struct ColourComponents;

template <typename _ComponentType>
struct ColourComponents<RGB, _ComponentType, std::enable_if_t<std::is_floating_point<_ComponentType>::value>> {
protected:
    typedef _ComponentType ComponentType;
    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return ComponentType(0); }
    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return ComponentType(1); }
private:
    /**
     * @brief The red component value.
     * @invariant Within the bounds of MyType::min() (inclusive) and MyType::max() (inclusive).
     */
    ComponentType r;
    /**
     * @brief The green component value.
     * @invariant Within the bounds of MyType::min() (inclusive) and MyType::max() (inclusive).
     */
    ComponentType g;
    /**
     * @brief The blue component value.
     * @invariant Within the bounds of MyType::min() (inclusive) and MyType::max() (inclusive).
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
     * @throws std::domain_error
     *  if @a a, @a g or @a b are not within the range of 0 (inclusive) and 1 (inclusive)
     */
    ColourComponents(ComponentType r, ComponentType g, ComponentType b)
        : r(r), g(g), b(b) {
        if (r < min() || r > max()) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "red component out of range");
        }
        if (g < min() || g > max()) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "green component out of range");
        }
        if (b < min() || b > max()) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "blue component out of range");
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
     */
    void setRed(ComponentType r) {
        if (r < 0.0f || r > 1.0f) {
            throw std::domain_error("red component outside range");
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
     */
    void setGreen(ComponentType g) {
        if (g < 0.0f || g > 1.0f) {
            throw std::domain_error("green component outside range");
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
     */
    void setBlue(ComponentType b) {
        if (b < 0.0f || b > 1.0f) {
            throw std::domain_error("blue component outside range");
        }
        this->b = b;
    }
};

template <typename _ComponentType>
struct ColourComponents<RGBA, _ComponentType, std::enable_if_t<std::is_floating_point<_ComponentType>::value>> :
    public ColourComponents<RGB, _ComponentType> {
protected:
    typedef RGBA ColourSpaceType;
    typedef _ComponentType ComponentType;
    static constexpr ComponentType min() { return ComponentType(0); }
    static constexpr ComponentType max() { return ComponentType(1); }
private:
    /**
     * @brief The blue component value.
     * @invariant Within the bounds of MyType::min() (inclusive) and MyType::max() (inclusive).
     */
    ComponentType a;
protected:
    ColourComponents(const ColourComponents& other)
        : ColourComponents<RGB, ComponentType>(other), a(other.a) {
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
     * @throws std::domain_error
     *  if @a a, @a g, @a b or @a a are not within the range of 0 (inclusive) and 1 (inclusive)
     */
    ColourComponents(ComponentType r, ComponentType g, ComponentType b, ComponentType a)
        : ColourComponents<RGB, ComponentType>(r, g, b), a(a) {
        if (a < min() || a > max()) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "alpha component out of range");
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
    */
    ColourComponents(const ColourComponents<RGB, ComponentType>& other, ComponentType a) :
        ColourComponents<RGB, ComponentType>(other),
        a(a) {
        if (a < 0.0f || a > 1.0f) {
            throw std::domain_error("alpha component out of bounds");
        }
    }

    void assign(const ColourComponents& other) {
        ColourComponents<RGB, ComponentType>::assign(other);
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
     */
    void setAlpha(const ComponentType a) {
        if (a < 0.0f || a > 1.0f) {
            throw std::domain_error("alpha component out of range");
        }
        this->a = a;
    }
};

template <typename _ColourSpaceType, typename _ComponentType>
struct Colour;

} // namespace Math
} // namespace Ego

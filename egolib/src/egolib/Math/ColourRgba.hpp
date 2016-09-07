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

/// @file   egolib/Math/ColourRgba.hpp
/// @brief  Colours in RGBA colour spaces.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/ColourRgb.hpp"

namespace Ego {
namespace Math {

template <typename _ColourSpaceType>
struct Colour<_ColourSpaceType, std::enable_if_t<Internal::IsRgba<_ColourSpaceType>::value>> : 
    public ColourComponents<_ColourSpaceType> {
public:
    using ColourSpaceType = _ColourSpaceType;
    using ComponentType = typename ColourSpaceType::ComponentType;
    using MyType = Colour<ColourSpaceType>;

public:
    /**
     * @brief The colour "red".
     * @return the colour "red"
     * @see Ego::Math::Colour<RGBb>::red()
     */
    static const MyType& red() {
        static const MyType colour(Colour<Opaque<ColourSpaceType>>::red(),
                                   ColourSpaceType::max());
        return colour;
    }

    /**
     * @brief The colour "green".
     * @return the colour "green"
     * @see Ego::Math::Colour<RGBb>::green()
     */
    static const MyType& green() {
        static const MyType colour(Colour<Opaque<ColourSpaceType>>::green(),
                                   ColourSpaceType::max());
        return colour;
    }

    /**
     * @brief The colour "blue".
     * @return the colour "blue"
     * @see Ego::Math::Colour<RGBb>::blue()
     */
    static const MyType blue() {
        static const MyType colour(Colour<Opaque<ColourSpaceType>>::blue(),
                                   ColourSpaceType::max());
        return colour;
    }

    /**
     * @brief The colour "white".
     * @return the colour "white"
     * @see Ego::Math::Colour<RGBb>::white()
     */
    static const MyType white() {
        static const MyType colour(Colour<Opaque<ColourSpaceType>>::white(),
                                   ColourSpaceType::max());
        return colour;
    }

    /**
     * @brief The colour "black".
     * @return the colour "black"
     * @see Ego::Math::Colour<RGBb>::black()
     */
    static const MyType black() {
        static const MyType colour(Colour<Opaque<ColourSpaceType>>::black(),
                                   ColourSpaceType::max());
        return colour;
    }

    /**
     * @brief The colour "cyan".
     * @return the colour "cyan"
     * @see Ego::Math::Colour<RGBb>::cyan()
     */
    static const MyType cyan() {
        static const MyType colour(Colour<Opaque<ColourSpaceType>>::cyan(),
                                   ColourSpaceType::max());
        return colour;
    }

    /**
     * @brief The colour "magenta".
     * @return the colour "magenta"
     * @see Ego::Math::Colour<RGBb>::magenta()
     */
    static const MyType magenta() {
        static const MyType colour(Colour<Opaque<ColourSpaceType>>::magenta(),
                                   ColourSpaceType::max());
        return colour;
    }

    /**
     * @brief The colour "yellow".
     * @return the colour "yellow"
     * @see Ego::Math::Colour<RGBb>::yellow()
     */
    static const MyType yellow() {
        static const MyType colour(Colour<Opaque<ColourSpaceType>>::yellow(),
                                   ColourSpaceType::max());
        return colour;
    }

    /**
     * @brief The colour "purple".
     * @return the colour "purple"
     * @see Ego::Math::Colour<RGBb>::purple()
     */
    static const MyType purple() {
        static const MyType colour(Colour<Opaque<ColourSpaceType>>::purple(),
                                   ColourSpaceType::max());
        return colour;
    }

    /**
     * @brief The colour "grey".
     * @return the colour "grey"
     * @see Ego::Math::Colour<RGBb>::grey()
     */
    static const MyType grey() {
        static const MyType colour(Colour<Opaque<ColourSpaceType>>::grey(),
                                   ColourSpaceType::max());
        return colour;
    }

public:
    /**
     * @brief
     *  Default construct with component values corresponding to "opaque black".
     */
    Colour() :
        ColourComponents<ColourSpaceType>(ColourSpaceType::min(),
                                          ColourSpaceType::min(),
                                          ColourSpaceType::min(),
                                          ColourSpaceType::max()) {
        // Intentionally empty
    }

    /**
     * @brief
     *  Construct this colour from the specified color of the corresponding opaque colour space and the specified alpha component value.
     * @param rgb
     *  the red, green and blue components of the colour as a colour in RGB colour space
     * @param a
     *  the component value of the blue component
     * @throws Id::OutOfBoundsException
     *  if @a a is not within the bounds of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    Colour(const Colour<Opaque<ColourSpaceType>>& other, ComponentType a) :
        ColourComponents<ColourSpaceType>(other, a) {
    }

    /**
     * @brief
     *  Construct this colour from the component values of a specified other colour.
     * @param other
     *  the other colour
     */
    template <typename _OtherColourSpaceType>
    Colour(const Colour<_OtherColourSpaceType>& other) :
        ColourComponents<ColourSpaceType>(other) {
        // Intentionally empty.
    }

    /**
     * @brief
     *  Construct this colour from the specified component values.
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
    Colour(ComponentType r, ComponentType g, ComponentType b, ComponentType a) :
        ColourComponents<ColourSpaceType>(r, g, b, a) {
        // Intentionally empty.
    }

public:
    /**
     * @brief Assign this colour from another colour.
     * @param other the other colour
     * @return this colour
     */
    const MyType& operator=(const MyType& other) {
        this->ColourComponents<ColourSpaceType>::assign(other);
        return *this;
    }

    /**
     * @brief
     *  Invert this colour value.
     * @return
     *  the inverted colour
     * @remark
     *  Given a colour  \f$(r,g,b,a)\f$ in real-valued, normalized RGBA space,
     *  then corresponding inverted colour is \f$(1-r,1-g,1-b,a)\f$. Inverting
     *  a colour twice yields the same colour (modula floating-point precision).
     * @remark
     *  The corresponding inverted colour is also known as the complementary colour.
     */
    MyType invert() const {
        return MyType(ColourSpaceType::max() - this->getRed(),
                      ColourSpaceType::max() - this->getGreen(),
                      ColourSpaceType::max() - this->getBlue(),
                      this->getAlpha());
    }

    /**
     * @brief
     *  Make a brighter version of this colour
     * @param brightness
     *  the scalar for brightness increase (0 = normal, 1 = twice as bright)
     * @return
     *  the brighter colour
     * @remark
     *  Given a colour  \f$(r,g,b,a)\f$ in real-valued, normalized RGBA space,
     *  then corresponding inverted colour is \f$(brighness*r,brighness*1,brighness*1,a)\f$. This conversion
     *  is not reverrsible.
     */
    MyType brighter(float brightness) const {
        if (brightness <= 0.0f) return *this;
        brightness += 1.0f;
        return MyType(std::min(ColourSpaceType::max(), this->getRed()*brightness),
                      std::min(ColourSpaceType::max(), this->getGreen()*brightness),
                      std::min(ColourSpaceType::max(), this->getBlue()*brightness),
                      this->getAlpha());
    }

public:
    bool operator==(const MyType& other) const {
        return this->getRed() == other.getRed()
            && this->getGreen() == other.getGreen()
            && this->getBlue() == other.getBlue()
            && this->getAlpha() == other.getAlpha();
    }
    bool operator!=(const MyType& other) const {
        return this->getRed() != other.getRed()
            || this->getGreen() != other.getGreen()
            || this->getBlue() != other.getBlue()
            || this->getAlpha() != other.getAlpha();
    }
};

} // namespace Math
} // namespace Ego

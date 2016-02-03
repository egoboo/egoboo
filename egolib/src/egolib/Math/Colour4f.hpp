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

/// @file   egolib/Math/Colour4f.hpp
/// @brief  Colours in RGBA colour space.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Colour3f.hpp"

namespace Ego {
namespace Math {

template <>
struct Colour<RGBA, float> : public ColourComponents<RGBA, float> {
public:
    typedef float ComponentType;
    typedef RGBA ColourSpaceType;
    typedef Colour<ColourSpaceType, ComponentType> MyType;

public:
    /**
     * @brief The colour "red".
     * @return the colour "red"
     * @see Ego::Math::Colour<RGB,float>::red()
     */
    static const MyType& red() {
        static const MyType colour(Colour<RGB,float>::red(), 1.0f);
        return colour;
    }

    /**
     * @brief The colour "green".
     * @return the colour "green"
     * @see Ego::Math::Colour<RGB,float>::green()
     */
    static const MyType& green() {
        static const MyType colour(Colour<RGB,float>::green(), 1.0f);
        return colour;
    }

    /**
     * @brief The colour "blue".
     * @return the colour "blue"
     * @see Ego::Math::Colour<RGB,float>::blue()
     */
    static const MyType blue() {
        static const MyType colour(Colour<RGB,float>::blue(), 1.0f);
        return colour;
    }

    /**
     * @brief The colour "white".
     * @return the colour "white"
     * @see Ego::Math::Colour<RGB,float>::white()
     */
    static const MyType white() {
        static const MyType colour(Colour<RGB,float>::white(), 1.0f);
        return colour;
    }

    /**
     * @brief The colour "black".
     * @return the colour "black"
     * @see Ego::Math::Colour<RGB,float>::black()
     */
    static const MyType black() {
        static const MyType colour(Colour<RGB,float>::black(), 1.0f);
        return colour;
    }

    /**
     * @brief The colour "cyan".
     * @return the colour "cyan"
     * @see Ego::Math::Colour<RGB,float>::cyan()
     */
    static const MyType cyan() {
        static const MyType colour(Colour<RGB,float>::cyan(), 1.0f);
        return colour;
    }

    /**
     * @brief The colour "magenta".
     * @return the colour "magenta"
     * @see Ego::Math::Colour<RGB,float>::magenta()
     */
    static const MyType magenta() {
        static const MyType colour(Colour<RGB,float>::magenta(), 1.0f);
        return colour;
    }

    /**
     * @brief The colour "yellow".
     * @return the colour "yellow"
     * @see Ego::Math::Colour<RGB,float>::yellow()
     */
    static const MyType yellow() {
        static const MyType colour(Colour<RGB,float>::yellow(), 1.0f);
        return colour;
    }

    /**
     * @brief The colour "purple".
     * @return the colour "purple"
     * @see Ego::Math::Colour<RGB,float>::purple()
     */
    static const MyType purple() {
        static const MyType colour(Colour<RGB,float>::purple(), 1.0f);
        return colour;
    }

    /**
     * @brief The colour "grey".
     * @return the colour "grey"
     * @see Ego::Math::Colour<RGB,float>::grey()
     */
    static const MyType grey() {
        static const MyType colour(Colour<RGB,float>::grey(), 1.0f);
        return colour;
    }

public:
    /**
     * @brief
     *  Create a colour default constructor (opaque black)
     */
    Colour() :
        ColourComponents<ColourSpaceType, ComponentType>(ColourComponents<ColourSpaceType, ComponentType>::min(), ColourComponents<ColourSpaceType, ComponentType>::min(), ColourComponents<ColourSpaceType, ComponentType>::min(), ColourComponents<ColourSpaceType, ComponentType>::max()) {
        // Intentionally empty
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
    Colour(const Colour<RGB,float>& other, float a) :
        ColourComponents<RGBA,float>(other, a) {
    }

    /**
     * @brief
     *  Create a colour.
     * @param other
     *  the other colour
     */
    Colour(const Colour& other) :
        ColourComponents<ColourSpaceType, ComponentType>(other) {
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
    Colour(ComponentType r, ComponentType g, ComponentType b, ComponentType a) :
        ColourComponents<ColourSpaceType, ComponentType>(r, g, b, a) {
        // Intentionally empty.
    }

    const MyType& operator=(const MyType& other) {
        assign(other);
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
        return MyType(1.0f - getRed(), 1.0f - getGreen(), 1.0f - getBlue(), getAlpha());
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
        return MyType(std::min(1.0f, getRed()*brightness), std::min(1.0f, getGreen()*brightness), std::min(1.0f, getBlue()*brightness), getAlpha());
    }

    /**
     * @brief
     *  Convert a colour value in RGBA colour space represented by
     *      four Byte values
     *  into the internal representation.
     * @param r
     *  the Byte value of the red component
     * @param g
     *  the Byte value of the green component
     * @param b
     *  the Byte value of the blue component
     * @param a
     *  the Byte value of the alpha component
     * @return
     *  the colour
     */
    static MyType parse(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return MyType(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f, ((float)a) / 255.0f);
    }
};

} // namespace Math
} // namespace Ego

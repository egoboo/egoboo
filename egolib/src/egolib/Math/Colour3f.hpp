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

/// @file   egolib/Math/Colour3.hpp
/// @brief  Colours in RGB colour space.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/ColourSpace.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  A colour value in RGB colour space represented by
 *       three single - precision floating point values
 *          each within the range of 0 (inclusive) to 1 (inclusive).
 */
template <>
struct Colour<RGB, float> : public ColourComponents<RGB, float> {
public:
    typedef float ComponentType;
    typedef RGB ColourSpaceType;
    typedef Colour<ColourSpaceType, ComponentType> MyType;

public:
    /**
     * @brief The colour "red" (255,0,0).
     * @return the colour "red"
     */
    static const MyType& red() {
        static const MyType colour = parse(0xff, 0x00, 0x00);
        return colour;
    }

    /**
     * @brief The colour "green" (0,255,0).
     * @return the colour "green"
     */
    static const MyType& green() {
        static const MyType colour = parse(0x00, 0xff, 0x00);
        return colour;
    }

    /**
     * @brief The colour "blue" (0,0,255).
     * @return the colour "blue"
     */
    static const MyType& blue() {
        static const MyType colour = parse(0x00, 0x00, 0xff);
        return colour;
    }

    /**
     * @brief The colour "white" (255,255,255).
     * @return the colour "white"
     */
    static const MyType& white() {
        static const MyType colour = parse(0xff, 0xff, 0xff);
        return colour;
    }

    /**
     * @brief The colour "black" (0,0,0).
     * @return the colour "black"
     */
    static const MyType& black() {
        static const MyType colour = parse(0x00, 0x00, 0x00);
        return colour;
    }

    /**
     * @brief The colour "cyan" (0,255,255).
     * @return the colour "cyan"
     * @remark The colour "cyan" is the complementary colour of the colour "red".
     */
    static const MyType& cyan() {
        static const MyType colour = parse(0x00, 0xff, 0xff);
        return colour;
    }

    /**
     * @brief The colour "magenta" (255,0,255).
     * @return the colour "magenta"
     * @remark The colour "magenta" is the complementary colour of the colour "green".
     */
    static const MyType& magenta() {
        static const MyType colour = parse(0xff, 0x00, 0xff);
        return colour;
    }

    /**
     * @brief The colour "yellow" (255,255,0).
     * @return the colour "yellow"
     * @remark The colour "yellow" is the complementary colour of the colour "blue".
     */
    static const MyType& yellow() {
        static const MyType colour = parse(0xff, 0xff, 0x00);
        return colour;
    }

    /**
     * @brief The colour "mauve" (224, 176, 255).
     * @return the colour "mauve".
     */
    static const MyType& mauve() {
        static const MyType colour = parse(0xe0, 0xb0, 0xff);
        return colour;
    }

    /**
     * @brief The colour "purple" (153, 0, 153).
     * @return the colour "purple".
     */
    static const MyType& purple() {
        static const MyType colour = parse(0x99, 0, 0x99);
        return colour;
    }

    /**
     * @brief The colour "grey" (75, 75, 75).
     * @return the colour "grey".
     */
    static const MyType& grey() {
        static const MyType colour = parse(75, 75, 75);
        return colour;
    }

public:
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
     *  Default constructor (opaque black)
     */
    Colour() :
        ColourComponents<ColourSpaceType, ComponentType>(0.0f, 0.0f, 0.0f) {
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
     *  if @a a, @a g or @a b a are not within the range of 0 (inclusive) and 1 (inclusive)
     */
    Colour(ComponentType r, ComponentType g, ComponentType b) :
        ColourComponents<ColourSpaceType, ComponentType>(r, g, b) {
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
     *  Given a colour  \f$(r,g,b)\f$ in real-valued, normalized RGB space,
     *  then corresponding inverted colour is \f$(1-r,1-g,1-b)\f$. Inverting
     *  a colour twice yields the same colour (modulo floating-point precision).
     * @remark
     *  The corresponding inverted colour is also known as the complementary colour.
     */
    MyType invert() const {
        return MyType(1.0f - getRed(), 1.0f - getGreen(), 1.0f - getBlue());
    }

    /**
     * @brief
     *  Convert a colour value in RGB colour space represented by
     *      three Byte values
     *  into the internal representation.
     * @param r
     *  the Byte value of the red component
     * @param g
     *  the Byte value of the green component
     * @param b
     *  the Byte value of the blue component
     * @return
     *  the colour
     */
    static MyType parse(uint8_t r, uint8_t g, uint8_t b) {
        return MyType(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
    }

}; // struct Colour

} // Math
} // Ego

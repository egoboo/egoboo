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

/// @file   egolib/Math/ColourRGB.hpp
/// @brief  Colours in RGB colour spaces.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Colour.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  A colour value in RGB colour space represented by
 *       three single - precision floating point values
 *          each within the range of 0 (inclusive) to 1 (inclusive).
 */
template <typename _ColourSpaceType>
struct Colour<_ColourSpaceType, std::enable_if_t<Internal::IsRgb<_ColourSpaceType>::value>> :
    public ColourComponents<_ColourSpaceType>,
    public Id::EqualToExpr<Colour<_ColourSpaceType>> {
public:
    using ColourSpaceType = _ColourSpaceType;
    using ComponentType = typename ColourSpaceType::ComponentType;
    using MyType = Colour<ColourSpaceType>;

public:
    /**
     * @brief The colour "red" (255,0,0).
     * @return the colour "red"
     */
    static const MyType& red() {
        static const MyType colour(Colour<RGBb>(255, 0, 0));
        return colour;
    }

    /**
     * @brief The colour "green" (0,255,0).
     * @return the colour "green"
     */
    static const MyType& green() {
        static const MyType colour(Colour<RGBb>(0, 255, 0));
        return colour;
    }

    /**
     * @brief The colour "blue" (0,0,255).
     * @return the colour "blue"
     */
    static const MyType& blue() {
        static const MyType colour(Colour<RGBb>(0, 0, 255));
        return colour;
    }

    /**
     * @brief The colour "white" (255,255,255).
     * @return the colour "white"
     */
    static const MyType& white() {
        static const MyType colour(Colour<RGBb>(255, 255, 255));
        return colour;
    }

    /**
     * @brief The colour "black" (0,0,0).
     * @return the colour "black"
     */
    static const MyType& black() {
        static const MyType colour(Colour<RGBb>(0, 0, 0));
        return colour;
    }

    /**
     * @brief The colour "cyan" (0,255,255).
     * @return the colour "cyan"
     * @remark The colour "cyan" is the complementary colour of the colour "red".
     */
    static const MyType& cyan() {
        static const MyType colour(Colour<RGBb>(0, 255, 255));
        return colour;
    }

    /**
     * @brief The colour "magenta" (255,0,255).
     * @return the colour "magenta"
     * @remark The colour "magenta" is the complementary colour of the colour "green".
     */
    static const MyType& magenta() {
        static const MyType colour(Colour<RGBb>(255, 0, 255));
        return colour;
    }

    /**
     * @brief The colour "yellow" (255,255,0).
     * @return the colour "yellow"
     * @remark The colour "yellow" is the complementary colour of the colour "blue".
     */
    static const MyType& yellow() {
        static const MyType colour(Colour<RGBb>(255, 255, 0));
        return colour;
    }

    /**
     * @brief The colour "mauve" (224, 176, 255) [Maerz and Paul].
     * @return the colour "mauve".
     */
    static const MyType& mauve() {
        static const MyType colour(Colour<RGBb>(224, 176, 255));
        return colour;
    }

    /**
     * @brief The colour "purple" (128, 0, 128).
     * @return the colour "purple".
     */
    static const MyType& purple() {
        static const MyType colour(Colour<RGBb>(128, 0, 128));
        return colour;
    }

    /**
     * @brief The colour "grey" (75, 75, 75).
     * @return the colour "grey".
     */
    static const MyType& grey() {
        static const MyType colour(Colour<RGBb>(75, 75, 75));
        return colour;
    }

public:
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
     *  Default construct with component values corresponding to "opaque black".
     */
    Colour() :
        ColourComponents<ColourSpaceType>(ColourSpaceType::min(),
                                          ColourSpaceType::min(),
                                          ColourSpaceType::min()) {
        // Intentionally empty.
    }

    /**
     * @brief
     *  Construct this colour from the specified component values
     * @param r
     *  the component value of the red component
     * @param g
     *  the component value of the green component
     * @param b
     *  the component value of the blue component
     * @throws Id::OutOfBoundsException
     *  if @a r, @a g, or @a b a are not within the range of ColourSpaceType::min() (inclusive) and ColourSpaceType::max() (inclusive)
     */
    Colour(ComponentType r, ComponentType g, ComponentType b) :
        ColourComponents<ColourSpaceType>(r, g, b) {
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

public:
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
        return MyType(ColourSpaceType::max() - this->getRed(),
                      ColourSpaceType::max() - this->getGreen(),
                      ColourSpaceType::max() - this->getBlue());
    }

public:
	// CRTP
    bool equalTo(const MyType& other) const {
        return this->getRed() == other.getRed()
            && this->getGreen() == other.getGreen()
            && this->getBlue() == other.getBlue();
    }

}; // struct Colour

} // Math
} // Ego

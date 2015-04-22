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

/// @file   egolib/Math/Colour3.h
/// @brief  Colours in RGB colour space.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego
{
	namespace Math
    {

		/**
		 * @brief
		 *	A colour value in RGB colour space represented by
         *      three single-precision floating point values
         *          each within the range of 0 (inclusive) to 1 (inclusive).
		 * @author
		 *	Michael Heilmann
		 */
		class Colour3f
		{
		public:
			/**
			 * @brief The colour "red" (255,0,0).
             * @return the colour "red"
			 */
            static const Colour3f& red()
            {
                static const Colour3f colour(1.0f, 0.0f, 0.0f);
                return colour;
            }

			/**
			 * @brief The colour "green" (0,255,0).
             * @return the colour "green"
			 */
            static const Colour3f& green()
            {
                static const Colour3f colour(0.0f, 1.0f, 0.0f);
                return colour;
            }

			/**
			 * @brief The colour "blue" (0,0,255).
             * @return the colour "blue"
			 */
            static const Colour3f& blue()
            {
                static const Colour3f colour(0.0f, 0.0f, 1.0f);
                return colour;
            }

			/**
			 * @brief The colour "white" (255,255,255).
             * @return the colour "white"
			 */
            static const Colour3f& white()
            {
                static const Colour3f colour(1.0f, 1.0f, 1.0f);
                return colour;
            }

			/**
			 * @brief The colour "black" (0,0,0).
             * @return the colour "black"
			 */
            static const Colour3f& black()
            {
                static const Colour3f colour(0.0f, 0.0f, 0.0f);
                return colour;
            }

            /**
             * @brief The colour "cyan" (0,255,255).
             * @return the colour "cyan"
             * @remark The colour "cyan" is the complementary colour of the colour "red".
             */
            static const Colour3f& cyan()
            {
                static const Colour3f colour(0.0f, 1.0f, 1.0f);
                return colour;
            }

            /**
             * @brief The colour "magenta" (255,0,255).
             * @return the colour "magenta"
             * @remark The colour "magenta" is the complementary colour of the colour "green". 
             */
            static const Colour3f& magenta()
            {
                static const Colour3f colour(1.0f, 0.0f, 1.0f);
                return colour;
            }

            /**
             * @brief The colour "yellow" (255,255,0).
             * @return the colour "yellow"
             * @remark The colour "yellow" is the complementary colour of the colour "blue".
             */
            static const Colour3f& yellow()
            {
                static const Colour3f colour(1.0f, 1.0f, 0.0f);
                return colour;
            }


		private:

			/**
			 * @brief
			 *	The red component.
			 * @invariant
			 *	0.0f <= r <= 1.0f
			 */
			float _r;

			/**
			 * @brief
			 *	The green component.
			 * @invariant
			 *	0.0f <= r <= 1.0f
			 */
			float _g;

			/**
			 * @brief
			 *	The blue component.
			 * @invariant
			 *	0.0f <= r <= 1.0f
			 */
			float _b;

		public:

			/**
			 * @brief
			 *	Get the value of the red component.
			 * @return
			 *	the value of the red component
			 */
			float getRed() const
			{
				return _r;
			}

			/**
			 * @brief
			 *	Get the value of the green component.
			 * @return
			 *	the value of the green component
			 */
			float getGreen() const
			{
				return _g;
			}

			/**
			 * @brief
			 *	Get the value of the blue component.
			 * @return
			 *	the value of the blue component
			 */
			float getBlue() const
			{
				return _b;
			}

			/**
			 * @brief
			 *	Create a colour.
			 * @param other
			 *	the other colour
			 */
			Colour3f(const Colour3f& other) : 
				_r(other._r),
				_g(other._g),
				_b(other._b)
			{
			}
			/**
			 * @brief
			 *	Default constructor (BLACK)
			 */
			Colour3f() :
				_r(0.0f),
				_g(0.0f),
				_b(0.0f)
			{
				//ctor				
			}

			/**
			 * @brief
			 *	Create a color.
			 * @param r
			 *	the component value of the red component
			 * @param g
			 *	the component value of the green component
			 * @param b
			 *	the component value of the blue component
			 * @throws std::domain_error
			 *	if @a a, @a g or @a b a are not within the range of 0 (inclusive) and 1 (inclusive)
			 */
			Colour3f(float r, float g, float b) :
				_r(r),
				_g(g),
				_b(b)
			{
				if (_r < 0.0f || _r > 1.0f)
				{
					throw std::domain_error("red component outside bounds");
				}
				if (_g < 0.0f || _g > 1.0f)
				{
					throw std::domain_error("green component outside bounds");
				}
				if (_b < 0.0f || _b > 1.0f)
				{
					throw std::domain_error("blue component outside bounds");
				}
			}

			/**
			 * @brief
			 *	Invert this colour value.
			 * @return
			 *	the inverted colour
			 * @remark
			 *	Given a colour  \f$(r,g,b)\f$ in real-valued, normalized RGB space,
			 *	then corresponding inverted colour is \f$(1-r,1-g,1-b)\f$. Inverting
			 *	a colour twice yields the same colour (modulo floating-point precision).
			 * @remark
			 *	The corresponding inverted colour is also known as the complementary colour.
			 */
			Colour3f invert() const
			{
				return Colour3f(1.0f - _r, 1.0f - _g, 1.0f - _b);
			}

            /**
             * @brief
             *	Convert a colour value in RGB colour space represented by
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
            static Colour3f parse(uint8_t r, uint8_t g, uint8_t b)
            {
                return Colour3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
            }

		};

	};
};
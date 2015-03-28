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

/// @file  egolib/Math/Colour4f.h
/// @brief Colours in RGBA colour space.

#pragma once

#include "egolib/Math/Colour3f.hpp"

namespace Ego
{
	namespace Math
    {

		/**
		 * @brief
		 *	A colour value in RGBA colour space represented by
         *      four single-precision floating point values
         *          each within the range of 0 (inclusive) to 1 (inclusive).
		 * @author
		 *	Michael Heilmann
		 */
		class Colour4f
		{
		public:
			/**
			 * @brief The colour "red".
             * @return the colour "red"
             * @see Ego::Math::Colour3f::red()
			 */
            static const Colour4f& red()
            {
                static const Colour4f colour(Colour3f::red(), 1.0f);
                return colour;
            }

			/**
			 * @brief The colour "green".
             * @return the colour "green"
             * @see Ego::Math::Colour3f::green()
			 */
            static const Colour4f& green()
            {
                static const Colour4f colour(Colour3f::green(), 1.0f);
                return colour;
            }

			/**
			 * @brief The colour "blue".
             * @return the colour "blue"
             * @see Ego::Math::Colour3f::blue()
			 */
            static const Colour4f blue()
            {
                static const Colour4f colour(Colour3f::blue(), 1.0f);
                return colour;
            }

			/**
			 * @brief The colour "white".
             * @return the colour "white"
             * @see Ego::Math::Colour3f::white()
			 */
            static const Colour4f white()
            {
                static const Colour4f colour(Colour3f::white(), 1.0f);
                return colour;
            }

			/**
			 * @brief The colour "black".
             * @return the colour "black"
             * @see Ego::Math::Colour3f::black()
			 */
            static const Colour4f black()
            {
                static const Colour4f colour(Colour3f::black(), 1.0f);
                return colour;
            }

            /**
             * @brief The colour "cyan".
             * @return the colour "cyan"
             * @see Ego::Math::Colour3f::cyan()
             */
            static const Colour4f cyan()
            {
                static const Colour4f colour(Colour3f::cyan(), 1.0f);
                return colour;
            }

			/**
			 * @brief The colour "magenta".
             * @return the colour "magenta"
             * @see Ego::Math::Colour3f::magenta()
			 */
            static const Colour4f magenta()
            {
                static const Colour4f colour(Colour3f::magenta(), 1.0f);
                return colour;
            }

            /**
             * @brief The colour "yellow".
             * @return the colour "yellow"
             * @see Ego::Math::Colour3f::yellow()
             */
            static const Colour4f yellow()
            {
                static const Colour4f colour(Colour3f::yellow(), 1.0f);
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

			/**
			 * @brief
			 *	The alpha component.
			 * @invariant
			 *	0.0f <= r <= 1.0f
			 * @remark
			 *	0.0f is completely transparent, 1.0f is completely opaque.
			 */
			float _a;
		
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
			 *	Get the value of the blue component.
			 * @return
			 *	the value of the blue component
			 */
			float getAlpha() const
            {
				return _a;
			}

			/**
			 * @brief
			 *	Create a colour default constructor (BLACK)
			 */
			Colour4f() :
				_r(0.0f),
				_g(0.0f),
				_b(0.0f),
				_a(0.0f)
			{
				//ctor
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
            Colour4f(const Colour3f& other,float a) :
                _r(other.getRed()),
                _g(other.getGreen()),
                _b(other.getBlue()),
                _a(a)
            {
                if (_a < 0.0f || _a > 1.0f)
                {
                    throw std::domain_error("alpha component outside bounds");
                }
            }

			/**
			 * @brief
			 *	Create a colour.
			 * @param other
			 *	the other colour
			 */
			Colour4f(const Colour4f& other) :
				_r(other._r),
				_g(other._g),
				_b(other._b),
				_a(other._a)
			{
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
			 * @param a
			 *	the component value of the alpha component
			 * @throws std::domain_error
			 *	if @a a, @a g, @a b or @a a are not within the range of 0 (inclusive) and 1 (inclusive)
			 */
			Colour4f(float r, float g, float b, float a) :
				_r(r),
				_g(g),
				_b(b),
				_a(a)
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
				if (_a < 0.0f || _a > 1.0f)
				{
					throw std::domain_error("alpha component outside bounds");
				}
			}

			/**
			 * @brief
			 *	Invert this colour value.
			 * @return
			 *	the inverted colour
			 * @remark
			 *	Given a colour  \f$(r,g,b,a)\f$ in real-valued, normalized RGBA space,
			 *	then corresponding inverted colour is \f$(1-r,1-g,1-b,1-a)\f$. Inverting
			 *	a colour twice yields the same colour (modula floating-point precision).
			 * @remark
			 *	The corresponding inverted colour is also known as the complementary colour.
			 */
			Colour4f invert() const
			{
				return Colour4f(1.0f - _r, 1.0f - _g, 1.0f - _b, 1.0 - _a);
			}

            /**
             * @brief
             *	Convert a colour value in RGBA colour space represented by
             *      four Byte values
             *  into the internal representation.
             * @param r
             *  the Byte value of the red component
             * @param g
             *  the Byte value of the green component
             * @param b
             *  the Byte value of the blue component
             * @param a
             *   the Byte value of the alpha component
             * @return
             *  the colour
             */
            static Colour4f parse(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
            {
                return Colour4f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f, ((float)a) / 255.0f);
            }

		};
	};
};
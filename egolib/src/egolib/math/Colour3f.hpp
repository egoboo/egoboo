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

/// @file  egolib/math/Colour3.h
/// @brief Colours in real-valued, normalized RGB space.

#pragma once

#include "egolib/platform.h"

namespace Ego {
	namespace Math {

		/**
		 * @brief
		 *	A colour in real-valued, normalized RGB space
		 * @author
		 *	Michael Heilmann
		 */
		class Colour3f
		{
		public:
			/**
			 * @brief The colour "red" (255,0,0).
			 */
			static const Colour3f RED;

			/**
			 * @brief The colour "green" (0,255,0).
			 */
			static const Colour3f GREEN;

			/**
			 * @brief The colour "blue" (0,0,255).
			 */
			static const Colour3f BLUE;

			/**
			 * @brief The colour "white" (255,255,255).
			 */
			static const Colour3f WHITE;

			/**
			 * @brief The colour "black" (0,0,0).
			 */
			static const Colour3f BLACK;

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
			 *	a colour twice yields the same colour (modula floating-point precision).
			 * @remark
			 *	The corresponding inverted colour is also known as the complementary colour.
			 */
			Colour3f invert() const
			{
				return Colour3f(1.0f - _r, 1.0f - _g, 1.0f - _b);
			}
		};

	};
};
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

/// @file   egolib/Graphics/ColorDepth.hpp
/// @brief  Information on the depths of a colors and color components
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {

/**
 * @brief The "depth" of a color is the number of bits used to represent a color value.
 * The number of bits used to represent the red, green, blue or alpha component is called
 * the "red depth", "green depth", "blue depth" and "alpha depth" of the color.
 */
struct ColorDepth {
private:
	/// @brief The color depth.
	uint16_t depth;

	/// @brief The red depth.
	uint8_t redDepth;

	/// @brief The green depth.
	uint8_t greenDepth;

	/// @brief The blue depth.
	uint8_t blueDepth;
	
	/// @brief The alpha depth.
	uint8_t alphaDepth;

public:
	/** 
	 * @brief Construct this color depth.
	 * @param depth the color depth
	 * @param redDepth, greenDepth, blueDepth, alphaDepth the red, green, blue and alpha depth
	 */
	ColorDepth(uint16_t depth, uint8_t redDepth, uint8_t greenDepth, uint8_t blueDepth, uint8_t alphaDepth);

	/** 
	 * @brief Construct this color depth.
	 * @param other the other color depth
	 */
	ColorDepth(const ColorDepth& other);

	/**
	 * @brief Assign this color depth.
	 * @param other the other color depth
	 * @return this color depth
	 */
	const ColorDepth& operator=(const ColorDepth& other);

public:
	/**
	 * @brief Get if this color depth is equal to another color depth.
	 * @param other the other color depth
	 * @return @a true if this color depth is equal to the other color depth, @a false otherwise
	 */
	bool operator==(const ColorDepth& other) const;

	/**
	 * @brief Get if this color depth is not equal to another color depth.
	 * @param other the other color depth
	 * @return @a true if this color depth is not equal to the other color depth, @a false otherwise
	 */
	bool operator!=(const ColorDepth& other) const;
	
public:
	/**
	 * @brief Get the depth.
	 * @return the depth
	 */
	uint16_t getDepth() const;

	/**
	 * @brief Get the red depth.
	 * @return the red depth
	 */
	uint8_t getRedDepth() const;

	/**
	 * @brief Get the green depth.
	 * @return the green depth
	 */
	uint8_t getGreenDepth() const;

	/** 
	 * @brief Get the blue depth.
	 * @return the blue depth
	 */
	uint8_t getBlueDepth() const;

	/** 
	 * @brief Get the alpha depth.
	 * @return the alpha depth
	 */
	uint8_t getAlphaDepth() const;

}; // struct ColorDepth

} // namespace Ego

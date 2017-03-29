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

/// @file   egolib/Graphics/ColourDepth.hpp
/// @brief  Information on the depths of a colours and colour components
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {

/**
 * @brief The "depth" of a colour is the number of bits used to represent a colour value.
 * The number of bits used to represent the red, green, blue or alpha component is called
 * the "red depth", "green depth", "blue depth" and "alpha depth" of the colour.
 */
struct ColourDepth : public id::equal_to_expr<ColourDepth> {
private:
	/// @brief The colour depth.
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
	 * @brief Construct this colour depth.
	 * @param depth the colour depth
	 * @param redDepth, greenDepth, blueDepth, alphaDepth the red, green, blue and alpha depth
	 */
    ColourDepth(uint16_t depth, uint8_t redDepth, uint8_t greenDepth, uint8_t blueDepth, uint8_t alphaDepth);

	/** 
	 * @brief Construct this colour depth.
	 * @param other the other colour depth
	 */
    ColourDepth(const ColourDepth& other);

	/**
	 * @brief Assign this colour depth.
	 * @param other the other colour depth
	 * @return this colour depth
	 */
	const ColourDepth& operator=(const ColourDepth& other);

public:
	// CRTP
    bool equal_to(const ColourDepth& other) const EGO_NOEXCEPT;

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

}; // struct ColourDepth

} // namespace Ego

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

#include "egolib/Graphics/ColorDepth.hpp"

namespace Ego {

ColorDepth::ColorDepth(uint16_t depth, uint8_t redDepth, uint8_t greenDepth, uint8_t blueDepth, uint8_t alphaDepth)
	: depth(depth),
	  redDepth(redDepth), greenDepth(greenDepth), blueDepth(blueDepth), alphaDepth(alphaDepth) {
}

ColorDepth::ColorDepth(const ColorDepth& other)
	: depth(other.getDepth()), 
	  redDepth(other.getRedDepth()), greenDepth(other.getGreenDepth()), blueDepth(other.getBlueDepth()), alphaDepth(other.getAlphaDepth()) {
}

const ColorDepth& ColorDepth::operator=(const ColorDepth& other) {
	depth = other.depth;
	redDepth = other.redDepth;
	greenDepth = other.greenDepth;
	blueDepth = other.blueDepth;
	alphaDepth = other.alphaDepth;
	return *this;
}

bool ColorDepth::operator==(const ColorDepth& other) const {
	return depth == other.depth
		&& redDepth == other.redDepth
		&& greenDepth == other.greenDepth
		&& blueDepth == other.blueDepth
		&& alphaDepth == other.alphaDepth;
}

bool ColorDepth::operator!=(const ColorDepth& other) const {
	return !((*this) == other);
}

uint16_t ColorDepth::getDepth() const {
	return depth;
}

uint8_t ColorDepth::getRedDepth() const {
	return redDepth;
}

uint8_t ColorDepth::getGreenDepth() const {
	return greenDepth;
}

uint8_t ColorDepth::getBlueDepth() const {
	return blueDepth;
}

uint8_t ColorDepth::getAlphaDepth() const {
	return alphaDepth;
}

} // namespace Ego

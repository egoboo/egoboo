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

/// @file  egolib/math/Colour3f.cpp
/// @brief Colours in real-valued, normalized RGB space.

#include "egolib/math/Colour3f.hpp"

namespace Ego {
	namespace Math {
		const Colour3f Colour3f::RED = Colour3f(1.0f, 0.0f, 0.0f);
		const Colour3f Colour3f::GREEN = Colour3f(0.0f, 1.0f, 0.0f);
		const Colour3f Colour3f::BLUE = Colour3f(0.0f, 0.0f, 1.0f);
		const Colour3f Colour3f::WHITE = Colour3f(1.0f, 1.0f, 1.0f);
		const Colour3f Colour3f::BLACK = Colour3f(0.0f, 0.0f, 0.0f);
	};
};
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

/// @file  egolib/Math/Colour3f.cpp
/// @brief Colours in real-valued, normalized RGB space.

#include "egolib/Math/Colour3f.hpp"

namespace Ego
{
	namespace Math
    {
		const Colour3f Colour3f::RED(1.0f, 0.0f, 0.0f);
		const Colour3f Colour3f::GREEN(0.0f, 1.0f, 0.0f);
		const Colour3f Colour3f::BLUE(0.0f, 0.0f, 1.0f);
		const Colour3f Colour3f::WHITE(1.0f, 1.0f, 1.0f);
		const Colour3f Colour3f::BLACK(0.0f, 0.0f, 0.0f);
        const Colour3f Colour3f::CYAN(0.0f,1.0f,1.0f);
        const Colour3f Colour3f::MAGENTA(1.0f, 0.0f, 1.0f);
        const Colour3f Colour3f::YELLOW(1.0f, 1.0f, 0.0f);
	};
};
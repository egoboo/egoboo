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

/// @file  egolib/Math/Colour4f.cpp
/// @brief Colours in real-valued, normalized RGBA space.

#include "egolib/Math/Colour4f.hpp"

namespace Ego
{
	namespace Math
    {
		const Colour4f Colour4f::RED(1.0f, 0.0f, 0.0f, 1.0f);
		const Colour4f Colour4f::GREEN(0.0f, 1.0f, 0.0f, 1.0f);
		const Colour4f Colour4f::BLUE(0.0f, 0.0f, 1.0f, 1.0f);
		const Colour4f Colour4f::WHITE(1.0f, 1.0f, 1.0f, 1.0f);
		const Colour4f Colour4f::BLACK(0.0f, 0.0f, 0.0f, 1.0f);
        const Colour4f Colour4f::CYAN(0.0f, 1.0f, 1.0f, 1.0f);
		const Colour4f Colour4f::MAGENTA(1.0f, 0.0f, 1.0f, 1.0f);
        const Colour4f Colour4f::YELLOW(1.0f, 1.0f, 0.0f, 1.0f);
	};
};
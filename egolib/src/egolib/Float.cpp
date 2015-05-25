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
#include "egolib/Float.hpp"

Uint32 float_toBits(float x)
{
	union { Uint32 i; float f; } val;

	val.f = x;

	return val.i;
}

float float_fromBits(Uint32 x)
{
	union { Uint32 i; float f; } val;

	val.i = x;

	return val.f;

}

bool float_infinite(float x)
{
	Uint32 u = float_toBits(x);

	return (0 == (u & IEEE32_FRACTION)) && (IEEE32_EXPONENT == (u & IEEE32_EXPONENT));
}

bool float_nan(float x)
{
	Uint32 u = float_toBits(x);

	return (0 != (u&IEEE32_FRACTION)) && (IEEE32_EXPONENT == (u & IEEE32_EXPONENT));
}

bool float_bad(float x)
{
	Uint32 u = float_toBits(x);

	return (IEEE32_EXPONENT == (u & IEEE32_EXPONENT)) ? true : false;
}

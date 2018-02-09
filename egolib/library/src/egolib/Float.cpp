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

uint32_t single_to_bits(float x)
{
	union { uint32_t i; float f; } u;
	u.f = x;
	return u.i;
}

float single_from_bits(uint32_t x)
{
	union { uint32_t i; float f; } u;
	u.i = x;
	return u.f;
}

uint64_t double_to_bits(double x)
{
	union { uint32_t i; double f; } u;
	u.f = x;
	return u.i;
}


double double_from_bits(uint64_t x)
{
	union { uint64_t i; float f; } u;
	u.i = x;
	return u.f;
}

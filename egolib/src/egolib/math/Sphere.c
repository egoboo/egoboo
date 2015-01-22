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

/// @file  egolib/math/Sphere.c
/// @brief Spheres.

#include "egolib/math/Sphere.h"

bool sphere_self_clear(sphere_t& self)
{
	fvec3_self_clear(self.origin.v);
	self.radius = 0.0f;
	return true;
}

float sphere_get_radius(const sphere_t& self)
{
	return self.radius;
}

bool sphere_self_is_clear(const sphere_t& self)
{
	return 0.0f == self.radius
		&& fvec3_self_is_clear(self.origin.v);
}
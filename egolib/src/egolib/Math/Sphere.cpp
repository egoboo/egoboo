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

/// @file  egolib/Math/Sphere.c
/// @brief Spheres.

#include "egolib/Math/Sphere.h"

const fvec3_t& sphere_t::getCenter() const
{
	return origin;
}

float sphere_t::getRadius() const
{
	return radius;
}

void sphere_t::assign(const sphere_t& other)
{
	radius = other.radius;
	origin = other.origin;
}

sphere_t& sphere_t::operator=(const sphere_t& other)
{
	assign(other);
	return *this;
}

sphere_t::sphere_t() :
	origin(fvec3_t::zero()),
	radius(0.0f)
{
}

sphere_t::sphere_t(const sphere_t& other) :
	origin(other.origin),
	radius(other.radius)
{
}


bool sphere_t::intersects(const fvec3_t& other) const
{
	// Get the squared distance between the point and the center of the sphere.
	float distance_2 = (origin - other).length_2();
	// Get the squared radius of the sphere.
	float radius_2 = radius * radius;
	// If the squared distance beween the point and the center of the sphere
	// is smaller than or equal to the squared radius of the sphere ...
	if (distance_2 <= radius_2)
	{
		// ... the sphere and the point intersect.
		return true;
	}
	// Otherwise they don't intersect.
	return false;
}

bool sphere_t::intersects(const sphere_t& other) const
{
	// Get the squared distance between the centers of the two spheres.
	float distance_2 = (origin - other.origin).length_2();
	// Get the squared sum of the radiis of the two spheres.
	float sumOfRadii = radius + other.radius;
	float sumOfRadii_2 = sumOfRadii * sumOfRadii;
	// If the squared distance beween the centers of the spheres
	// is smaller than or equal to the squared sum of the radii of the spheres ...
	if (distance_2 <= sumOfRadii_2)
	{
		// ... the spheres intersect.
		return true;
	}
	// Otherwise they don't intersect.
	return false;
}

bool sphere_self_clear(sphere_t& self)
{
	self.origin = fvec3_t::zero();
	self.radius = 0.0f;
	return true;
}

bool sphere_self_is_clear(const sphere_t& self)
{
	return 0.0f == self.radius
		&& fvec3_t::zero() == self.origin;
}
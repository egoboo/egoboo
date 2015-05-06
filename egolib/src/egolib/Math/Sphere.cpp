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

sphere_t::sphere_t() :
    _center(fvec3_t::zero()),
    _radius(0.0f)
{}

sphere_t::sphere_t(const fvec3_t& center, float radius) :
    _center(center),
    _radius(radius)
{
    if (_radius < 0.0f)
    {
        throw std::domain_error("sphere radius is negative");
    }
}

sphere_t::sphere_t(const sphere_t& other) :
    _center(other._center),
    _radius(other._radius)
{}

const fvec3_t& sphere_t::getCenter() const
{
    return _center;
}

void sphere_t::setCenter(const fvec3_t& center)
{
    _center = center;
}

float sphere_t::getRadius() const
{
    return _radius;
}

void sphere_t::setRadius(float radius)
{
    if (radius < 0.0f)
    {
        throw std::domain_error("sphere radius is negative");
    }
    _radius = radius;
}

void sphere_t::assign(const sphere_t& other)
{
    _radius = other._radius;
    _center = other._center;
}

sphere_t& sphere_t::operator=(const sphere_t& other)
{
    assign(other);
    return *this;
}

bool sphere_t::intersects(const fvec3_t& other) const
{
    // Get the squared distance between the point and the center of the sphere.
    float distance_2 = (_center - other).length_2();
    // Get the squared radius of the sphere.
    float radius_2 = _radius * _radius;
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
    float distance_2 = (_center - other._center).length_2();
    // Get the squared sum of the radiis of the two spheres.
    float sumOfRadii = _radius + other._radius;
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

#if 0
bool sphere_self_clear(sphere_t& self)
{
    self = sphere_t();
    return true;
}

bool sphere_self_is_clear(const sphere_t& self)
{
    return 0.0f == self._radius
        && fvec3_t::zero() == self._origin;
}
#endif
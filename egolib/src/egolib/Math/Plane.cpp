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

/// @file  egolib/Math/Plane.cpp
/// @brief Planes.

#include "egolib/Math/Plane.hpp"

plane_t::plane_t() :
	_n(0.0f, 0.0f, 1.0f),
	_d(0.0f)
{
	//ctor
}

plane_t::plane_t(const fvec3_t& a, const fvec3_t& b, const fvec3_t& c) : plane_t()
{
	fvec3_t u = b - a;
	if (u == fvec3_t::zero)
	{
		throw std::domain_error("b = a");
	}
	fvec3_t v = c - a;
	if (u == fvec3_t::zero)
	{
		throw std::domain_error("c = a");
	}
	_n = u.cross(v);
	if (0.0f == _n.normalize())
	{
		/* u x v = 0 is only possible for u,v != 0 if u = v and thus b = c. */
		throw std::domain_error("b = c");
	}
	_d = -_n.dot(a);
}

plane_t::plane_t(const fvec3_t& p, const fvec3_t& n) :
	_n(n), _d(0.0f)
{
	if (_n.normalize() == 0.0f)
	{
		throw std::domain_error("normal vector is zero vector");
	}
	_d = -_n.dot(p);
}

plane_t::plane_t(const fvec3_t& t, const float d) :
    _n(t), _d(d)
{
    if (_n.normalize() == 0.0f)
    {
        throw std::domain_error("axis vector is zero vector");
    }
}

plane_t::plane_t(const plane_t& other) :
	_n(other._n),
	_d(other._d)
{
}

float plane_t::distance(const fvec3_t& point) const
{
	return _n.dot(point) + _d;
}

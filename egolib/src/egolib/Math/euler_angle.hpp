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

/// @file idlib/math/euler_angle.hpp
/// @brief Euler angles.
/// @author Michael Heilmann

#include "egolib/integrations/idlib.hpp"

#pragma once

namespace idlib {

/// @brief An Euler angle.
/// @tparam Angle a specialization of idlib::angle
template <typename Angle>
struct euler_angle
{
	/// @brief The angle type.
	using angle_type = Angle;

    euler_angle() :
		m_x(), m_y(), m_z()
	{}
    
	euler_angle(const angle_type& x, const angle_type& y, const angle_type& z) :
		m_x(x), m_y(y), m_z(z)
	{}
    
	const angle_type& operator[](size_t index) const
	{
        switch (index)
		{
            case 0:
                return m_x;
            case 1:
                return m_y;
            case 2:
                return m_z;
            default:
                throw idlib::out_of_bounds_error(__FILE__, __LINE__, "index out of range");
        };
    }
	
	angle_type& operator[](size_t index)
	{
        switch (index)
		{
            case 0:
                return m_x;
            case 1:
                return m_y;
            case 2:
                return m_z;
            default:
                throw idlib::out_of_bounds_error(__FILE__, __LINE__, "index out of range");
        };
    }

private:
    angle_type m_x, m_y, m_z;

}; // struct euler_angle

} // namespace idlib

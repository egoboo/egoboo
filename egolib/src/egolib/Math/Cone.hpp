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

/// @file  egolib/Math/Cone.hpp
/// @brief Cones.

#include "egolib/Math/Vector.hpp"

/**
 * @brief
 *  A cone given by its origin, an axis and its opening angle.
 */
struct cone_t
{
    fvec3_t origin;
    fvec3_t axis;

    // use these values to pre-calculate trig functions based off of the opening angle
    float   inv_sin;
    float   sin_2;
    float   cos_2;

    cone_t() :
        origin(0, 0, 0),
        axis(0, 0, 0),
        inv_sin(0.0f),
        sin_2(0.0f),
        cos_2(0.0f)
    {
        //ctor
    }

    /**
     * @brief
     *	Assign this cone the values of another cone.
     * @param other
     *	the other cone
     * @post
     *	This cone was assigned the values of the other cone.
     */
    void assign(const cone_t& other)
    {
        origin = other.origin;
        axis = other.axis;
        inv_sin = other.inv_sin;
        sin_2 = other.sin_2;
        cos_2 = other.cos_2;
    }

    /**
     * @brief
     *	Assign this cone the values of another cone.
     * @param other
     *	the other cone
     * @return
     *	this cone
     * @post
     *	This cone was assigned the values of the other cone.
     */
    cone_t& operator=(const cone_t& other)
    {
        assign(other);
        return *this;
    }

    /**
     * @brief
     *  Get the origin of this cone.
     * @return
     *  the origin of this cone
     */
    const fvec3_t& getOrigin() const
    {
        return origin;
    }

    /**
     * @brief
     *  Get the axis of this cone.
     * @return
     *  the axis of this cone
     */
    const fvec3_t& getAxis() const
    {
        return axis;
    }

    /**
     * @brief
     *  Translate this cone.
     * @param t
     *  the translation vector
     */
    void translate(const fvec3_t& t)
    {
        origin += t;
    }

};

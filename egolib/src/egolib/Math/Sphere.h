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

/// @file  egolib/Math/Sphere.h
/// @brief Spheres.

#pragma once

#include "egolib/Math/Vector.hpp"

/**
 * @brief
 *  A sphere.
 *  The terms the/a "sphere_t object" and the/a "sphere" are synonyms.
 */
struct sphere_t
{

private:

    /**
     * @brief
     *  The center of the sphere.
     */
    fvec3_t _center;

    /**
     * @brief
     *  The radius of the sphere.
     * @invariant
     *  Greater than or equal to @a 0.
     */
    float _radius;

public:

    /**
     * @brief
     *  Construct this sphere assigning it the default values of a sphere.
     * @post
     *  This sphere was assigned the default values of a sphere.
     * @remark
     *  The default values of a sphere are the center of @a (0,0,0) and the radius of @a 0.
     */
    sphere_t();

    /**
     * @brief
     *  Construct this sphere assigning it the specified values.
     * @param center
     *  the center of the sphere
     * @param radius
     *  the radius of the sphere
     * @throw std::domain_error
     *  if the radius is negative
     * @pre
     *  The radius is not negative
     * @post
     *  The sphere was assigned the specified values.
     */
    sphere_t(const fvec3_t& center, float radius);

    /**
     * @brief
     *  Construct this sphere assigning it the values of another sphere.
     * @post
     *  This sphere was assigned the default values of another sphere.
     */
    sphere_t(const sphere_t& other);

public:

    /**
     * @brief
     *  Get the center of this sphere.
     * @return
     *  the center of this sphere
     */
    const fvec3_t& getCenter() const;

    /**
     * @brief
     *  Set the center of this sphere.
     * @param center
     *  the center
     * @post
     *  The sphere was assigned the center
     */
    void setCenter(const fvec3_t& center);

    /**
     * @brief
     *  Get the radius of this  sphere.
     * @return
     *  the radius of this sphere
     */
    float getRadius() const;

    /**
     * @brief
     *  Set the radius of this sphere.
     * @param radius
     *  the radius
     * @pre
     *  The radius must be greater than or equal to @a 0.
     * @throw std::domain_error
     *  if the radius is smaller than @a 0
     * @post
     *  If an exception is raised, the sphere's radius was not modified.
     *  Otherwise, the sphere was assigned the radius.
     */
    void setRadius(float radius);

    /**
     * @brief
     *  Assign this sphere values of another sphere.
     * @param other
     *  the other sphere
     * @post
     *  This sphere was assigned the values of the sphere.
     */
    void assign(const sphere_t& other);

    /**
     * @brief
     *  Assign this sphere the values of another sphere.
     * @param other
     *  the other sphere
     * @return
     *  this sphere
     * @post
     *  This sphere was assigned the values of the other sphere.
     */
    sphere_t& operator=(const sphere_t& other);


    /**
     * @brief
     *  Get if this sphere intersects with a point.
     * @param other
     *  the point
     * @return
     *  @a true if this sphere intersects with the point,
     *  @a false otherwise
     * @remark
     *  A sphere \f$(c,r)\f$ with the center $c$ and the radius $r$
     *  and a point \f$p\f$ intersect if \f$|p - c| \leq r\f$ holds.
     *  That condition is equivalent to the condition \f$|p - c|^2
     *  \leq r^2\f$ but the latter is more efficient to test (two
     *  multiplications vs. one square root).
     */
    bool intersects(const fvec3_t& point) const;

    /**
     * @brief
     *  Get if this sphere intersects with another sphere.
     * @param other
     *  the other sphere
     * @return
     *  @a true if this sphere intersects with the other sphere,
     *  @a false otherwise
     * @remark
     *  Two spheres \f$(c_0,r_0)\f$ and \f$(c_1,r_1)\f$ with the
     *  centers \f$c_0\f$ and \f$c_1\f$ and the radii \f$r_0\f$
     *  and \f$r_1\f$ intersect if \f$|c_1 - c_0| \leq r_0 + r_1\f$
     *  holds. That condition is equivalent to the condition
     *  \f$|c_1 - c_0|^2 \leq (r_0 + r_1)^2\f$ but the latter
     *  is more efficient to test (two multiplications vs. one
     *  square root).
     */
    bool intersects(const sphere_t& other) const;

};

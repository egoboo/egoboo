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

/// @file egolib/bv.h
/// @brief Convex bounding volumes consiting of spheres enclosing boxes.

#pragma once

#include "egolib/Math/_Include.hpp"

/**
 * @brief
 *  A convex bounding volume consisting of a sphere enclosing a bounding box.
 * @todo
 *  Recompute sphere on-demand.
 */
struct bv_t
{
private:
    /**
     * @brief
     *  Update the sphere from the AABB.
     */
    void updateSphere() {
        _sphere = Ego::Math::convexHull<AABB3f,Sphere3f>(_aabb);
    }

    /**
     * @brief
     *  The sphere.
     */
    Sphere3f _sphere;

    /**
     * @brief
     *  The AABB.
     */
	AABB3f _aabb;
public:


    /**
     * @brief
     *  Get the sphere.
     * @return
     *  the sphere
     */
    const Sphere3f& getSphere() const {
        return _sphere;
    }

    /**
     * @brief
     *  Get the AABB.
     * @return
     *  the AABB
     */
    const AABB3f& getAABB() const {
        return _aabb;
    }

    /**
     * @brief
     *  Construct this bounding volume with its default values.
     * @remark
     *  The default values of a bounding volume are the default sohere and the default AABB.
     */
    bv_t()
      : _sphere(), _aabb() {
        /* Intentionaly empty. */
    }

    /**
     * @brief
     *  Construct this bounding volume with a given AABB.
     * @param aabb
     *  the aabb
     */
    bv_t(const AABB3f& aabb)
        : _sphere(Ego::Math::convexHull<AABB3f,Sphere3f>(aabb)), _aabb(aabb) {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Construct this bounding volume with the values of another bounding volume.
     * @param other
     *  the other bounding volume
     */
    bv_t(const bv_t& other)
        : _sphere(other._sphere), _aabb(other._aabb) {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Assign this convex bounding volume the values of another convex bounding volume.
     * @param other
     *  the other convex bounding volume
     * @post
     *  This convex bounding volume box was assigned the values of the other convex bounding box.
     */
    void assign(const bv_t& other) {
        _aabb = other._aabb;
        _sphere = other._sphere;
    }

    /**
     * @brief
     *  Assign this convex bounding volume the values of another convex bounding volume.
     * @param other
     *  the other convex bounding volume
     * @return
     *  this convex bounding volume
     * @post
     *  This convex bounding volume box was assigned the values of the other convex bounding box.
     */
    bv_t& operator=(const bv_t& other) {
        assign(other);
        return *this;
    }


    bool contains(const bv_t& other) const {
        return _aabb.contains(other._aabb);
    }

    bool overlaps(const bv_t& other) const {
        return _aabb.overlaps(other._aabb);
    }

    void join(const bv_t& other) {
        _aabb.join(other._aabb);
        updateSphere();
    }

};

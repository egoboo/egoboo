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

/// @file  egolib/Math/AABB.hpp
/// @brief Axis-aligned bounding boxes.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Vector.hpp"
#include "egolib/Math/Translatable.hpp"
#include "egolib/Math/EuclideanSpace.hpp"
#include "egolib/Math/Sphere.h"

namespace Ego {
namespace Math {

/**
 * @brief
 *  An axis-aligned bounding box ("AABB").
 * @param _ScalarType
 *  must fulfil the <em>scalar</em> concept
 * @param _Dimensionality
 *  must fulfil the <em>dimensionality</em> concept
 * @remark
 *  The terms "the/an axis-aligned bounding box (object)" and "the/an AABB (object)" are synonyms.
 * @author
 *  Michael Heilmann
 */
template <typename _VectorSpaceType>
struct AABB : public Translatable<_VectorSpaceType> {
public:
    /// @brief The Euclidean space over which the AABBs are defined.
    typedef EuclideanSpace<_VectorSpaceType> EuclideanSpaceType;
    /// The vector space type (of the Euclidean space).
    typedef typename EuclideanSpaceType::VectorSpaceType VectorSpaceType;
    /// The scalar field type (of the vector space).
    typedef typename EuclideanSpaceType::ScalarFieldType ScalarFieldType;
    /// The vector type (of the vector space).
    typedef typename EuclideanSpaceType::VectorType VectorType;
    /// The scalar type (of the scalar field).
    typedef typename EuclideanSpaceType::ScalarType ScalarType;
    /// @brief @a MyType is the type of this template/template specialization.
    typedef AABB<_VectorSpaceType> MyType;

private:
    /**
     * @brief
     *  The minimum along each axis.
     */
    VectorType _min;

    /**
     * @brief
     *  The maximum along each axis.
     */
    VectorType _max;

public:
    /**
     * @brief
     *  Construct this AABB with its default values.
     * @remark
     *  The default values of an AABB are the center of @a (0,0,0) and the size of @a 0 along all axes.
     */
    AABB()
        : _min(), _max() {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Construct this AABB with the given minima and maxima.
     * @param min
     *  the minimum along each axis
     * @param max
     *  the maximum along each axis
     * @throw std::logic_error
     *  if the minimum along an axis is greater than the maximum along that axis
     */
    AABB(const VectorType& min, const VectorType& max)
        : _min(min), _max(max) {
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality(); ++i) {
            if (min[i] > max[i]) {
                throw std::logic_error("minimum along an axis is greater than the maximum along that axis");
            }
        }
    }

    /**
    * @brief
    *   Copy constructor
    **/
    AABB(const AABB &other) : AABB(other._min, other._max) {
        /* Intentionally empty. */        
    }

    /**
     * @brief
     *  Get the minimum.
     * @return
     *  the minimum
     */
    const VectorType& getMin() const {
        return _min;
    }

    /**
     * @brief
     *  Get the maximum.
     * @return
     *  the maximum
     */
    const VectorType& getMax() const {
        return _max;
    }

    /**
     * @brief
     *  Get the center.
     * @return
     *  the center
     */
    VectorType getCenter() const {
        return (_min + _max) * 0.5f;
    }

	/**
	 * @brief
	 *	Get the size.
	 * @return
	 *	the size
	 */
	VectorType getSize() const {
		return _max - _min;
	}

    /**
     * @brief
     *  Assign this bounding box the values of another bounding box.
     * @param other
     *  the other bounding box
     * @post
     *  This bounding box was assigned the values of the other bounding box.
     */
    void assign(const MyType& other) {
        _min = other._min;
        _max = other._max;
    }

    /**
     * @brief
     *  Assign this AABB the join if itself with another AABB.
     * @param other
     *  the other AABB
     * @post
     *  The result of the join was assigned to this AABB.
     */
    void join(const MyType& other) {
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality(); ++i) {
            _min[i] = std::min(_min[i], other._min[i]);
            _max[i] = std::max(_max[i], other._max[i]);
        }
    }

    /**
     * @brief
     *  Get if this AABB contains another AABB.
     * @param other
     *  the other AABB
     * @return
     *  @a true if this AABB contains the other AABB,
     *  @a false otherwise
     * @remark
     *  This function is <em>not</em> commutative.
     */
    bool contains(const MyType& other) const {
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality(); ++i) {
            if (_max[i] > other._max[i]) return false;
            if (_min[i] < other._min[i]) return false;
        }
        return true;
    }

    /**
     * @brief
     *  Get if an AABB overlaps with another AABB.
     * @param other
     *  the other AABB
     * @return
     *  @a true if this AABB and the other AABB,
     *  @a false otherwise
     * @remark
     *  The function is commutative.
     * @remark
     *  Two AABBs x and y overlap if
     *  for each axis k x.min[k] >= y.min[k] and x
     */
    inline bool overlaps(const MyType& other) const {
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality(); ++i) {
            // If the minimum of the LHS is greater than the maximum of the RHS along one axis,
            // then they can not overlap.
            if (_min[i] > other._max[i]) {
                return false;
            }
            // If the maximum of the LHS is smaller than the minimum of the RHS along one axis,
            // then they can not overlap.
            if (_max[i] < other._min[i]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief
     *  Get if this AABB is degenerated.
     * @return
     *  @a true if this AABB is degenerated,
     *  @a false otherwise
     * @remark
     *  An AABB is called "degenerated along an axis" if the minimum of the AABB at that axis equals the maximum of the AABB
     *  at that axis. If an AABB is "degenerated" along all axes, then the AABB is called "degenerated".
     */
    bool isDegenerated() const {
        return _min == _max;
    }

public:

    /**
     * @brief
     *  Assign this bounding box the values of another bounding box.
     * @param other
     *  the other bounding box
     * @return
     *  this bounding box
     * @post
     *  This bounding box was assigned the values of the other bounding box.
     */
    MyType& operator=(const MyType& other) {
        assign(other);
        return *this;
    }

	/** @copydoc Ego::Math::translatable */
	void translate(const VectorType& t) override {
		_min += t;
		_max += t;
	}

};

} // namespace Math
} // namespace Ego

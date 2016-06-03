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

/// @file egolib/Math/AxisAlignedBox.hpp
/// @brief Axis aligned boxes.
/// @author Michael Heilmann

#pragma once


#include "egolib/Math/EuclideanSpace.hpp"
#include "egolib/Math/Sphere.hpp"


namespace Ego {
namespace Math {

/**
 * @brief
 *  An axis-aligned box ("AAB").
 * @param _ScalarType
 *  must fulfil the <em>scalar</em> concept
 * @param _Dimensionality
 *  must fulfil the <em>dimensionality</em> concept
 */
template <typename _EuclideanSpaceType>
struct AxisAlignedBox {
public:
    Ego_Math_EuclideanSpace_CommonDefinitions(AxisAlignedBox);

private:
    /**
     * @brief
     *  The minimum along each axis.
     */
    PointType _min;

    /**
     * @brief
     *  The maximum along each axis.
     */
    PointType _max;

public:
    /**
     * @brief
     *  Construct this axis aligned box with its default values.
     * @remark
     *  The default values of an axis aligned box are the center of @a (0,0,0) and the size of @a 0 along all axes.
     */
    AxisAlignedBox()
        : _min(), _max() {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Construct this AxisAlignedBox with the given minima and maxima.
     * @param min
     *  the minimum along each axis
     * @param max
     *  the maximum along each axis
     * @throw std::logic_error
     *  if the minimum along an axis is greater than the maximum along that axis
     */
    AxisAlignedBox(const PointType& min, const PointType& max)
        : _min(min), _max(max) {
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality(); ++i) {
            if (min[i] > max[i]) {
                throw std::logic_error("minimum along an axis is greater than the maximum along that axis");
            }
        }
    }

    /**
     * @brief
     *  Construct this axis aligned box with the values of another axis aligned box.
     * @param other
     *  the other axis aligned box
     * @post
     *  This axis aligned box was constructed with the values of the other axis aligned box.
     */
    AxisAlignedBox(const AxisAlignedBox &other) : AxisAlignedBox(other._min, other._max) {
        /* Intentionally empty. */        
    }

    /**
     * @brief
     *  Get the minimum.
     * @return
     *  the minimum
     */
    const PointType& getMin() const {
        return _min;
    }

    /**
     * @brief
     *  Get the maximum.
     * @return
     *  the maximum
     */
    const PointType& getMax() const {
        return _max;
    }

    /**
     * @brief
     *  Get the center.
     * @return
     *  the center
     */
    PointType getCenter() const {
        return _min + getSize() * 0.5f;
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
     *  Assign this axis aligned box the join if itself with another axis aligned box.
     * @param other
     *  the other axis aligned box
     * @post
     *  The result of the join was assigned to this axis aligned box.
     */
    void join(const MyType& other) {
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality(); ++i) {
            _min[i] = std::min(_min[i], other._min[i]);
            _max[i] = std::max(_max[i], other._max[i]);
        }
    }

    /**
     * @brief
     *  Get if this axis aligned box is degenerated.
     * @return
     *  @a true if this axis aligned is degenerated,
     *  @a false otherwise
     * @remark
     *  An axis aligned box is called "degenerated along an axis" if its
     *  minimum equals its maximum along that axis. If an axis aligned
     *  box is "degenerated" along all axes, then the AABB is called
     *  "degenerated".
     */
    bool isDegenerated() const {
        return _min == _max;
    }

public:

    /**
     * @brief
     *  Assign this axis aligned box the values of another axis aligned box.
     * @param other
     *  the other axis aligned box
     * @return
     *  this axis aligned box
     * @post
     *  This axis aligned box was assigned the values of the other axis aligned box.
     */
    MyType& operator=(const MyType& other) {
        assign(other);
        return *this;
    }

    bool operator==(const MyType& other) const {
        return _min == other._min
            && _max == other._max;
    }

    bool operator!=(const MyType& other) const {
        return _min != other._min
            || _max != other._max;
    }

}; // struct AxisAlignedBox

} // namespace Math
} // namespace Ego

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

/// @file egolib/Math/AxisAlignedCube.hpp
/// @brief Axis aligned cubes.
/// @author Michael Heilmann

#pragma once


#include "egolib/Math/EuclideanSpace.hpp"
#include "egolib/Math/Functors/Translate.hpp"


namespace Ego {
namespace Math {

template <typename _EuclideanSpaceType>
struct AxisAlignedCube {
public:
    Ego_Math_EuclideanSpace_CommonDefinitions(AxisAlignedCube);

private:

    /**
     * @brief
     *  The center of the cube.
     */
    PointType _center;
    
    /**
     * @brief
     *  The size of the cube.
     * @invariant
     *  Greater than or equal to @a 0.
     */
    ScalarType _size;

public:

    /**
     * @brief
     *  Construct this axis aligned cube with the default values of an axis aligned cube.
     * @post
     *  This axis aligned cube was constructed with the default values of an axis aligned cube.
     * @remark
     *  The default values of an axis aligned cube are the center of @a (0,0,0) and the size of @a 0.
     */
    AxisAlignedCube()
        : _center(), _size() {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Construct this axis aligned cube with the specified values.
     * @param center
     *  the center of the axis aligned cube
     * @param size
     *  the size of the axis aligned cube
     * @throw std::domain_error
     *  if the size is negative
     * @pre
     *  The size is not negative.
     * @post
     *  The cube was constructed with the specified values.
     */
    AxisAlignedCube(const PointType& center, const ScalarType& size)
        : _center(center), _size(size) {
        if (_size < 0) {
            throw std::domain_error("cube size is negative");
        }
    }

    /**
     * @brief
     *  Construct this axis aligned cube with the values of another axis aligned cube.
     * @param other
     *  the other axis aligned cube
     * @post
     *  This axis aligned cube was constructed with the values of the other axis aligned cube.
     */
    AxisAlignedCube(const MyType& other)
        : _center(other._center), _size(other._size) {
        /* Intentionally empty. */
    }

public:

    /**
     * @brief
     *  Get the center of this axis aligned cube.
     * @return
     *  the center of this axis aligned cube
     */
    const PointType& getCenter() const {
        return _center;
    }

    /**
     * @brief
     *  Get the size of this axis aligned cube.
     * @return
     *  the size of this axis aligned cube
     */
    const ScalarType& getSize() const {
        return _size;
    }

    /**
    * @brief
    *  Get the minimum of this axis aligned cube.
    * @return
    *  the minimum of this axis aligned cube
    */
    PointType getMin() const {
        typedef ConstantGenerator<ScalarType> MyGenerator;
        return _center - VectorType(MyGenerator(_size/2.0f), std::make_index_sequence<VectorSpaceType::dimensionality()>{});
    }

    /**
     * @brief
     *  Get the maximum of this axis aligned cube.
     * @return
     *  the maximum of this axis aligned cube
     */
    PointType getMax() const {
        typedef ConstantGenerator<ScalarType> MyGenerator;
        return _center + VectorType(MyGenerator(_size/2.0f), std::make_index_sequence<VectorSpaceType::dimensionality()>{});
    }

    /**
     * @brief
     *  Assign this axis aligned cube with the values of another axis aligned cube.
     * @param other
     *  the other axis aligned cube
     * @post
     *  This axis aligned cube was assigned with the values of the other axis aligned cube.
     */
    void assign(const MyType& other) {
        _center = other._center;
        _size = other._size;
    }

public:

    /**
     * @brief
     *  Assign this axis aligned cube with the values of another axis aligned cube.
     * @param other
     *  the other axis aligned cube
     * @return
     *  this axis aligned cube
     * @post
     *  This axis aligned cube was assigned the values of the other axis aligned cube.
     */
    MyType& operator=(const MyType& other) {
        assign(other);
        return *this;
    }

public:
    bool operator==(const MyType& other) const {
        return _center == other._center
            && _size == other._size;
    }

    bool operator!=(const MyType& other) const {
        return _center != other._center
            || _size != other._size;
    }

}; // struct AxisAlignedCube

} // namespace Math
} // namespace Ego

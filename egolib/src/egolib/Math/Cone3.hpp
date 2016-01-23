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

/// @file  egolib/Math/Cone3.hpp
/// @brief Cones.

#pragma once

#include "egolib/Math/OrderedField.hpp"
#include "egolib/Math/VectorSpace.hpp"
#include "egolib/Math/EuclideanSpace.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  A cone given by its origin, an axis and its opening angle.
 * @todo
 *	Re-implement this crap.
 */
template <typename _ScalarType>
struct Cone3 {
public:
    /// @brief The Euclidean space over which the cones are defined.
    typedef EuclideanSpace<VectorSpace<Field<_ScalarType>, 3>> EuclideanSpaceType;
    /// The vector space type (of the Euclidean space).
    typedef typename EuclideanSpaceType::VectorSpaceType VectorSpaceType;
    /// The scalar field type (of the vector space).
    typedef typename EuclideanSpaceType::ScalarFieldType ScalarFieldType;
    /// The vector type (of the vector space).
    typedef typename EuclideanSpaceType::VectorType VectorType;
    /// The scalar type (of the scalar field).
    typedef typename EuclideanSpaceType::ScalarType ScalarType;
    /// @brief @a MyType is the type of this template/template specialization.
    typedef Cone3<_ScalarType> MyType;

public:
    VectorType origin;
    VectorType axis;

public:
    // use these values to pre-calculate trig functions based off of the opening angle
    ScalarType inv_sin;
    ScalarType sin_2;
    ScalarType cos_2;

public:
    /// @todo Zero axis is not allowed.
    Cone3() :
        origin(0, 0, 0),
        axis(0, 0, 0),
        inv_sin(0.0f),
        sin_2(0.0f),
        cos_2(0.0f)
    {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Assign this cone the values of another cone.
     * @param other
     *  the other cone
     * @post
     *  This cone was assigned the values of the other cone.
     */
    void assign(const MyType& other) {
        origin = other.origin;
        axis = other.axis;
        inv_sin = other.inv_sin;
        sin_2 = other.sin_2;
        cos_2 = other.cos_2;
    }

    /**
     * @brief
     *  Assign this cone the values of another cone.
     * @param other
     *  the other cone
     * @return
     *  this cone
     * @post
     *  This cone was assigned the values of the other cone.
     */
    MyType& operator=(const MyType& other) {
        assign(other);
        return *this;
    }

    /**
     * @brief
     *  Get the origin of this cone.
     * @return
     *  the origin of this cone
     */
    const VectorType& getOrigin() const {
        return origin;
    }

    /**
     * @brief
     *  Get the axis of this cone.
     * @return
     *  the axis of this cone
     */
    const VectorType& getAxis() const {
        return axis;
    }

    /**
     * @brief
     *  Translate this cone.
     * @param t
     *  the translation vector
     */
    void translate(const VectorType& t) {
        origin += t;
    }

};

} // namespace Math
} // namespace Ego

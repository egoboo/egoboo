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

/// @file  egolib/Math/Cube.hpp
/// @brief Cubes.

#pragma once

#include "egolib/Math/Vector.hpp"
#include "egolib/Math/Translatable.hpp"
#include "egolib/Math/EuclideanSpace.hpp"

namespace Ego {
namespace Math {

template <typename _EuclideanSpaceType>
struct Cube : public Translatable<typename _EuclideanSpaceType::VectorSpaceType> {
public:
    /// @brief The Euclidean space over which the cubes are defined.
    typedef _EuclideanSpaceType EuclideanSpaceType;
    /// The vector space type (of the Euclidean space).
    typedef typename EuclideanSpaceType::VectorSpaceType VectorSpaceType;
    /// The scalar field type (of the vector space).
    typedef typename EuclideanSpaceType::ScalarFieldType ScalarFieldType;
    /// The vector type (of the vector space).
    typedef typename EuclideanSpaceType::VectorType VectorType;
    /// The scalar type (of the scalar field).
    typedef typename EuclideanSpaceType::ScalarType ScalarType;
    /// @brief @a MyType is the type of this template/template specialization.
    typedef Cube<EuclideanSpaceType> MyType;

private:

    /**
     * @brief
     *  The center of the cube.
     */
    VectorType _center;
    
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
     *  Construct this cube with the default values of a cube.
     * @post
     *  This cube was constructed with the default values of a cube.
     * @remark
     *  The default values of a cube are the center of @a (0,0,0) and the size of @a 0.
     */
    Cube()
        : _center(), _size() {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Construct this cube with the specified values.
     * @param center
     *  the center of the cube
     * @param size
     *  the size of the cube
     * @throw std::domain_error
     *  if the size is negative
     * @pre
     *  The size is not negative.
     * @post
     *  The cube was constructed with the specified values.
     */
    Cube(const VectorType& center, const ScalarType& size)
        : _center(center), _size(size) {
        if (_size < 0) {
            throw std::domain_error("cube size is negative");
        }
    }

    /**
     * @brief
     *  Construct this cube with the values of another cube.
     * @param other
     *  the other cube
     * @post
     *  The cube was constructed with the values of the other cube.
     */
    Cube(const MyType& other)
        : _center(other._center), _size(other._size) {
        /* Intentionally empty. */
    }

public:

    /**
     * @brief
     *  Get the center of this cube.
     * @return
     *  the center of this cube
     */
    const VectorType getCenter() const {
        return _center;
    }

    /**
     * @brief
     *  Get the size of this cube.
     * @return
     *  the size of this cube
     */
    const ScalarType& getSize() const {
        return _size;
    }

    /**
    * @brief
    *  Get the minimum of this cube.
    * @return
    *  the minimum of this cube
    */
    VectorType getMin() const {
        typedef ConstantGenerator<ScalarType> MyGenerator;
        return _center - VectorType(MyGenerator(_size), std::make_index_sequence<VectorSpaceType::dimensionality()>{});
    }

    /**
     * @brief
     *  Get the maximum of this cube.
     * @return
     *  the maximum of this cube
     */
    VectorType getMax() const {
        typedef ConstantGenerator<ScalarType> MyGenerator;
        return _center + VectorType(MyGenerator(_size), std::make_index_sequence<VectorSpaceType::dimensionality()>{});
    }

    /**
     * @brief
     *  Assign this cube with the values of another cube.
     * @param other
     *  the other cube
     * @post
     *  This cube was assigned with the values of the other cube.
     */
    void assign(const MyType& other) {
        _center = other._center;
        _size = other._size;
    }

public:

    /**
     * @brief
     *  Assign this cube with the values of another cube.
     * @param other
     *  the other cube
     * @return
     *  this cube
     * @post
     *  This cube was assigned the values of the other cube.
     */
    MyType& operator=(const MyType& other) {
        assign(other);
        return *this;
    }

	/** @copydoc Ego::Math::translatable */
	void translate(const VectorType& t) override {
		_center += t;
	}

};

} // namespace Math
} // namespace Ego

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


namespace Ego {
namespace Math {

template <typename _EuclideanSpaceType>
struct AxisAlignedCube : public id::equal_to_expr<AxisAlignedCube<_EuclideanSpaceType>> {
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
        /// @todo Remove hard-coded constant.
        return _center - id::one<VectorType>() * (_size / 2.0f);
    }

    /**
     * @brief
     *  Get the maximum of this axis aligned cube.
     * @return
     *  the maximum of this axis aligned cube
     */
    PointType getMax() const {
		/// @todo Remove hard-coded constant.
		return _center + id::one<VectorType>() * (_size / 2.0f);
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
	// CRTP
    bool equal_to(const MyType& other) const {
        return _center == other._center
            && _size == other._size;
    }

}; // struct AxisAlignedCube

} // namespace Math
} // namespace Ego

namespace id {

/// @brief Specialization of id::enclose_functor enclosing an axis aligned cube in an axis aligned cube.
/// @detail The axis aligned cube \f$b\f$ enclosing an axis aligned cube \f$a\f$ is \f$a\f$ itself i.e. \f$b = a\f$.
/// @tparam E the Euclidean space type of the geometries
template <typename E>
struct enclose_functor<Ego::Math::AxisAlignedCube<E>,
	                   Ego::Math::AxisAlignedCube<E>>
{
	auto operator()(const Ego::Math::AxisAlignedCube<E>& source) const
	{ return source; }
}; // struct enclose_functor

/// @brief Specialization of id::is_enclosing_functor.
/// Determines if an axis aligned cube encloses a point.
/// @remark An axis aligned cube \$A\f$ does <em>not</em> enclose a point \f$P\f$
/// if for at least one axis \$k\f$ at least one of the following conditions is true:
/// - \f$P_k > A_{max_k}\f$
/// - \f$P_k < A_{min_k}\f$
/// Otherwise \f$A\f$ contains \f$P\f$.
/// This is a variant of the Separating Axis Theorem (aka SAT).
/// @tparam E the Euclidean space type of the geometries
template <typename E>
struct is_enclosing_functor<Ego::Math::AxisAlignedCube<E>,
	                        typename E::PointType>
{
	bool operator()(const Ego::Math::AxisAlignedCube<E>& a,
		            const typename E::PointType& b) const
	{
		for (size_t i = 0; i < E::Dimensionality; ++i)
		{
			if (a.getMax()[i] < b[i]) return false;
			// - the minimum of a is greater than the minimum of b.
			if (a.getMin()[i] > b[i]) return false;
		}
		return true;
	}
}; // struct is_enclosing_functor

/// @brief Specialization of id::is_enclosing_functor.
/// Determines if an axis aligned cube encloses another axis aligned cube.
/// @remark An axis aligned cube \f$A\f$ does <em>not</em> enclose an axis aligned cube \f$B\f$
/// if for at least one axis \$k\f$ at least one of the following conditions is true:
/// - \f$A_{min_k} > B_{min_k}\f$
/// - \f$A_{max_k} < B_{max_k}\f$
/// Otherwise \f$x\f$ contains \f$y\f$.
/// This is a variant of the Separating Axis Theorem (aka SAT).
/// @tparam E the Euclidean space type of the geometries
template <typename E>
struct is_enclosing_functor<Ego::Math::AxisAlignedCube<E>,
	                        Ego::Math::AxisAlignedCube<E>>
{
	bool operator()(const Ego::Math::AxisAlignedCube<E>& a,
		            const Ego::Math::AxisAlignedCube<E>& b) const
	{
		for (size_t i = 0; i < E::Dimensionality; ++i)
		{
			// If a is the cube that is supposed to contain the
			// cube b, then a does not contain b if along some axis
			// - the maximum of a is smaller than the maximum of b, or
			if (a.getMax()[i] < b.getMax()[i]) return false;
			// - the minimum of a is greater than the minimum of b.
			if (a.getMin()[i] > b.getMin()[i]) return false;
		}
		return true;
	}
}; // struct is_enclosing_functor

/// @brief Specialization of id::translate_functor.
/// Translates an axis aligned cube.
/// @tparam E the Euclidean space type of the geometry
template <typename E>
struct translate_functor<Ego::Math::AxisAlignedCube<E>,
	                     typename E::VectorType>
{
	auto operator()(const Ego::Math::AxisAlignedCube<E>& x,
		            const typename E::VectorType& t) const
	{
		return Ego::Math::AxisAlignedCube<E>(x.getCenter() + t, x.getSize());
	}
}; // struct translate_functor

/// @brief Specialization of id::is_intersecting_functor.
/// Determines if two axis aligned cubes intersect.
/// @remark Two axis aligned cubes \f$A\f$ and \f$B\f$ do <em>not</em> intersect
/// if for at least one axis \f$k\f$ at least one of the following conditions is true:
/// - \f$A_{min_k} > B_{max_k}\f$
/// - \f$A_{max_k} < B_{min_k}\f$
/// Otherwise \f$A\f$ and \f$B\f$ intersect.
/// This is a variant of the Separating Axis Theorem (aka SAT).
template <typename E>
struct is_intersecting_functor<Ego::Math::AxisAlignedCube<E>,
	                           Ego::Math::AxisAlignedCube<E>>
{
	bool operator()(const Ego::Math::AxisAlignedCube<E>& a,
		            const Ego::Math::AxisAlignedCube<E>& b) const
	{
		for (size_t i = 0; i < E::dimensionality(); ++i)
		{
			// If the minimum of a is greater than the maximum of b along one axis,
			// then they can not intersect.
			if (a.getMin()[i] > b.getMax()[i])
			{
				return false;
			}
			// If the maximum of a is smaller than the minimum of b along one axis,
			// then they can not intersect.
			if (a.getMax()[i] < b.getMin()[i])
			{
				return false;
			}
		}
		return true;
	}
}; // struct is_intersecting_functor

/// @brief Specialization of id::is_intersecting_functor.
/// Determines if an axis aligned cube and a point intersect.
/// @remark A point \f$P\f$ and an axis aligned cube \f$A\f$ do <em>not</em> intersect
/// if for at least one axis \f$k\f$ at least one of the following conditions is true:
/// - \f$P_k > A_{max_k}\f$
/// - \f$P_k < A_{min_k}\f$
/// Otherwise \f$P\f$ and \f$A\f$ intersect.
template <typename E>
struct is_intersecting_functor<Ego::Math::AxisAlignedCube<E>,
	                           typename E::PointType>
{
	bool operator()(const Ego::Math::AxisAlignedCube<E>& a,
		            const typename E::PointType& b) const
	{
		for (size_t i = 0; i < E::dimensionality(); ++i)
		{
			if (a.getMax()[i] < b[i]) return false;
			if (a.getMin()[i] > b[i]) return false;
		}
		return true;
	}
}; // struct is_intersecting_functor

/// @brief Specialization of id::is_intersecting_functor.
/// Determines if a point and an axis aligned cube intersect.
/// @remark The method for determinating if an axis aligned cube and a point intersect is
/// commutative. By swapping the arguments that method can be reused to determine if a
/// point and an axis aligned box intersect.
template <typename E>
struct is_intersecting_functor<typename E::PointType,
	                           Ego::Math::AxisAlignedCube<E>>
{
	bool operator()(const typename E::PointType& a, const Ego::Math::AxisAlignedCube<E>& b) const
	{ return is_intersecting(b, a); }
}; // struct is_intersecting_functor

} // namespace id

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
struct AxisAlignedBox : public id::equal_to_expr<AxisAlignedBox<_EuclideanSpaceType>> {
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
     *  Construct this AxisAlignedBox from the given points.
     * @param a
     *  one point
     * @param b
     *  another point
     * @remarks
     *  Given the points \f$a\f$ and \f$b\f$, the minimum \f$min\f$
     *  and the maximum \f$max\f$ of the axis aligned box are given
     *  by
     *  The minima and maxima of the axis aligned box are
     *  \f{align*}{
     *  min_i = \min\left(a_i,b_i\right) \;\; max_i = \max\left(a_i,b_i\right)
     *  \f}
     */
    AxisAlignedBox(const PointType& a, const PointType& b)
        : _min(a), _max(b) {
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality(); ++i) {
            if (_min[i] > _max[i]) {
                std::swap(_min[i], _max[i]);
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

	// CRTP
    bool equal_to(const MyType& other) const {
        return _min == other._min
            && _max == other._max;
    }

protected:
	struct Cookie {};
	friend struct id::translate_functor<MyType, VectorType>;
	AxisAlignedBox(Cookie cookie, const PointType& min, const PointType& max)
		: _min(min), _max(max) {
	}
}; // struct AxisAlignedBox

} // namespace Math
} // namespace Ego

namespace id {

/// @brief Specialization of id::enclose_functor enclosing an axis aligned box into an axis aligned box.
/// @details The axis aligned box \f$b\f$ enclosing an axis aligned box \f$a\f$ is \f$a\f$ itself i.e. \f$a = b\f$.
/// @tparam E the Euclidean space type of the geometries
template <typename E>
struct enclose_functor<Ego::Math::AxisAlignedBox<E>,
                       Ego::Math::AxisAlignedBox<E>>
{
	auto operator()(const Ego::Math::AxisAlignedBox<E>& source) const
	{ return source; }
}; // struct enclose_functor

/// @brief Specialization of id::is_enclosing_functor.
/// Determines if an axis aligned box encloses a point.
/// @tparam E the Euclidean space type of the geometry
template <typename E>
struct is_enclosing_functor<Ego::Math::AxisAlignedBox<E>,
                            typename E::PointType>
{
	bool operator()(const Ego::Math::AxisAlignedBox<E>& a,
                    const typename E::PointType& b) const
	{
		for (size_t i = 0; i < E::dimensionality(); ++i)
		{
			if (a.getMax()[i] < b[i]) return false;
			if (a.getMin()[i] > b[i]) return false;
		}
		return true;
	}
}; // struct is_enclosing_functor

/// @brief Specialization of id::is_enclosing_functor.
/// Determines if an axis aligned box contains another axis aligned box.
/// @remark An axis aligned box \f$A\f$ does <em>not</em> contain an axis aligned box \f$B\f$
/// if for at least one axis \$k\f$ at least one of the following conditions is true:
/// - \f$A_{min_k} > B_{min_k}\f$
/// - \f$A_{max_k} < B_{max_k}\f$
/// Otherwise \f$A\f$ contains \f$B\f$.
/// This is a variant of the Separating Axis Theorem (aka SAT).
/// @tparam E the Euclidean space type of the geometries
template <typename E>
struct is_enclosing_functor<Ego::Math::AxisAlignedBox<E>,
                            Ego::Math::AxisAlignedBox<E>>
{
	bool operator()(const Ego::Math::AxisAlignedBox<E>& a,
		            const Ego::Math::AxisAlignedBox<E>& b) const
	{
		for (size_t i = 0; i < E::dimensionality(); ++i)
		{
			// If a is the axis-aligned bounding box that is supposed to contain the
			// axis-aligned bounding box b, then a does not contain b if along some axis
			// - the maximum of a is smaller than the maximum of b, or
			if (a.getMax()[i] < b.getMax()[i]) return false;
			// - the minimum of a is greater than the minimum of b.
			if (a.getMin()[i] > b.getMin()[i]) return false;
		}
		return true;
	}
}; // struct is_enclosing_functor

/// @brief Specialization of id::translate_functor.
/// Translates an axis aligned box.
/// @tparam E the Euclidean space type of the geometry
template <typename E>
struct translate_functor<Ego::Math::AxisAlignedBox<E>,
	                     typename E::VectorType>
{
	auto operator()(const Ego::Math::AxisAlignedBox<E>& x,
		            const typename E::VectorType& t) const
	{
		return Ego::Math::AxisAlignedBox<E>(typename Ego::Math::AxisAlignedBox<E>::Cookie(),
			                                x.getMin() + t,
			                                x.getMax() + t);
	}
}; // struct translate_functor

/// @brief Specialization of id::is_intersecting_functor.
/// Determines wether two axis aligned boxes intersect.
/// @remark Two axis aligned boxes \f$A\f$ and \f$B\f$ do <em>not</em> intersect
/// if for at least one axis \f$k\f$ at least one of the following conditions is true:
/// - \f$A_{min_k} > B_{max_k}\f$
/// - \f$A_{max_k} < B_{min_k}\f$
/// Otherwise \f$A\f$ and \f$B\f$ intersect.
/// This is a variant of the Separating Axis Theorem (aka SAT).
/// @tparam E the Euclidean space type of the geometries.
template<typename E>
struct is_intersecting_functor<Ego::Math::AxisAlignedBox<E>,
                               Ego::Math::AxisAlignedBox<E>>
{
	bool operator()(const Ego::Math::AxisAlignedBox<E>& a,
		            const Ego::Math::AxisAlignedBox<E>& b) const
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
/// Determines if an axis aligned bounding box and a point intersect.
/// @remark A point \f$P\f$ and an axis aligned bounding box \f$A\f$ do <em>not</em> intersect
/// if for at least one axis \f$k\f$ at least one of the following conditions is true:
/// - \f$P_k > A_{max_k}\f$
/// - \f$P_k < A_{min_k}\f$
/// Otherwise \f$P\f$ and \f$A\f$ intersect.
template <typename E>
struct is_intersecting_functor<Ego::Math::AxisAlignedBox<E>,
	                           typename E::PointType>
{
	bool operator()(const Ego::Math::AxisAlignedBox<E>& a,
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
/// Determines if a point and an axis aligned box intersect.
/// @remark The method for determinating if an axis aligned box and a pointer intersect is
/// commutative. By swapping the arguments that method can be reused to determine if a
/// point and an axis aligned box intersect.
template <typename E>
struct is_intersecting_functor<typename E::PointType,
	                           Ego::Math::AxisAlignedBox<E>>
{
	bool operator()(const typename E::PointType& a,
                    const Ego::Math::AxisAlignedBox<E>& b) const
	{ return is_intersecting(b, a); }
}; // struct is_intersecting_functor

} // namespace id

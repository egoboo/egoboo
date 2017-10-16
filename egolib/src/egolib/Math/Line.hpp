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

/// @file egolib/Math/Line.hpp
/// @brief Rays.
/// @author Michael Heilmann

#pragma once


#include "egolib/Math/EuclideanSpace.hpp"


namespace Ego {
namespace Math {

/**
* @brief
*  A ray.
* @remark
*  A line in \f$\mathbb{R}^n\f$ is a set of points
*  \begin{align*}
*  L = \left\{ \left(1 - t\right) \cdot \mathbf{A} + t \vec{B} | t \in \mathbb{R} \right\}
*  \end{align*}
*  where \$A, B\f$ are location vectors in \f$\mathbb{R}^n\f$.
*/
template <typename _EuclideanSpaceType>
struct Line : public id::equal_to_expr<Line<_EuclideanSpaceType>> {
public:
    Ego_Math_EuclideanSpace_CommonDefinitions(Line);

private:
    /// The 1st \f$A\f$.
    PointType a;

    /// The 2nd point \f$B\f$.
    PointType b;

public:

    /**
     * @brief
     *  Default construct this line.
     * @post
     *  The line starts at the origin and is of zero length.
     */
    Line() : Line(PointType(), PointType()) {
    }

    /**
     * @brief
     *  Construct this line with the specified points.
     * @param a, b
     *   the points
     */
    Line(const PointType& a, const PointType& b) : a(a), b(b) {
    }

    /**
     * @brief
     *  Copy-construct this line with the values of another line.
     * @param other
     *  the other line
     */
    Line(const Line& other) : a(other.a), b(other.b) {
    }

    /**
     * @brief
     *  Assign this line with the values of another line.
     * @param other
     *  the other line
     * @return
     *  this line
     */
    const Line& operator=(const Line& other) {
        a = other.a;
        b = other.b;
        return *this;
    }

    /**
     * @brief
     *  Get the first endpoint of the line.
     * @return
     *  the first endpoint of the line
     */
    const PointType& getA() const {
        return a;
    }

    /**
     * @brief
     *  Get the second endpoint of the line.
     * @return
     *  the second endpoint of the line
     */
    const PointType& getB() const {
        return b;
    }
    
    /**
     * @brief
     *  Get a point \f$P(t)\f$ along this line.
     * @param t
     *  the parameter \f$t\f$
     * @return
     *  the point \f$P(t) = \vec{a} + t(\vec{b} - \vec{a})\f$ along the line
     * @remark
     *  The point \f$P(t)\f$ with parameter \f$t \in \mathbb{R}\f$ along the line
     *  \f$L = \left\{ A + t (B - A) | t \in \mathbb{R} \right\}$
     *  is defined as \f$P(t) = A + t (B-A)\f$.
     */
    VectorType getPoint(ScalarType t) const {
        return a + (b - a) * t;
    }

public:
	// CRTP
    bool equal_to(const MyType& other) const {
        return a == other.a
            && b == other.b;
    }

}; // struct Line

} // namespace Math
} // namespace Ego

namespace id {

/// @brief Specialization of id::enclose_functor enclosing a line in a line.
/// @detail The line \f$b\f$ enclosing a line \f$a\f$ is \f$a\f$ itself i.e. \f$b = a\f$.
/// @tparam E the Euclidean space type of the geometries
template <typename E>
struct enclose_functor<Ego::Math::Line<E>,
	                   Ego::Math::Line<E>>
{
	auto operator()(const Ego::Math::Line<E>& source) const
	{ return source; }
}; // struct enclose_functor

/// @brief Specialization of id::translate_functor.
/// Translates a line.
/// @tparam E the Euclidean space type of the geometry
template <typename E>
struct translate_functor<Ego::Math::Line<E>,
	                     typename E::VectorType>
{
	auto operator()(const Ego::Math::Line<E>& x,
		            const typename E::VectorType& t) const
	{ return Ego::Math::Line<E>(x.getA() + t, x.getB() + t); }
}; // struct translate_functor

} // namespace id

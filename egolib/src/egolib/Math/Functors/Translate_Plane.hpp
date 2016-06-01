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

/// @file egolib/Math/Functors/Translate_Plane.hpp
/// @brief Translation of planes.
/// @author Michael Heilmann

#pragma once


#include "egolib/Math/Plane.hpp"
#include "egolib/Math/Functors/Translate.hpp"


namespace Ego {
namespace Math {

/**
 * @remark
 *	The first (slow) method to compute the translation of a plane \f$\hat{n} \cdot P + d = 0\f$
 *	is to compute a point on the plane, translate the point, and compute from the new point and
 *	and the old plane normal the new plane:
 *	To translate a plane \f$\hat{n} \cdot P + d = 0\f$, compute a point on the plane
 *	\f$X\f$ (i.e. a point \f$\hat{n} \cdot X + d = 0\f$) by
 *  \f{align*}{
 *	X = (-d) \cdot \hat{n}
 *	\f}
 *	Translate the point \f$X\f$ by \f$\vec{t}\f$ into a new point \f$X'\f$:
 *	\f{align*}{
 *	X' = X + \vec{t}
 *	\f}
 *	and compute the new plane
 *	\f{align*}{
 *	\hat{n} \cdot P + d' = 0, d' = -\left(\hat{n} \cdot X'\right)
 *	\f}
 * @remark
 *	The above method is not the fastest method. Observing that the old and the new plane equation only
 *	differ by \f$d\f$ and \f$d'\f$, a faster method of translating a plane can be devised by computing
 *	\f$d'\f$ directly. Expanding \f$d'\f$ gives
 *	\f{align*}{
 *	d' =& -\left(\hat{n} \cdot X'\right)\\
 *     =& -\left[\hat{n} \cdot \left((-d) \cdot \hat{n} + \vec{t}\right)\right]\\
 *     =& -\left[(-d) \cdot \hat{n} \cdot \hat{n} + \hat{n} \cdot \vec{t}\right]\\
 *     =& -\left[-d + \hat{n} \cdot \vec{t}\right]\\
 *     =& d - \hat{n} \cdot \vec{t}
 *	\f}
 *	The new plane can then be computed by
 *	\f{align*}{
 *	\hat{n} \cdot P + d' = 0, d' = d - \hat{n} \cdot \vec{t}
 *	\f}
 * @param t
 *	the translation vector
 * @copydoc Ego::Math::translatable
 */
template <typename _EuclideanSpaceType>
struct Translate<Plane3<_EuclideanSpaceType, void>> {
    typedef Plane3<_EuclideanSpaceType, void> X;
    typedef typename _EuclideanSpaceType::VectorType T;
    X operator()(const X& x, const T& t) const {
        return X(x._n, x._d - x._n.dot(t));
    }
};

template <typename _EuclideanSpaceType>
Plane3<_EuclideanSpaceType, void> translate(const Plane3<_EuclideanSpaceType, void>& x, const typename _EuclideanSpaceType::VectorType& t) {
    Translate<Plane3<_EuclideanSpaceType, void>> f;
    return f(x, t);
}

} // namespace Math
} // namespace Ego

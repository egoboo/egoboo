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

/// @file egolib/Math/Plane.hpp
/// @brief 3D planes.
/// @author Michael Heilmann

#pragma once


#include "egolib/Math/EuclideanSpace.hpp"
#include "egolib/Math/Functors/Translate.hpp"


namespace Ego {
namespace Math {

/**
 * @brief
 *  A plane in the normal-distance form defined as
 *  \f[
 *  \hat{n} \cdot P + d = 0
 *  \f]
 *  where \f$\hat{n}\f$ is the unit plane normal and \f$d\f$ is the distance of the plane from the origin.
 *  Any point \f$P\f$ for which the above equation is fulfilled is on the plane.
 * @author
 *  Michael Heilmann
 */
template <typename _EuclideanSpaceType, typename _EnabledType = std::enable_if_t<_EuclideanSpaceType::dimensionality() == 3>>
struct Plane3 {
public:
    Ego_Math_EuclideanSpace_CommonDefinitions(Plane3);

private:

    /**
     * @brief
     *  The plane normal.
     * @invariant
     *  The plane normal is a unit vector.
     */
    VectorType _n;
    
    /**
     * @brief
     *  The distance of the plane from the origin.
     */
    ScalarType _d;

public:

    /**
     * @brief
     *  Default constructor.
     * @remark
     *  The default plane has the plane normal @a (0,0,1) and a distance from the origin of @a 0.
     */
    Plane3() :
        _n(0.0f, 0.0f, 1.0f), _d(0.0f)
    {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Create a plane from a plane equation \f$a + b + c + d = 0\f$.
     * @param a, b, c, d
     *  the plane equation
     * @remark
     *  Given a plane equation \f$a + b + c + d = 0\f$, the normal-distance form
     *  \f$(\hat{n}',d')\f$ is defined as \f$\hat{n}' = \frac{\vec{n}}{|\vec{n}'|}\f$
     *  and \f$d' = \frac{d}{|\vec{n}'|}\f$ where \f$\vec{n}' = (a,b,c)\f$.
     */
    Plane3(const ScalarType& a, const ScalarType& b, const ScalarType& c, const ScalarType& d)
        : _n(a, b, c), _d(d) {
        auto l = _n.length();
        if (ScalarFieldType::additiveNeutral() == l) {
            throw std::domain_error("normal vector is zero vector");
        }
        auto il = 1.0f / l;
        _n *= il;
        _d *= il;
    }

    /**
     * @brief
     *  Construct a plane with three non-collinear points.
     * @param a, b, c
     *  the point
     * @throw std::domain_error
     *  if the points are collinear
     * @remark
     *  Assume \f$a\f$, \f$b\f$ and \f$c\f$ are not collinear. Let \f$u = b - a\f$,
     *  \f$v = c - a\f$, \f$n = u \times v\f$, \f$\hat{n} = \frac{n}{|n|}\f$ and
     *  \f$d = -\left(\hat{n} \cdot a\right)\f$.
     * @remark
     *  We show that \f$a\f$, \f$b\f$ and \f$c\f$ are on the plane given by the
     *  equation \f$\hat{n} \cdot p + d = 0\f$. To show this for \f$a\f$, rewrite
     *   the plane equation to
     *  \f{eqnarray*}{
     *  \hat{n} \cdot p  + -(\hat{n} \cdot a) = 0\\
     *  \f}
     *  shows for \f$p=a\f$ immediatly that \f$a\f$ is on the plane.
     * @remark
     *  To show that \f$b\f$ and \f$c\f$ are on the plane, the plane equation is
     *  rewritten yet another time using the bilinearity property of the dot product
     *  and the definitions of \f$d\f$ and \f$\hat{n}\f$ its
     *  \f{align*}{
     *     &\hat{n} \cdot p + -(\hat{n} \cdot a)  \;\;\text{Def. of } d\\
     *  =&\hat{n} \cdot p + \hat{n} \cdot (-a)  \;\;\text{Bilinearity of the dot product}\\
     *  =&\hat{n} \cdot (p - a)                 \;\;\text{Bilinearity of the dot product}\\
     *  =&\frac{n}{|n|} \cdot (p - a)           \;\;\text{Def. of } \hat{n}\\
     *  =&(\frac{1}{|n|}n) \cdot (p - a)        \;\;\\
     *  =&\frac{1}{|n|}(n \cdot (p - a))       \;\;\text{Compatibility of the dot product w. scalar multiplication}\\
     *  =&n \cdot (p - a)                       \;\;\\
     *  =&(u \times v) \cdot (p - a)            \;\;\text{Def. of } n
     *  \f}
     *  Let \f$p = b\f$ then
     *  \f{align*}{
     *   &(u \times v) \cdot (b - a)            \;\text{Def. of } u\\
     *  =&(u \times v) \cdot u\\
     *  =&0
     *  \f}
     *  or let \f$p = c\f$ then
     *  \f{align*}{
     *   &(u \times v) \cdot (c - a)              \;\text{Def. of } u\\
     *  =&(u \times v) \cdot v\\
     *  =&0
     *  \f}
     *  as \f$u \times v\f$ is orthogonal to both \f$u\f$ and \f$v\f$.
     *  This shows that \f$b\f$ and \f$c\f$ are on the plane.
     */
    Plane3(const VectorType& a, const VectorType& b, const VectorType& c) {
        auto u = b - a;
        if (u == VectorType::zero()) {
            throw std::domain_error("b = a");
        }
        auto v = c - a;
        if (u == VectorType::zero()) {
            throw std::domain_error("c = a");
        }
        _n = u.cross(v);
        if (0.0f == _n.normalize()) {
            /* u x v = 0 is only possible for u,v != 0 if u = v and thus b = c. */
            throw std::domain_error("b = c");
        }
        _d = -_n.dot(a);
    }

    /**
     * @brief
     *  Construct a plane with a point and a normal.
     * @param p
     *  the point
     * @param n
     *  the normal
     * @throw std::domain_error
     *  if the normal vector is the zero vector
     * @remark
     *  The plane normal is normalized if necessary.
     * @remark
     *  Let \f$X\f$ be the point and \f$\vec{n}\f$ the unnormalized plane normal, then the plane equation is given by
     *  \f{align*}{
     *  \hat{n} \cdot P + d = 0, \hat{n}=\frac{\vec{n}}{|\vec{n}|}, d = -\left(\hat{n} \cdot X\right)
     *  \f}
     *  \f$X\f$ is on the plane as
     *  \f{align*}{
     *   &\hat{n} \cdot X + d\\
     *  =&\hat{n} \cdot X + -(\hat{n} \cdot X)\\
     *  =&\hat{n} \cdot X - \hat{n} \cdot X\\
     *  =&0
     *  \f}
     */
    Plane3(const VectorType& p, const VectorType& n)
        : _n(n), _d(0.0f) {
        if (_n.normalize() == 0.0f) {
            throw std::domain_error("normal vector is zero vector");
        }
        _d = -_n.dot(p);
    }

    /**
     * @brief
     *  Construct a plane from a translation axis and a distance from the origin.
     * @param t
     *  the translation axis
     * @param d
     *  the distance from the origin
     * @post
     *  Given an axis \f$\vec{t}\f$ and a distance from the origin \f$d\f$
     *  the plane equation is given by
     *  \f{align*}{
     *  \hat{n} \cdot p + d = 0, \hat{n}=\frac{\vec{t}}{|\vec{t}|}
     *  \f}
     * @throw std::domain_error
     *    if the translation axis is the zero vector
     */
    Plane3(const VectorType& t, const ScalarType& d)
        : _n(t), _d(d) {
        if (_n.normalize() == 0.0f) {
            throw std::domain_error("axis vector is zero vector");
        }
    }


    /**
     * @brief
     *  Copy-construct a plane from another plane.
     * @param other
     *  the other plane
     * @post
     *  This plane was constructed with the values of the other plane.
     */
    Plane3(const MyType& other)
        : _n(other._n), _d(other._d) {
        /* Intentionally empty. */
    }

public:

    /**
     * @brief
     *  Get the plane normal of this plane.
     * @return
     *  the plane normal of this plane
     */
    const VectorType& getNormal() const {
        return _n;
    }

    /**
     * @brief
     *  Get the distance of this plane from the origin.
     * @return
     *  the distance of this plane from the origin
     */
    const ScalarType& getDistance() const {
        return _d;
    }

public:
    bool operator==(const MyType& other) const {
        return _d == other._d
            && _n == other._n;
    }

    bool operator!=(const MyType& other) const {
        return _d != other._d
            || _n != other._n;
    }

public:
    /**
     * @brief
     *  Get the distance of a point from this plane.
     * @param point
     *  the point
     * @return
     *  the distance of the point from the plane.
     *  The point is in the positive (negative) half-space of the plane if the distance is positive (negative).
     *  Otherwise the point is on the plane.
     * @remark
     *  Let \f$\hat{n} \cdot P + d = 0\f$ be a plane and \f$v\f$ be some point.
     *  We claim that \f$d'=\hat{n} \cdot v + d\f$ is the signed distance of the
     *  point \f$v\f$ from the plane.
     *
     *  To show this, assume \f$v\f$ is not in the plane. Then there
     *  exists a single point \f$u\f$ on the plane which is closest to
     *  \f$v\f$ such that \f$v\f$ can be expressed by translating \f$u\f$
     *  along the plane normal by the signed distance \f$d'\f$ from
     *  \f$u\f$ to \f$v\f$ i.e. \f$v = u + d' \hat{n}\f$. Obviously,
     *  if \f$v\f$ is in the positive (negative) half-space of the plane, then
     *  \f$d'>0\f$ (\f$d' < 0\f$). We obtain now
     *  \f{align*}{
     *    &\hat{n} \cdot v + d\\
     *  = &\hat{n} \cdot (u + d' \hat{n}) + d\\
     *  = &\hat{n} \cdot u + d' (\hat{n} \cdot \hat{n}) + d\\
     *  = &\hat{n} \cdot u + d' + d\\
     *  = &\hat{n} \cdot u + d + d'\\
     *  \f}
     *  However, as \f$u\f$ is on the plane
     *  \f{align*}{
     *  = &\hat{n} \cdot u + d + d'\\
     *  = &d'
     *  \f}
     */
    ScalarType distance(const PointType& point) const {
        return _n.dot(PointType::toVector(point)) + _d;
    }

	/**
	 * @brief
	 *	Get a point on this plane.
	 * @return
	 *	a point on this plane
	 * @remark
	 *	The point \f$X = (-d) \hat{n}\f$ is guaranteed to be on the plane.
	 *	To see that, insert \f$X\f$ into the plane equation:
	 *	\f{align*}{
	 *	\hat{n} \cdot X + d = \hat{n} \cdot \left[(-d) \hat{n}\right] + d
	 *	= (-d) \left(\hat{n} \cdot \hat{n}\right] + d
	 *	\f}
	 *	As \f$\hat{n}\f$ is a unit vector
	 *	\f{align*}{
	 *	  (-d) \left[\hat{n} \cdot \hat{n}\right] + d
	 *	= -d + d
	 *	= 0
	 *	\f}
	 */
	VectorType getPoint() const {
		return _n * (-_d);
	}

protected:
    struct Cookie {};
    friend struct Translate<MyType>;
    Plane3(Cookie cookie, const VectorType& n, const ScalarType& d)
        : _n(n), _d(d) {}

}; // struct Plane3

} // namespace Math
} // namespace Ego

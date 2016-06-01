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

/// @file egolib/Math/Cone3.hpp
/// @brief Single infinite 3D cones.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Angle.hpp"
#include "egolib/Math/EuclideanSpace.hpp"
#include "egolib/Math/Functors/Translate.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  An single infinite cone is defined an origin point \f$O\f$, an axis ray whose origin is at
 *  \f$O\f$ and unit length direction is \f$\hat{d}\f$, and and an acute angle \f$\theta\in\left
 *  (0, 90\right)\f$. A single cone is not bounded by its
 * @remark
 *  A point \$P\f$ is inside a cone if the angle between the axis ray \f$\hat{d}\f$ and \f$P - O\f$ is in \f$[0,\theta)\f$
 *  and one a cone if that angle is exactly 180.
 *
 *  That is, if \f$\alpha \in [0,180]\f$ is the angle between those vectors, then \f$\alpha \leq \theta\f$
 *  has to hold. Note that the cosine is a strictly(!) decreasing function over the interval \$[0,180]\$
 *  (ranging from 1 to 0). Hence \f$\alpha \leq \theta\f$ holds iff \f$\cos\alpha \geq \cos \theta\f$
 *  holds.
 *
 *  \f$\cos\alpha\f$ can be computed using the dot product
 *  \f{align*}{
 *             &\hat{d} \cdot \left(P-O\right) = |\hat{d}||P-O| \cos \alpha\\
 *  \Rightarrow&\hat{d} \cdot \left(P-O\right) = \cos \alpha\\
 *  \Rightarrow&\frac{\hat{d} \cdot \left(P-O\right) }{|P-O|} = \cos \alpha\\
 *  \Rightarrow&\hat{d} \cdot \frac{P-O}{|P-O|} = \cos \alpha
 *  \f}
 *  Consequently, the point \f$P\f$ is in the cone if
 *  \f{align*}{
 *  \hat{d} \cdot \frac{P-O}{|P-O|}  \geq \cos \theta
 *  \f}
 *  holds. This equation only holds if \f$P \neq O\f$. Rewriting the equation, one
 *  obtains
 *  \f{align*}{
 *  \hat{d} \cdot (P-O) \geq |P-O|\cos \theta
 *  \f}
 *  In this form, no test if \f$P \neq O\f$ holds is required. This is easy to see as
 *  when \$P=O\f$ holds, then \f$P\f$ is obviously inside the cone. In that case the
 *  inequality holds as both sides evaluate to zero.
 *
 *  To summarize: The point is on the cone if \f$\hat{d} \cdot (P-O) = |P-O|\cos \theta\f$
 *  and is inside the cone if \f$\hat{d} \cdot (P-O) > |P-O|\cos \theta\$. Otherwise it is
 *  outside of the cone.
 */
template <typename _EuclideanSpaceType, typename _EnabledType = std::enable_if_t<_EuclideanSpaceType::dimensionality() == 3>>
struct Cone3 {
public:
    Ego_Math_EuclideanSpace_CommonDefinitions(Cone3);

public:
    /** @brief The origin point \f$P\f$ of the cone. */
    PointType origin;
    /** @brief The unit axis vector \f$\hat{a}\f$ of the cone. */
    VectorType axis;
    /** @brief The angle, in degrees, \f$\theta \in \left(0,90\right)\f$. */
    Angle<AngleUnit::Degrees> angle;

public:
    /**
     * @brief Construct this cone with default values.
     * @remark The default values of a cone are
     * \f$O=\left(0,0,0\right)\f$,
     * \f$\hat{a}=\left(0,0,1\right)$, and
     * \f$\theta=60\f$.
     */
    Cone3() :
        origin(0, 0, 0), axis(0, 0, 1), angle(60.0f) {
        /* Intentionally empty. */
    }

    /**
     * @brief Construct this cone.
     * @param origin the origin point \f$O\f$ of this cone
     * @param axis the axis vector \f$\vec{a},\vec{a} \neq \vec{0}\f$ of this cone
     * @param angle the acute angle \f$\theta\f$, in degrees
     * @throw Id::RuntimeErrorException \f$\vec{a} = \vec{0}\f$
     * @throw Id::RuntimeErrorException \f$\theta\f$ is not an acute angle 
     */
    Cone3(const PointType& origin, const VectorType& axis, const Angle<AngleUnit::Degrees>& angle)
        : origin(origin), axis(axis), angle(angle) {
        this->axis.normalized();
        if (!angle.isAcute()) {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "the angle is not an acute angle");
        }
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
        angle = other.angle;
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
    const PointType& getOrigin() const {
        return origin;
    }

    /**
     * @brief
     *  Get the unit axis vector \f$\hat{a}\f$ of this cone.
     * @return
     *  the unit axis vector of \f$\hat{a}\f$ of this cone
     */
    const VectorType& getAxis() const {
        return axis;
    }

    /**
     * @brief
     *  Get the angle, in degrees, \f$\theta \in \left(0,90\right)\f$ of this cone
     * @return
     *  the angle, in degrees, \f$\theta \in \left(0,90\right)\f$ of this cone
     */
    const Angle<AngleUnit::Degrees>& getAngle() const {
        return angle;
    }

    /**
     * @brief
     *  Get the radius of this cone at the specified height \f$h \in \left[0,+\infty\right)\f$.
     * @param height
     *   the height \f$h \in \left[0,+\infty\right)\f$.
     * @return
     *  the radius at the specified height
     * @throw Id::RuntimeErrorException
     *  if the height is negative
     * @remark
     *  Given a right triangle, it is known that \f$\tan\alpha=
     *  \frac{\mathit{opposite}}{\mathit{adjacent}}\f$.
     *
     *  The height \f$h\f$ is the adjacent, the radius \f$r\f$ is
     *  the opposite and the angle is \f$\theta\f$, one   obtains
     *  \f$\tan\alpha = \frac{r}{h}\f$, hence \f$r=h \tan\alpha\f$.
     */
    ScalarType getRadiusAt(ScalarType height) const {
        return height * std::tan(angle);
    }

    /**
     * @brief
     *  Get the slant height of this cone at the specified height \f$h \in \left[0,+\infty\right)\f$.
     * @param height
     *   the height \f$h \in \left[0,+\infty\right)\f$.
     * @return
     *  the slant height at the specified height
     * @throw Id::RuntimeErrorException
     *  if the height is negative
     * @remark
     *  Given a right triangle, it is known that \f$\cos\alpha=
     *  \frac{\mathit{adjacent}}{\mathit{hypotenuse}}\f$.
     *
     *  The height \f$h\f$ is the adjacent, the slant height \f$s\f$ is
     *  the hypotenuse and the angle is \f$\theta\f$,    one   obtains
     *  \f$\cos\alpha = \frac{h}{s}\f$, hence \f$s = \frac{h}{\cos\alpha}\f$.
     * @remark
     *  The Pythagorean theorem provides is with \f$s=\sqrt{r^2+h^2}\f$,
     *  however, squaring two numbers and taking the square root seems
     *  more expensive than just taking the cosine.
     */
    ScalarType getSlantHeight(ScalarType height) const {
        return height / std::cos(angle);
    }

public:
    bool operator==(const MyType& other) const {
        return origin == other.origin
            && axis == other.axis
            && angle == other.angle;
    }

    bool operator!=(const MyType& other) const {
        return origin != other.origin
            || axis != other.axis
            || angle != other.angle;
    }

protected:
    struct Cookie {};
    friend struct Translate<MyType>;
    Cone3(Cookie cookie, const PointType& origin, const VectorType& axis, const Angle<AngleUnit::Degrees>& angle)
        : origin(origin), axis(axis), angle(angle) {
    }

}; // struct Cone3

} // namespace Math
} // namespace Ego

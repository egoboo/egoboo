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

#include "egolib/platform.h"

namespace idlib {

template <typename P>
struct cone;

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
template <typename S>
struct cone<point<vector<S, 3>>> : public equal_to_expr<cone<point<vector<S, 3>>>>
{
public:
	/// @brief The point type of this cone type.
	using point_type = point<vector<S, 3>>;

	/// @brief The vector type of this cone type.
	using vector_type = typename point_type::vector_type;

	/// @brief The scalar type of this cone type.
	using scalar_type = typename point_type::scalar_type;

	/// @brief The angle type of this cone type.
	using angle_type = angle<float, degrees>;

	/// @brief Construct this cone with default values.
	/// @remark The default values of a cone are
	/// - origin point \f$O=\left(0,0,0\right)\f$,
	/// - axis vector \f$\hat{a}=\left(0,0,1\right)$, and
	/// - angle \f$\theta=60\f$.
    cone()
		: m_origin(zero<point_type>()), 
		  m_axis(zero<scalar_type>(), zero<scalar_type>(), one<scalar_type>()), 
		  m_angle(60.0f)
	{}

    /// @brief Construct this cone.
	/// @param origin the origin point \f$O\f$ of this cone
	/// @param axis the axis vector \f$\vec{a},\vec{a} \neq \vec{0}\f$ of this cone
    /// @param angle the acute angle \f$\theta\f$, in degrees
    /// @throw idlib::runtime_error \f$\vec{a} = \vec{0}\f$
    /// @throw idlib::runtime_error \f$\theta\f$ is not an acute angle 
    cone(const point_type& origin, const vector_type& axis, const angle<float, degrees>& angle)
        : m_origin(origin), m_axis(axis), m_angle(angle) 
	{
		m_axis = normalize(axis, euclidean_norm_functor<vector_type>{}).get_vector();
        if (!m_angle.is_acute())
		{
            throw runtime_error(__FILE__, __LINE__, "the angle is not an acute angle");
        }
    }

	cone(const cone&) = default;
	cone& operator=(const cone&) = default;

    /// @brief Get the origin of this cone.
    /// @return the origin of this cone
    const point_type& get_origin() const
	{ return m_origin; }

    /// @brief Get the unit axis vector \f$\hat{a}\f$ of this cone.
    /// @return the unit axis vector of \f$\hat{a}\f$ of this cone
    const vector_type& get_axis() const
	{ return m_axis; }

    /// @brief Get the angle, in degrees, \f$\theta \in \left(0,90\right)\f$ of this cone
    /// @return the angle, in degrees, \f$\theta \in \left(0,90\right)\f$ of this cone
    const angle_type& get_angle() const
	{ return m_angle; }

    /// @brief Get the radius of this cone at the specified height \f$h \in \left[0,+\infty\right)\f$.
    /// @param height the height \f$h \in \left[0,+\infty\right)\f$.
    /// @return the radius at the specified height
    /// @throw idlib::runtime_error the height is negative
    /// @remark
    /// Given a right triangle, it is known that \f$\tan\alpha=
    /// \frac{\mathit{opposite}}{\mathit{adjacent}}\f$.
    ///
    /// The height \f$h\f$ is the adjacent, the radius \f$r\f$  is
    /// the opposite and the angle is \f$\theta\f$, one    obtains
    /// \f$\tan\alpha = \frac{r}{h}\f$, hence \f$r=h \tan\alpha\f$.
    scalar_type get_radius_at(scalar_type height) const
	{ return height * std::tan(m_angle); }

    /// @brief Get the slant height of this cone at the specified height \f$h \in \left[0,+\infty\right)\f$.
    /// @param height the height \f$h \in \left[0,+\infty\right)\f$.
    /// @return the slant height at the specified height
    /// @throw idlib::runtime_error the height is negative
    /// @remark
    /// Given a right triangle, it is known that \f$\cos\alpha=
    /// \frac{\mathit{adjacent}}{\mathit{hypotenuse}}\f$.
    ///
    /// The height \f$h\f$ is the adjacent, the slant height \f$s\f$ is
    /// the hypotenuse and the angle is \f$\theta\f$,    one   obtains
    /// \f$\cos\alpha = \frac{h}{s}\f$, hence \f$s = \frac{h}{\cos\alpha}\f$.
    /// @remark
    /// The Pythagorean theorem provides is with \f$s=\sqrt{r^2+h^2}\f$,
    /// however, squaring two numbers and taking the square root seems
    /// more expensive than just taking the cosine.
    scalar_type get_slant_height(scalar_type height) const
	{ return height / std::cos(m_angle); }

	// CRTP
    bool equal_to(const cone& other) const
	{
        return m_origin == other.m_origin
            && m_axis == other.m_axis
            && m_angle == other.m_angle;
    }

protected:
    struct cookie {};
    friend struct translate_functor<cone, vector_type>;
    cone(cookie cookie, const point_type& origin, const vector_type& axis, const angle<float, degrees>& angle)
        : m_origin(origin), m_axis(axis), m_angle(angle)
	{}

private:
	/// @brief The origin point \f$P\f$ of the cone.
	point_type m_origin;
	
	/// @brief The unit axis vector \f$\hat{a}\f$ of the cone.
	vector_type m_axis;
	
	/// @brief The angle, in degrees, \f$\theta \in \left(0,90\right)\f$.
	idlib::angle<float, idlib::degrees> m_angle;

}; // struct cone

 /// @brief Specialization of idlib::enclose_functor enclosing a cone into a cone.
 /// @details The cone \f$b\f$ enclosing a cone \f$a\f$ is \f$a\f$ itself i.e. \f$a = b\f$.
 /// @tparam P the point type of the cone type
template <typename P>
struct enclose_functor<cone<P>, cone<P>>
{
	auto operator()(const cone<P>& source) const
	{ return source; }
}; // struct enclose_functor

/// @brief Specialization of idlib::translate_functor.
/// Translates a cone.
/// @tparam P the point type of the cone type
template <typename P>
struct translate_functor<cone<P>, typename P::vector_type>
{
	auto operator()(const cone<P>& x, const typename P::vector_type& t) const
	{
		return cone<P>(typename cone<P>::cookie(), x.get_origin() + t, x.get_axis(), x.get_angle());
	}
}; // struct translate_functor

} // namespace idlib

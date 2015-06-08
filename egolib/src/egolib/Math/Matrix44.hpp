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

/// @file   egolib/Math/Matrix44.hpp
/// @brief  4x4 matrices.
/// @author Michael Heilmann et al.
#pragma once

#include "egolib/_math.h"
#include "egolib/Math/Vector.hpp"

/// @brief Egoboo currently uses column-major format. This will change to column major.
#define fmat_4x4_layout_RowMajor (1)
#define fmat_4x4_layout_ColumnMajor (2)
#define fmat_4x4_layout fmat_4x4_layout_ColumnMajor

typedef float fmat_4x4_base_t[16];      ///< the basic 4x4 single precision floating point ("float")  matrix type

/// A wrapper for fmat_4x4_base_t.
/// Necessary in C so that the function return can be assigned to another matrix more simply.
struct fmat_4x4_t
{
	union
	{
		fmat_4x4_base_t v;
		/**
		 * @remark:
		 *	A two dimensional array \f$a[4][4]\f$ is layed out in memory as
		 *	\f[
		 *	a[0][0] a[0][1] a[0][2] a[0][3] a[1][0] a[1][1] a[1][2] a[1][3] \ldots a[3][0] a[3][1] a[3][2] a[3][3]
		 *	\f]
		 *	and an one dimensional array \f$b[16]\f$ is layed out in memory as
		 *	\f[
		 *	a[0]    a[1]    a[2]    a[3]    a[4]    a[5]    a[6]    a[7]           a[12]   a[13]   a[14]   a[15]
		 *	\f]
		 */
		float v2[4][4];
	};

    /**
     * @brief
     *  Get the identity (aka multiplicative neutral) matrix.
     * @return
     *  the identity matrix
     * @remark
     *	The \f$4 \times 4\f$ identity (aka multiplicative neutral) matrix is defined as
     *	\f[
     *		\left[\begin{matrix}
     *		1 & 0 & 0 & 0 \\
     *		0 & 1 & 0 & 0 \\
     *		0 & 0 & 1 & 0 \\
     *		0 & 0 & 0 & 1
     *		\end{matrix}\right]
     * \f]
     */
    static const fmat_4x4_t& identity()
    {
        static const fmat_4x4_t identity
            (
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            );
        return identity;
    }

    /**
     * @brief
     *	Get the zero (aka additive neutral) matrix.
     * @return
     *  the zero matrix
     * @remark
     *	The \f$4 \times 4\f$ zero (aka additive neutral) matrix is defined as
     *	\f[
     *	\left[\begin{matrix}
     *	0 & 0 & 0 & 0 \\
     *	0 & 0 & 0 & 0 \\
     *	0 & 0 & 0 & 0 \\
     *	0 & 0 & 0 & 0
     *	\end{matrix}\right]
     *	\f]
     */
    static const fmat_4x4_t& zero()
    {
        static const fmat_4x4_t zero
            (
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f
            );
        return zero;
    }

    /**
     * @brief
     *  Compute the tensor product matrix of two vectors.
     * @param u, v
     *  the vector
     * @return
     *  the tensor product matrix
     * @remark
     *  The \f$3 \times 3\f$ tensor product matrix \f$\vec{u}\Oplus\vec{v}\f$ of two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^3\f$ is defined as
     *  \f[
     *  \vec{u}\otimes\vec{v} =
     *  \left[\begin{matrix}
     *  u_x v_x & u_x v_y & u_x v_z \\
     *  u_y v_x & u_y v_y & u_y v_z \\
     *  u_z v_x & u_z v_y & u_z v_z
     *  \end{matrix}\right]
     *  \f]
     * @remark
     *  For the special case of \f$\vec{u}=\vec{w}\f$,\f$\vec{v}=\vec{w}\f$ the tensor product matrix reduces to
     *  \f[
     *  \vec{w}\otimes\vec{w} =
     *  \left[\begin{matrix}
     *  w^2_x   & w_x w_y & w_x w_z \\
     *  w_x w_y & w^2_y   & w_y w_z \\
     *  w_x w_z & w_y w_z & w^2_z
     *  \end{matrix}\right]
     *  \f]
     * @todo
     *  Move this into fvec4_t.
     * @todo
     *  Add an implementation for fvec2_t and fvec3_t returning fmat_2x2_t and fmat_3x3_t.
     * @todo
     *  Update documentation for the fvec4_t case.
     */
    static fmat_4x4_t tensor(const fvec4_t& v, const fvec4_t& w)
    {
        return
            fmat_4x4_t
            (
                v[kX] * w[kX], v[kX] * w[kY], v[kX] * w[kZ], v[kX] * w[kW],
                v[kY] * w[kX], v[kY] * w[kY], v[kY] * w[kZ], v[kY] * w[kW],
                v[kZ] * w[kX], v[kZ] * w[kY], v[kZ] * w[kZ], v[kZ] * w[kW],
                v[kW] * w[kX], v[kW] * w[kY], v[kW] * w[kZ], v[kW] * w[kW]
            );
    }
public:

    fmat_4x4_t()
	{
		for (size_t i = 0; i < 16; ++i)
		{
			this->v[i] = 0.0f;
		}
	}

	fmat_4x4_t
		(
			float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33
		)
	{
#if fmat_4x4_layout == fmat_4x4_layout_RowMajor
        v2[0][0] = m00; v2[0][1] = m01; v2[0][2] = m02; v2[0][3] = m03;
        v2[1][0] = m10; v2[1][1] = m11; v2[1][2] = m12; v2[1][3] = m13;
        v2[2][0] = m20; v2[2][1] = m21; v2[2][2] = m22; v2[2][3] = m23;
        v2[3][0] = m30; v2[3][1] = m31; v2[3][2] = m32; v2[3][3] = m33;
#elif fmat_4x4_layout == fmat_4x4_layout_ColumnMajor
        v2[0][0] = m00; v2[1][0] = m01; v2[2][0] = m02; v2[3][0] = m03;
        v2[0][1] = m10; v2[1][1] = m11; v2[2][1] = m12; v2[3][1] = m13;
        v2[0][2] = m20; v2[1][2] = m21; v2[2][2] = m22; v2[3][2] = m23;
        v2[0][3] = m30; v2[1][3] = m31; v2[2][3] = m32; v2[3][3] = m33;
#else
#error(fmat_4x4_layout must be either fmat_4x4_layout_RowMajor or fmat_4x4_layout_ColumnMajor)
#endif
	}

	fmat_4x4_t(const fmat_4x4_t& other)
	{
		for (size_t i = 0; i < 16; ++i)
		{
			this->v[i] = other.v[i];
		}
	}

	/**
	 * @brief
	 *	Get the translation vector of this matrix i.e. the vector \f$(m_{0,3},m_{1,3},m_{2,3})\f$.
	 * @return
	 *	the translation vector
	 */
	fvec3_t getTranslation() const
	{
		return fvec3_t((*this)(0, 3), (*this)(1, 3), (*this)(2, 3));
	}

	const float& operator()(const size_t i) const
	{
#ifdef _DEBUG
		EGOBOO_ASSERT(i < 16);
#endif
		return this->v[i];
	}
	float& operator()(const size_t i)
	{
#ifdef _DEBUG
		EGOBOO_ASSERT(i < 16);
#endif
		return this->v[i];
	}
	const float& operator()(const size_t i, const size_t j) const
	{
#ifdef _DEBUG
		EGOBOO_ASSERT(i < 4);
		EGOBOO_ASSERT(j < 4);
#endif
#if fmat_4x4_layout == fmat_4x4_layout_RowMajor
		return this->v2[i][j];
#elif fmat_4x4_layout == fmat_4x4_layout_ColumnMajor
		return this->v2[j][i];
#else
	#error(fmat_4x4_layout must be either fmat_4x4_layout_RowMajor or fmat_4x4_layout_ColumnMajor)
#endif
	}
	float& operator()(const size_t i, const size_t j)
	{
#ifdef _DEBUG
		EGOBOO_ASSERT(i < 4);
		EGOBOO_ASSERT(j < 4);
#endif
#if fmat_4x4_layout == fmat_4x4_layout_RowMajor
		return this->v2[i][j];
#elif fmat_4x4_layout == fmat_4x4_layout_ColumnMajor
		return this->v2[j][i];
#else
		#error(fmat_4x4_layout must be either fmat_4x4_layout_RowMajor or fmat_4x4_layout_ColumnMajor)
#endif
	}

	/**
	 * @brief
	 *	Compute the product of this matrix and another matrix.
	 * @param other
	 *	the other matrix
	 * @return
	 *	the product <tt>(*this) * other</tt>
	 * @remark
	 *	The product \f$C = A \cdot B\f$ of two \f$4 \times 4\f$ matrices \f$A\f$ and \f$B\f$ is defined as
	 *	\f[
	 *	C_{i,j} = \sum_{i=0}^3 A_{i,k} \cdot B_{k,j}
	 *	\f]
	 */
	fmat_4x4_t multiply(const fmat_4x4_t& other) const
	{
		fmat_4x4_t result;
		for (size_t i = 0; i < 4; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				for (size_t k = 0; k < 4; k++)
				{
					result(i, j) += (*this)(i, k) * other(k, j);
				}
			}
		}
		return result;
	}

	/**
	 * @brief
	 *	Compute the product of this matrix and another matrix.
	 * @param other
	 *	the other matrix
	 * @return
	 *	the product <tt>(*a) * b</tt>
	 */
	fmat_4x4_t operator*(const fmat_4x4_t& other) const
	{
		return multiply(other);
	}

    /**
     * @brief
     *  Compute the product of this matrix and a scalar.
     * @param scalar
     *  the scalar
     * @return
     *  the matrix
     */
    fmat_4x4_t operator*(const float& scalar) const
    {
        return
            fmat_4x4_t
            (
                (*this)(0, 0) * scalar, (*this)(0, 1) * scalar, (*this)(0, 2) * scalar, (*this)(0, 3) * scalar,
                (*this)(1, 0) * scalar, (*this)(1, 1) * scalar, (*this)(1, 2) * scalar, (*this)(1, 3) * scalar,
                (*this)(2, 0) * scalar, (*this)(2, 1) * scalar, (*this)(2, 2) * scalar, (*this)(2, 3) * scalar,
                (*this)(3, 0) * scalar, (*this)(3, 1) * scalar, (*this)(3, 2) * scalar, (*this)(3, 3) * scalar
            );
    }

	/**
	 * @brief
	 *	Compute the transpose of a matrix.
	 * @param a
	 *	the matrix
	 * @return
	 *	the transpose <tt>a^T</tt>
	 * @remark
	 *	The transpose \f$M^T\f$ of a matrix \f$M\f$ is defined as
	 *	\f[
	 *	M^T_{i,j} = M_{j,i}
	 *	\f]
	 */
	fmat_4x4_t getTranspose() const
	{
		return
			fmat_4x4_t
			(
				(*this)(0, 0), (*this)(1, 0), (*this)(2, 0), (*this)(3,0),
				(*this)(0, 1), (*this)(1, 1), (*this)(2, 1), (*this)(3,1),
				(*this)(0, 2), (*this)(1, 2), (*this)(2, 2), (*this)(3,2),
				(*this)(0, 3), (*this)(1, 3), (*this)(2, 3), (*this)(3,3)
			);
	}

    /**
     * @brief
     *  Overloaded addition operator.
     */
    fmat_4x4_t operator+(const fmat_4x4_t& other) const
    {
        fmat_4x4_t result = *this;
        return result += other;
    }

	/**
	 * @brief
	 *	Overloaded assignment addition operator.
	 */
	fmat_4x4_t& operator+=(const fmat_4x4_t& other)
	{
		for (size_t i = 0; i < 16; ++i)
		{
			(*this)(i) += other(i);
		}
		return *this;
	}

    /**
     * @brief
     *  Overloaded subtraction operator.
     */
    fmat_4x4_t operator-(const fmat_4x4_t& other) const
    {
        fmat_4x4_t result = *this;
        return result -= other;
    }

    /**
     * @brief
     *  Overloaded unary minus operator.
     */
    fmat_4x4_t operator-() const
    {
        return (*this) * (-1.0f);
    }

	/**
	 * @brief
	 *	Overloaded assignment subtraction operator.
	 */
	fmat_4x4_t& operator-=(const fmat_4x4_t& other)
	{
		for (size_t i = 0; i < 16; ++i)
		{
			(*this)(i) -= other(i);
		}
		return *this;
	}

	/**
	 * @brief
	 *	Assign this matrix the values of another matrix.
	 * @param other
	 *	the other matrix
	 * @post
	 *	This matrix was assigned the values of another matrix.
	 */
	void assign(const fmat_4x4_t& other)
	{
		if (this != &other)
		{
			for (size_t i = 0; i < 16; ++i)
			{
				(*this)(i) = other(i);
			}
		}
	}

	/**
	 * @brief
	 *	Assign this matrix the values of another matrix.
	 * @param other
	 *	the other matrix
	 * @return
	 *	this matrix
	 * @post
	 *	This matrix was assigned the values of another matrix.
	 */
	fmat_4x4_t& operator=(const fmat_4x4_t& other)
	{
		assign(other);
		return *this;
	}

	/**
	 * @brief
	 *	Get if this matrix is equal to another matrix.
	 * @param other
	 *	the other matrix
	 * @return
	 *	@a true if this matrix is equal to the other matrix
	 */
	bool operator==(const fmat_4x4_t& other) const
	{
		for (size_t i = 0; i < 16; ++i)
		{
			if ((*this)(i) != other(i))
			{
				return false;
			}
		}
		return true;
	}

	/**
	 * @brief
	 *	Get if this matrix is not equal to another matrix
	 * @param other
	 *	the other matrix
	 * @return
	 *	@a true if this matrix is not equal to the other matrix
	 */
	bool operator!=(const fmat_4x4_t& other) const
	{
		for (size_t i = 0; i < 16; ++i)
		{
			if ((*this)(i) != other(i))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * @brief
	 *	Get a viewing transformation (~ world space -> camera space) matrix.
	 * @param eye
	 *	the position of the eye point
	 * @param center
	 *	the position of the reference point
	 * @param up
	 *	the direction of the up vector
     * @return
     *  the matrix
	 * @pre
	 *	eye != center (debug & release)
	 *	up  != 0
	 */
    static fmat_4x4_t lookAt(const fvec3_t& eye, const fvec3_t& center, const fvec3_t& up)
	{
		fvec3_t f = center - eye;
		fvec3_t u = up;

		f.normalize();
		u.normalize();

		fvec3_t s = f.cross(u);
		s.normalize();

		u = s.cross(f);

        return
            fmat_4x4_t
            (
             s[kX],  s[kY],  s[kZ], 0.0f,
             u[kX],  u[kY],  u[kZ], 0.0f,
            -f[kX], -f[kY], -f[kZ], 0.0f,
              0.0f,   0.0f,   0.0f, 1.0f
            )
            *
            fmat_4x4_t::translation(-eye);
	}

	/**
	 * @brief
	 *	Get an orthographic projection (~ camera space -> normalized device coordinate space) matrix.
	 * @param left, right
	 *	the coordinates of the left and right vertical clipping planes
	 * @param bottom, top
	 *	the coordinates of the bottom and top horizontal clipping planes
	 * @param zNear, zFar
	 *	the distance to the nearer and farther depth clipping planes.
	 *  These values are negative if the plane is to be behind the viewer.
     * @return
     *  the matrix
	 * @remark
	 *	The orthographic projection matrix is
	 *	\f[
	 *	\left[\begin{matrix}
	 *	\frac{2}{right-left} & 0                    &   0                    & t_x \\
	 *	0                    & \frac{2}{top-bottom} &   0                    & t_y \\
	 *	0                    & 0                    &  \frac{-2}{zFar-zNear} & t_z \\
	 *  0                    & 0                    &   0                    & 1 \\
	 *	\end{matrix}\right]
	 *	\f]
	 *	where \f$t_x = -\frac{right+left}{right-left}\f$,
	 *        \f$t_y = -\frac{top+bottom}{top-bottom}\f$,
	 *        \f$t_z = -\frac{zFar+zNear}{zFar-zNear}\f$.
	 */
    static fmat_4x4_t ortho(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar)
	{
		float dx = right - left, dy = top - bottom, dz = zFar - zNear;
		EGOBOO_ASSERT(dx != 0.0f && dy != 0.0f && dz != 0.0f);
		float tx = -(right + left) / dx, ty = -(top + bottom) / dy, tz = -(zFar + zNear) / (dz);

        return
            fmat_4x4_t
            (
                2.0f / dx, 0.0f,     0.0f,    tx,
                0.0f,      2.0f/dy,  0.0f,    ty,
                0.0f,      0.0f,    -2.0f/dz, tz,
                0.0f,      0.0f,     0.0f,    1.0f
            );
	}

	/**
	 * @brief
	 *	Get a perspective projection (~ view space -> normalized device coordinate space) matrix.
	 * @param fovy
	 *	the field of view angle, in degrees, in the y direction
	 * @param aspect
	 *	the aspect ratio in the x direction
	 * @param zNear
	 *	the distance of the viewer to the near clipping plane
	 * @param zFrar
	 *	the distance of the viewer to the far clipping plane
     * @return
     *  the matrix
	 * @pre
	 *	@a zNear as well as @a zFar must be positive, <tt>zNear - zFar</tt> must not be @a 0,
	 *	@a aspect must not be @a 0.
	 * @remark
	 *	The aspect ratio specifies the field of view in the x direction and is the ratio of the x (width) / y (height).
	 * @remark
	 *	The perspective projection matrix is
	 *	\f[
	 *	\left[\begin{matrix}
	 *	\frac{f}{aspect} & 0 &  0                                     & 0 \\
	 *	0                & f &  0                                     & 0 \\
	 *	0                & 0 &  \frac{(zFar + zNear)}{(zNear - zFar)} & \frac{(2 * zFar * zNear)}{(zNear - zFar)} \\
	 *  0                & 0 & -1                                     & 1 \\
	 *	\end{matrix}\right]
	 *	\f]
	 *	where \f$f = cot(0.5 fovy)\f$.
	 */
    static fmat_4x4_t perspective(const float fovy, const float aspect, const float zNear, const float zFar)
    {
        EGOBOO_ASSERT(aspect != 0.0f);
        EGOBOO_ASSERT(zFar > 0.0f && zNear > 0.0f);
        EGOBOO_ASSERT((zNear - zFar) != 0.0f);

        float tan = std::tan(Ego::Math::degToRad(fovy) * 0.5f);
        EGOBOO_ASSERT(tan != 0.0f);
        float f = 1 / tan;

        return
            fmat_4x4_t
            (
                f / aspect, 0.0f, 0.0f,                            0.0f,
                0.0f,       f,    0.0f,                            0.0f,
                0.0f,       0.0f, (zFar + zNear) / (zNear - zFar), (2.0f * zFar * zNear) / (zNear - zFar),
                0.0f,       0.0f, -1.0f,                           1.0f
            );
    }

	/**
	 * @brief
	 *	Assign this matrix the values of a translation matrix.
	 * @param t
	 *	the translation vector
	 * @remark
	 *	The \f$4 \times 4\f$ translation matrix for the translation vector $\left(t_x,t_y,t_z\right)$ is defined as
	 *	\f[
	 *	\left[\begin{matrix}
	 *	1 & 0 & 0 & t_x \\
	 *	0 & 1 & 0 & t_y \\
	 *	0 & 0 & 1 & t_z \\
	 *	0 & 0 & 0 & 1   \\
	 *	\end{matrix}\right]
	 *	\f]
	 */
	static fmat_4x4_t translation(const fvec3_t& t)
	{
        return
            fmat_4x4_t
            (
                1, 0, 0, t[kX],
                0, 1, 0, t[kY],
                0, 0, 1, t[kZ],
                0, 0, 0,   1
            );
	}

	/**
	 * @brief
	 *	Get a matrix representing an anticlockwise rotation around the x-axis.
	 * @param a
	 *	the angle of rotation, in Radians
     * @return
     *  the matrix
	 * @remark
	 *	\f[
	 *	\left[\begin{matrix}
	 *	1 & 0 &  0 & 0 \\
	 *	0 & c & -s & 0 \\
	 *	0 & s &  c & 0 \\
	 *	0 & 0 &  0 & 1 \\
	 *	\end{matrix}\right]
	 *	\f]
	 *	where \f$c=\cos(a)\f$ and \f$s=\sin(a)\f$.
     * @todo
     *  Angles should be in degrees.
	 */
	static fmat_4x4_t rotationX(const float a)
	{
		float c = std::cos(a), s = std::sin(a);
        return
            fmat_4x4_t
            (
                1,  0,  0, 0,
                0, +c, -s, 0,
                0, +s,  c, 0,
                0,  0,  0, 1
            );
	}

	/**
	 * @brief
	 *	Get a matrix representing a anticlockwise rotation around the y-axis.
	 * @param a
	 *	the angle of rotation, in Radians
     * @return
     *  the matrix
	 * @remark
	 *	\f[
	 *	\left[\begin{matrix}
	 *	 c & 0 & s & 0 \\
	 *	 0 & 1 & 0 & 0 \\
	 *	-s & 0 & c & 0 \\
	 *	 0 & 0 & 0 & 1 \\
	 *	\end{matrix}\right]
	 *	\f]
	 *	where \f$c=\cos(a)\f$ and \f$s=\sin(a)\f$.
     * @todo
     *  Angles should be in degree.
	 */
	static fmat_4x4_t rotationY(const float a)
	{
        float c = std::cos(a), s = std::sin(a);
        return
            fmat_4x4_t
            (
                +c, 0, +s, 0,
                 0, 1,  0, 0,
                -s, 0, +c, 0,
                 0, 0,  0, 1
            );
	}

	/**
	 * @brief
	 *	Get a matrix representing an anticlockwise rotation about the z-axis.
	 * @param a
	 *	the angle of rotation, in Radians
     * @return
     *  the matrix
     * @remark
     *	\f[
     *	\left[\begin{matrix}
     *	c & -s & 0 & 0 \\
     *	s &  c & 0 & 0 \\
     *	0 &  0 & 1 & 0 \\
     *	0 &  0 & 0 & 1 \\
     *	\end{matrix}\right]
     *	\f]
     *	where \f$c=cos(a)\f$ and \f$s=sin(a)\f$.
     * @todo
     *  Angles should be in degrees.
	 */
    static fmat_4x4_t rotationZ(const float a)
	{
        float c = std::cos(a), s = std::sin(a);
        return
            fmat_4x4_t
            (
                +c, -s, 0, 0,
                +s, +c, 0, 0,
                 0,  0, 1, 0,
                 0,  0, 0, 1
            );
	}


    /**
     * @brief
     *  Get a rotation matrix representing a counter-clockwise rotation around an axis.
     * @param axis
     *  the rotation axis
     * @param angle
     *  the rotation angle
     * @return
     *  the rotation matrix
     * @throw std::invalid_argument
     *  if the rotation axis is the zero vector
     * @remark
     *  Given an axis of rotation represented by the unit vector \f$\hat{\vec{r}}=(k_x,k_y,k_z,1)\f$ and an angle
     *  \f$\theta\f$ in degrees, we shall obtain a \f$4 \times 4\f$ matrix \f$\mathcal{T}\f$ called the Rodrigues rotation
     *  matrix \f$R\f$ for the axis of rotation \f$\hat{\vec{k}}\f$ and the angle \f$\theta\f$. This matrix has
     *  represents the transformation of rotating counter-clockwise by \f$\theta\f$ degrees about the axis
     *  \f$\hat{k}\f$ i.e. for any point \f$\vec{v}=(v_x,v_y,v_z,1)\f$
     *  \[
     *  \vec{v}' = \mathbf{L} \vec{v}
     *  \]
     *  is the counter-clockwise rotation of \f$\vec{v}\f$ by \f$\theta\f$ degrees about the axis \f$\hat{\vec{r}}\f$.
     *  </br>
     *  The derivation of that matrix is provided here for reference, the geometric reasoning is omitted.
     *  </br>
     *  To compute the rotated point \f$\vec{v}'\f$, we begin by splutting \f$\vec{v}\f$ into a part \f$\vec{v}_{\parallel}\f$
     *  parallel to \f$\hat{\vec{r}}\f$ and into a part \f$\vec{v}_{\perp}\f$ perpendicular to \f$\hat{\vec{r}}\f$ which lies
     *  on the plane of rotation. Recall from fvec3_t::decompose(const fvec3_t&,const fvec3_t&,fvec3_t&,fvec3_t&) that the
     *  parallel part is the projection of \f$\vec{v}\f$ on \f$\hat{\vec{r}}\f$, the perpendicular part is the rejection
     *  \f$\vec{v}_{\perp}\f$ of \f$\vec{v}\f$ from \f$\hat{\vec{r}}\f$:
     *  \f{align*}{
     *  \vec{v}_{\parallel} = \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\\
     *  \vec{v}_{\perp} = \vec{v} - \vec{v}_{\parallel}
     *  \f}
     *  To compute the effect of the rotation, a two-dimensional basis on the plane of rotation is required.
     *  \f$\vec{v}_{\perp}\f$ is used as the first basis vector. As second basis vector perpendicular to
     *  \f$\vec{v}_{\perp}\f$ is required. One can use the vector
     *  \f{align*}{
     *  \vec{w} = \hat{\vec{r}} \times \vec{v}_{\perp} = \hat{\vec{r}} \times \vec{v}
     *  \f}
     *  which is perpendicular to and has the same length as \f$\vec{v}_{\perp}\f$ as
     *  shown in fvec3_t::decompose(const fvec3_t&,const fvec3_t&,fvec3_t&,fvec3_t&).
     *  </br>
     *  If in \f$\mathbb{R}^2\f$ one rotates the vector \f$\vec{i}=(1,0)\f$ by \f$\theta\f$
     *  degrees in the plane of rotation spanned by the standard basis \f$\vec{i}=(1,0)\f$,
     *  \f$\vec{j}=(0,1)\f$ for \f$\mathbb{R}^2\f$, the result is the vector
     *  \f{align*}{
     *  \vec{i}' = \cos(\theta)\vec{i} + \sin(\theta) \vec{j}
     *  \f}
     *  If \f$\vec{v}_{\perp}\f$ and \f$\vec{w}\f$ are used as the standard basis and
     *  \f$\vec{v}_{\perp}\f$ is rotated in the plane of rotation spanned by the basis
     *  \f$\vec{v}_{\perp}\f$ and \f$\vec{w}\f$, the result of the rotation is given in
     *  a similar manner:
     *  \f{align*}{
     *  \vec{v}'_{\perp} = \cos(\theta)\vec{v}_{\perp} + \sin(\theta)\vec{w}
     *  \f}
     *  </br>
     *  As \f$\vec{v}_{\parallel}\f$ is completely unaffected by the rotation about \f$\hat{\vec{r}}\f$
     *  the final result of the anti-clockwise rotation of \f$\vec{v}\f$ by \f$\theta\f$ degree around
     *  \f$\hat{\vec{r}}\f$ is given by
     *  \f[
     *  \vec{v}' = \vec{v}_{\parallel} + \vec{v}'_{\perp}
     *  \f]
     *  </br>
     *  As
     *  \f{align*}{
     *  \vec{v}_{\parallel} =& \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\\
     *  \vec{v}_{\perp}     =& \vec{v} - \vec{v}_{\parallel} = \vec{v}_{\parallel}\\
     *                      =& \vec{v} - \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\\
     *  \vec{w}             =& \hat{\vec{r}} \times \vec{v}\\
     *  \vec{v}'_{\perp}    =& \cos(\theta)\vec{v}_{\perp} + \sin(\theta)\vec{w}\\
     *                      =& \cos(\theta)\left(\vec{v} - \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\right) + \sin(\theta)\left(\hat{\vec{r}} \times \vec{v}\right)
     *  \f}
     *  the above expression can be rewritten and simplified
     *  \f{align*}{
     *  \vec{v}' =& \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}} + \cos(\theta)\left(\vec{v} - \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\right) + \sin(\theta)\left(\hat{\vec{r}} \times \vec{v}\right)\\
     *           =& \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}} + \cos(\theta)\vec{v} - \cos(\theta)\left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}} + \sin(\theta)\left(\hat{\vec{r}} \times \vec{v}\right)\\
     *           =& \cos(\theta)\vec{v} + \left[\left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}} - \cos(\theta)\left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\right] + \sin(\theta)\left(\hat{\vec{r}} \times \vec{v}\right)\\
     *           =& \cos(\theta)\vec{v} + (1- \cos(\theta))\left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}} + \sin(\theta)\left(\hat{\vec{r}} \times \vec{v}\right)
     *  \f}
     *  which is known as the Rodrigues rotation formula.
     * @remark
     *  To obtain the Rodrigues rotation formula in a matrix form, the projection \f$\left(\vec{v} \cdot \hat{\vec{r}}
     *  \right)\vec{r}\f$ can be replaced by the tensor product \f$(\hat{\vec{r}} \otimes \hat{\vec{r}})\vec{v}\f$
     *  and the cross product \f$\hat{\vec{r}} \times \vec{v}\f$ can be replaced by a multiplication with a cross product
     *   matrix \f$\mathbf{R} \vec{v}\f$ which gives
     *  \f{align*}{
     *  \vec{v}' =& \cos(\theta)\vec{v} + (1- \cos(\theta))\left(\hat{\vec{r}} \otimes \hat{\vec{r}}\right)\vec{v} + \sin(\theta)\left(\mathbf{R}\vec{v}\right)\\
     *           =& \left[\cos(\theta)\mathbf{I} + (1 - \cos(\theta))\left(\hat{\vec{r}} \otimes \hat{\vec{r}}\right) + \sin(\theta)\mathbf{R}\right]\vec{v}
     *  \f}
     *  with
     *  \f[
     *  \mathbf{R} =
     *  \left[\begin{matrix}
     *   0                  & -\hat{\vec{r}}_3 &  \hat{\vec{r}}_2 \\
     *   \hat{\vec{r}}_3    & 0                & -\hat{\vec{r}}_1 \\
     *  -\hat{\vec{r}}_2    & \hat{\vec{r}}_1  & 0
     *  \end{matrix}\right]
     *  \f]
     *  and
     *  \f[
     *  \hat{\vec{r}} \otimes \hat{\vec{r}}
     *  =
     *  \left[\begin{matrix}
     *   \hat{\vec{r}}_1 \\
     *   \hat{\vec{r}}_2 \\
     *   \hat{\vec{r}}_3
     *  \end{matrix}\right]
     *  \otimes
     *  \left[\begin{matrix}
     *   \hat{\vec{r}}_1 & \hat{\vec{r}}_2 & \hat{\vec{r}}_3
     *  \end{matrix}\right]
     *  =
     *  \left[\begin{matrix}
     *  \hat{\vec{r}}^2_1               & \hat{\vec{r}}_1 \hat{\vec{r}}_2 & \hat{\vec{r}}_1 \hat{\vec{r}}_3 \\
     *  \hat{\vec{r}}_1 \hat{\vec{r}}_2 & \hat{\vec{r}}^2_2               & \hat{\vec{r}}_2 \hat{\vec{r}}_3 \\
     *  \hat{\vec{r}}_1 \hat{\vec{r}}_3 & \hat{\vec{r}}_2 \hat{\vec{r}}_3 & \hat{\vec{r}}^3_3
     *  \end{matrix}\right]
     *  \f]
     *  The matrix
     *  \f[
     *  \mathcal{T} = \cos(\theta)\mathbf{I} + (1 - \cos(\theta))\left(\hat{\vec{r}} \otimes \hat{\vec{r}}\right) + \sin(\theta)\mathbf{R}
     *  \f]
     *  is known as the Rodrigues rotation matrix.
     * @remark
     *  To compute the matrix \f$\mathbf{T}\f$ efficiently, its formula
     *  \f[
     *  \mathcal{T} = \cos(\theta)\mathbf{I} + (1 - \cos(\theta))\left(\hat{\vec{r}} \otimes \hat{\vec{r}}\right) + \sin(\theta)\mathbf{R}
     *  \f]
     *  is expanded. Let \f$s=\sin(\theta)\f$, \f$c = \cos(\theta)\f$ and \f$t = 1 - \cos(\theta)\f$
     *  \f{align*}{
     *    \mathcal{T}
     * =& c \mathbf{I} + t \left(\hat{\vec{r}} \otimes \hat{\vec{r}}\right) + s \mathbf{R}\\
     * =&
     *    c
     *    \left[\begin{matrix}
     *     1 & 0 & 0 \\
     *     0 & 1 & 0 \\
     *     0 & 0 & 1
     *     \end{matrix}\right]
     *     +
     *     t
     *     \left[\begin{matrix}
     *     \hat{\vec{r}}^2_1               & \hat{\vec{r}}_1 \hat{\vec{r}}_2 & \hat{\vec{r}}_1 \hat{\vec{r}}_3 \\
     *     \hat{\vec{r}}_1 \hat{\vec{r}}_2 & \hat{\vec{r}}^2_2               & \hat{\vec{r}}_2 \hat{\vec{r}}_3 \\
     *     \hat{\vec{r}}_1 \hat{\vec{r}}_3 & \hat{\vec{r}}_2 \hat{\vec{r}}_3 & \hat{\vec{r}}^3_3
     *     \end{matrix}\right]
     *     +
     *     s
     *     \left[\begin{matrix}
     *     0                  & -\hat{\vec{r}}_3 &  \hat{\vec{r}}_2 \\
     *     \hat{\vec{r}}_3    & 0                & -\hat{\vec{r}}_1 \\
     *     -\hat{\vec{r}}_2    & \hat{\vec{r}}_1  & 0
     *     \end{matrix}\right]\\
     * =&
     *    \left[\begin{matrix}
     *     c & 0 & 0 \\
     *     0 & c & 0 \\
     *     0 & 0 & c
     *     \end{matrix}\right]
     *     +
     *     \left[\begin{matrix}
     *     t\hat{\vec{r}}^2_1               & t\hat{\vec{r}}_1 \hat{\vec{r}}_2 & t\hat{\vec{r}}_1 \hat{\vec{r}}_3 \\
     *     t\hat{\vec{r}}_1 \hat{\vec{r}}_2 & t\hat{\vec{r}}^2_2               & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 \\
     *     t\hat{\vec{r}}_1 \hat{\vec{r}}_3 & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 & t\hat{\vec{r}}^3_3
     *     \end{matrix}\right]
     *     +
     *     \left[\begin{matrix}
     *     0                 & -s\hat{\vec{r}}_3 &  s\hat{\vec{r}}_2 \\
     *     s\hat{\vec{r}}_3  & 0                 & -s\hat{\vec{r}}_1 \\
     *     -s\hat{\vec{r}}_2 & s\hat{\vec{r}}_1  & 0
     *     \end{matrix}\right]\\
     * =&
     *     \left[\begin{matrix}
     *     t\hat{\vec{r}}^2_1 + c                              & t\hat{\vec{r}}_1 \hat{\vec{r}}_2 - s\hat{\vec{r}}_3 & t\hat{\vec{r}}_1 \hat{\vec{r}}_3 + s\hat{\vec{r}}_2\\
     *     t\hat{\vec{r}}_1 \hat{\vec{r}}_2 + s\hat{\vec{r}}_3 & t\hat{\vec{r}}^2_2 + c                              & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 - s\hat{\vec{r}}_1  \\
     *     t\hat{\vec{r}}_1 \hat{\vec{r}}_3 - s\hat{\vec{r}}_2 & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 + s\hat{\vec{r}}_1 & t\hat{\vec{r}}^3_3 + c
     *     \end{matrix}\right]
     *  \f}
     * @note
     *  In the matrix
     *  \f[
     *  \mathcal{T} =
     *  \left[\begin{matrix}
     *  t\hat{\vec{r}}^2_1 + c                              & t\hat{\vec{r}}_1 \hat{\vec{r}}_2 - s\hat{\vec{r}}_3 & t\hat{\vec{r}}_1 \hat{\vec{r}}_3 + s\hat{\vec{r}}_2\\
     *  t\hat{\vec{r}}_1 \hat{\vec{r}}_2 + s\hat{\vec{r}}_3 & t\hat{\vec{r}}^2_2 + c                              & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 - s\hat{\vec{r}}_1  \\
     *  t\hat{\vec{r}}_1 \hat{\vec{r}}_3 - s\hat{\vec{r}}_2 & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 + s\hat{\vec{r}}_1 & t\hat{\vec{r}}^3_3 + c
     *  \end{matrix}\right]\\
     *  s=\sin(\theta), c = \cos(\theta), t = 1 - \cos(\theta)
     *  \f]
     *  common subexpressions exist which can be eliminated by precomputation:
     *  \f{align*}{
     *  \mathcal{T} =
     *  \left[\begin{matrix}
     *  t_1 \hat{\vec{r}}_1 + c & t_12 - s_3              & t_13 + s_2 \\
     *  t_12 + s_3              & t_2 \hat{\vec{r}}_2 + c & t_23 - s_1 \\
     *  t_13 - s_2              & t_23 + s_1              & t_3 \hat{\vec{r}}_3 + c
     *  \end{matrix}\right]\\
     *  s=\sin(\theta), c = \cos(\theta), t = 1 - \cos(\theta),\\
     *  t_1 = t\hat{\vec{r}}_1, t_2 = t\hat{\vec{r}}_2, t_3 = t\hat{\vec{r}}_3,\\
     *  t_12 = t_1 \hat{\vec{r}}_2, t_13 = t_1 \hat{\vec{r}}_3, t_23 = t_2 \hat{\vec{r}}_3\\
     *  s_1 = s\hat{\vec{r}}_1, s_2 = s\hat{\vec{r}}_2, s_3 = s\hat{\vec{r}}_3
     *  \f}
     *  This implementation performs this form of elimination of common subexpressions.
     */
    static fmat_4x4_t rotation(const fvec3_t& axis, float angle)
    {
        float a = Ego::Math::degToRad(angle);
        float c = std::cos(a), s = std::sin(a);
        float t = 1.0f - c;
        float x = axis[kX], y = axis[kY], z = axis[kZ];
        float xx = x*x, yy = y*y, zz = z*z;
        float l = std::sqrt(xx+yy+zz);
        if (l == 0.0f)
        {
            throw std::invalid_argument("axis vector is zero vector");
        }
        x /= l; y /= l; z /= l;
        float sx = s * x, sy = s * y, sz = s * z,
              tx = t * x, ty = t * y, tz = t * z;
        float txy = tx * y, txz = tx * z, tyz = ty * z;

        return fmat_4x4_t
            (
            tx * x + c, txy - sz,   txz + sy,   0,
            txy + sz,   ty * y + c, tyz - sx,   0,
            txz - sy,   tyz + sx,   tz * z + c, 0,
            0,          0,          0,          1
            );
    }

	/**
	 * @brief
	 *	Get a scaling scaling matrix.
	 * @param self
	 *	this matrix
	 * @param s
	 *	a scaling vector
     * @return
     *  the matrix
	 * @remark
	 *	\f[
	 *	\left[\begin{matrix}
	 *	s_x &  0   & 0   & 0 \\
	 *	0   &  s_y & 0   & 0 \\
	 *	0   &  0   & s_z & 0 \\
	 *	0   &  0   & 0   & 1 \\
	 *	\end{matrix}\right]
	 *	\f]
	 */
    static fmat_4x4_t scaling(const fvec3_t& s)
	{
        return
            fmat_4x4_t
            (
                s[kX],     0,     0, 0,
                    0, s[kY],     0, 0,
                    0,     0, s[kZ], 0,
                    0,     0,     0, 1
            );
	}

	/**
	 * @brief
	 *	Transform vector.
	 * @param m
	 *	the transformation matrix
	 * @param source
	 *	the source vector
	 * @param [out] target
	 *	a vector which is assigned the transformation result
	 * @remark
	 *	\f[
	 *	\left[\begin{matrix}
	 *	m_{0,0} & m_{0,1} & m_{0,2} & m_{0,3} \\
	 *	m_{1,0} & m_{1,1} & m_{1,2} & m_{1,3} \\
	 *	m_{2,0} & m_{2,1} & m_{2,2} & m_{2,3} \\
	 *	m_{3,0} & m_{3,1} & m_{3,2} & m_{3,3} \\
	 *	\end{matrix}\right]
	 *	\cdot
	 * 	\left[\begin{matrix}
	 * 	v_{0} \\
	 *	v_{1} \\
	 *	v_{2} \\
	 *	v_{3} \\
	 *	\end{matrix}\right]
	 *	=
	 *	\left[\begin{matrix}
	 *	m_{0,0} \cdot v_{0} + m_{0,1} \cdot v_1 + m_{0,2} \cdot v_2 + m_{0,3} \cdot v_3 \\
	 *	m_{1,0} \cdot v_{0} + m_{1,1} \cdot v_1 + m_{1,2} \cdot v_2 + m_{1,3} \cdot v_3  \\
	 *	m_{2,0} \cdot v_{0} + m_{2,1} \cdot v_1 + m_{2,2} \cdot v_2 + m_{2,3} \cdot v_3  \\
	 *	m_{3,0} \cdot v_{0} + m_{3,1} \cdot v_1 + m_{3,2} \cdot v_2 + m_{3,3} \cdot v_3  \\
	 *	\end{matrix}\right]
	 *	\f]
	 */
	void transform(const fvec4_t& source, fvec4_t& target) const
	{
        target[kX] = (*this)(0, 0) * source[kX] + (*this)(0, 1) * source[kY] + (*this)(0, 2) * source[kZ] + (*this)(0, 3) * source[kW];
        target[kY] = (*this)(1, 0) * source[kX] + (*this)(1, 1) * source[kY] + (*this)(1, 2) * source[kZ] + (*this)(1, 3) * source[kW];
        target[kZ] = (*this)(2, 0) * source[kX] + (*this)(2, 1) * source[kY] + (*this)(2, 2) * source[kZ] + (*this)(2, 3) * source[kW];
        target[kW] = (*this)(3, 0) * source[kX] + (*this)(3, 1) * source[kY] + (*this)(3, 2) * source[kZ] + (*this)(3, 3) * source[kW];
	}

	/**
	 * @brief
	 *	Transform vectors.
	 * @param m
	 *	the transformation matrix
	 * @param sources
	 *	the source vectors
	 * @param [out] targets
	 *	an array of vectors which are assigned the transformation results
	 * @see
	 *	fmat_4x4_t::transform(const fmat_4x4_t& const fvec4_t&, fvec4_t&)
	 */
	void transform(const fvec4_t sources[], fvec4_t targets[], const size_t size) const
	{
		const fvec4_t *source = sources;
		fvec4_t *target = targets;
		for (size_t index = 0; index < size; index++)
		{
			transform(*source, *target);
			source++;
			target++;
		}
	}

};

void mat_FourPoints(fmat_4x4_t& DST, const fvec3_t& ori, const fvec3_t& wid, const fvec3_t& frw, const fvec3_t& up, const float scale);

/**
 * @remark
 *  The initial "up" vector of any Egoboo object is \f$d=(0,0,1)\f$.
 *  Premultiplying this vector with a 3x3 matrix
 *  \f{align*}{
 *  \left[\begin{matrix}
 *  m_{0,0} & m_{0,1} & m_{0,2} \\
 *  m_{1,0} & m_{1,1} & m_{1,2} \\
 *  m_{2,0} & m_{2,1} & m_{2,2} \\
 *  \end{matrix}\right]
 *  \cdot
 *  \left[\begin{matrix}
 *  d_0\\
 *  d_1\\
 *  d_2\\
 *  \end{matrix}\right]
 *  =
 *  \left[\begin{matrix}
 *  m_{0,0} d_0 + m_{0,1} d_1 + m_{0,2} d_2\\
 *  m_{1,0} d_0 + m_{1,1} d_1 + m_{1,2} d_2\\
 *  m_{2,0} d_0 + m_{2,1} d_1 + m_{2,2} d_2
 *  \end{matrix}\right]
 *  =
 *  \left[\begin{matrix}
 *  m_{0,2}\\
 *  m_{1,2}\\
 *  m_{2,2}
 *  \end{matrix}\right]
 *  This is odd, but remember that Egoboo was a 2D game.
 */
fvec3_t mat_getChrUp(const fmat_4x4_t& mat);

/**
 * @remark
 *  The initial "forward" vector of an Egoboo object is \f$d=(-1,0,0)\f$.
 *  Premultiplying this vector with a 3x3 matrix
 *  \f{align*}{
 *  \left[\begin{matrix}
 *  m_{0,0} & m_{0,1} & m_{0,2}\\
 *  m_{1,0} & m_{1,1} & m_{1,2}\\
 *  m_{2,0} & m_{2,1} & m_{2,2}\\
 *  \end{matrix}\right]
 *  \cdot
 *  \left[\begin{matrix}
 *  d_0\\
 *  d_1\\
 *  d_2\\
 *  \end{matrix}\right]
 *  =
 *  \left[\begin{matrix}
 *  m_{0,0} d_0 + m_{0,1} d_1 + m_{0,2} d_2\\
 *  m_{1,0} d_0 + m_{1,1} d_1 + m_{1,2} d_2\\
 *  m_{2,0} d_0 + m_{2,1} d_1 + m_{2,2} d_2
 *  \end{matrix}\right]
 *  =
 *  \left[\begin{matrix}
 *  -m_{0,0}\\
 *  -m_{1,0}\\
 *  -m_{2,0}
 *  \end{matrix}\right]
 *  \f}
 *  This is odd, but remember that Egoboo was a 2D game.
 */
fvec3_t mat_getChrForward(const fmat_4x4_t& mat);

/**
 * @remark
 *  The initial "right" vector of an Egoboo object is \f$d=(0,1,0)\f$.
 *  Premultiplying this vector with a 3x3 matrix
 *  \f{align*}{
 *  \left[\begin{matrix}
 *  m_{0,0} & m_{0,1} & m_{0,2}\\
 *  m_{1,0} & m_{1,1} & m_{1,2}\\
 *  m_{2,0} & m_{2,1} & m_{2,2}\\
 *  \end{matrix}\right]
 *  \cdot
 *  \left[\begin{matrix}
 *  d_0\\
 *  d_1\\
 *  d_2\\
 *  \end{matrix}\right]
 *  =
 *  \left[\begin{matrix}
 *  m_{0,0} d_0 + m_{0,1} d_1 + m_{0,2} d_2\\
 *  m_{1,0} d_0 + m_{1,1} d_1 + m_{1,2} d_2\\
 *  m_{2,0} d_0 + m_{2,1} d_1 + m_{2,2} d_2
 *  \end{matrix}\right]
 *  =
 *  \left[\begin{matrix}
 *  m_{0,1}\\
 *  m_{1,1}\\
 *  m_{2,1}
 *  \end{matrix}\right]
 *  \f}
 *  This is odd, but remember that Egoboo was a 2D game.
 */
fvec3_t mat_getChrRight(const fmat_4x4_t& mat);


bool mat_getCamUp(const fmat_4x4_t& mat, fvec3_t& up);
bool mat_getCamRight(const fmat_4x4_t& mat, fvec3_t& right);
bool mat_getCamForward(const fmat_4x4_t& mat, fvec3_t& forward);


fvec3_t mat_getTranslate(const fmat_4x4_t& mat);

float *mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(fmat_4x4_base_t mat, const fvec3_t& scale, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const fvec3_t& translate);
float *mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(fmat_4x4_base_t mat, const fvec3_t& scale, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const fvec3_t& translate);


/**
 * @brief
 *	Dump a textual representation of a matrix to standard output.
 * @param a
 *	the matrix
 */
void dump_matrix(const fmat_4x4_t& a);

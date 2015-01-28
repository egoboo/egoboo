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

/// @file egolib/matrix.h
/// @details matrices
#pragma once

#include "egolib/vec.h"

/// @brief Egoboo currently uses column-major format. This will change to column major.
#define fmat_4x4_layout_RowMajor (1)
#define fmat_4x4_layout_ColumnMajor (2)
#define fmat_4x4_layout fmat_4x4_layout_ColumnMajor

typedef float fmat_4x4_base_t[16];      ///< the basic 4x4 single precision floating point ("float")  matrix type
#if 0
typedef double dmat_4x4_base_t[16];      ///< the basic 4x4 double precision floating point ("double") matrix type
#endif


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
	 *	The \f$4 \times 4\f$ identity/multiplicative neutral matrix is defined as
	 *	\f[
	 *		\left[\begin{matrix}
	 *		1 & 0 & 0 & 0 \\
	 *		0 & 1 & 0 & 0 \\
	 *		0 & 0 & 1 & 0 \\
	 *		0 & 0 & 0 & 1
	 *		\end{matrix}\right]
	 * \f]
	 */
	static const fmat_4x4_t identity;

	/**
	 * @brief
	 *	Set this matrix to the zero matrix.
	 * @remark
	 *	The \f$4 \times 4\f$ zero matrix is defined as
	 *	\f[
	 *	\left[\begin{matrix}
	 *	0 & 0 & 0 & 0 \\
	 *	0 & 0 & 0 & 0 \\
	 *	0 & 0 & 0 & 0 \\
	 *	0 & 0 & 0 & 0
	 *	\end{matrix}\right]
	 *	\f]
	 */
	static const fmat_4x4_t zero;

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
		v2[0][0] = m00; v2[0][1] = m01; v2[0][2] = m02; v2[0][3] = m03;
		v2[1][0] = m10; v2[1][1] = m11; v2[1][2] = m12; v2[1][3] = m13;
		v2[2][0] = m20; v2[2][1] = m21; v2[2][2] = m22; v2[2][3] = m23;
		v2[3][0] = m30; v2[3][1] = m31; v2[3][2] = m32; v2[3][3] = m33;
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
	fmat_4x4_t multiply(const fmat_4x4_t& other)
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
	fmat_4x4_t operator*(const fmat_4x4_t& other)
	{
		return multiply(other);
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
	 *	Set this matrix to the zero matrix.
	 * @see fmat_4x4_t::zero
	 */
	void setZero()
	{
		(*this) = fmat_4x4_t::zero;
	}

	/**
	 * @brief
	 *	Set this matrix to the identity/multiplicative neutral matrix.
	 * @see fmat_4x4_t::identity
	 */
	void setIdentity()
	{
		(*this) = fmat_4x4_t::identity;
	}

	/**
	 * @brief
	 *	Assign this matrix the values of a viewing transformation (~ world space -> camera space) matrix.
	 * @param eye
	 *	the position of the eye point
	 * @param center
	 *	the position of the reference point
	 * @param up
	 *	the direction of the up vector
	 * @pre
	 *	eye != center (debug & release)
	 *	up  != 0
	 */
	void setLookAt(const fvec3_t& eye, const fvec3_t& center, const fvec3_t& up)
	{
		fvec3_t f = center - eye;
		fvec3_t u = up;

		f.normalize();
		u.normalize();

		fvec3_t s = f.cross(u);
		s.normalize();

		u = s.cross(f);

		/* Row 0. */
		(*this)(0, 0) = s.x;
		(*this)(0, 1) = s.y;
		(*this)(0, 2) = s.z;
		(*this)(0, 2) = 0.0f;

		/* Row 1. */
		(*this)(1, 0) = u.x;
		(*this)(1, 1) = u.y;
		(*this)(1, 2) = u.z;
		(*this)(1, 3) = 0.0f;

		/* Row 2. */
		(*this)(2, 0) = -f.x;
		(*this)(2, 1) = -f.y;
		(*this)(2, 2) = -f.z;
		(*this)(2, 3) = 0.0f;

		/* Row 3. */
		(*this)(3, 0) = 0.0f;
		(*this)(3, 1) = 0.0f;
		(*this)(3, 2) = 0.0f;
		(*this)(3, 3) = 1.0f;
	}

	/**
	 * @brief
	 *	Assign this matrix the values of a orthographic projection (~ camera space -> screen space) matrix.
	 * @param left, right
	 *	the coordinates of the left and right vertical clipping planes
	 * @param bottom, top
	 *	the coordinates of the bottom and top horizontal clipping planes
	 * @param zNear, zFar
	 *	the distance to the nearer and farther depth clipping planes.
	 *  These values are negative if the plane is to be behind the viewer.
	 * @remark
	 *	The orthographic projection matrix is
	 *	\f[
	 *	\left[\begin{matrix}
	 *	\frac{2}{right-left} & 0                    &  0                    & t_x \\
	 *	0                    & \frac{2}{top-bottom} &  0                    & t_y \\
	 *	0                    & 0                    &  \frac{2}{zFar-zNear} & t_z \\
	 *  0                    & 0                    &  0                    & 1 \\
	 *	\end{matrix}\right]
	 *	\f]
	 *	where \f$t_x = -\frac{right+left}{right-left}\f$,
	 *        \f$t_y = -\frac{top+bottom}{top-bottom}\f$,
	 *        \f$t_z = -\frac{zFar+zNear}{zFar-zNear}\f$.
	 */
	void setOrtho(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar)
	{
		float dx = right - left, dy = top - bottom, dz = zFar - zNear;
		EGOBOO_ASSERT(dx != 0.0f && dy != 0.0f && dz != 0.0f);
		float tx = -(right + left) / dx, ty = -(top + bottom) / dy, tz = -(zFar + zNear) / (dz);

		(*this)(0, 0) = 2.0f * dx;
		(*this)(1, 0) = 0.0f;
		(*this)(2, 0) = 0.0f;
		(*this)(3, 0) = 0.0f;

		(*this)(0, 1) = 0.0f;
		(*this)(1, 1) = 2.0f * dy;
		(*this)(2, 1) = 0.0f;
		(*this)(3, 1) = 0.0f;

		(*this)(0, 2) = 0.0f;
		(*this)(1, 2) = 0.0f;
		(*this)(2, 2) = 2.0f * dz;
		(*this)(3, 2) = 1.0f;

		(*this)(0, 3) = tx;
		(*this)(1, 3) = ty;
		(*this)(2, 3) = tz;
		(*this)(3, 3) = 1.0f;
	}

	/**
	 * @brief
	 *	Assign this matrix the values of a perspective projection (~ camera space -> screen space) matrix.
	 * @param fovy
	 *	the field of view angle, in degrees, in the y direction
	 * @param aspect
	 *	the aspect ratio in the x direction
	 * @param zNear
	 *	the distance of the viewer to the near clipping plane
	 * @param zFar
	 *	the distance of the viewer to the far clipping plane
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
	void setPerspective(const float fovy, const float aspect, const float zNear, const float zFar)
	{
		EGOBOO_ASSERT(aspect != 0.0f);
		EGOBOO_ASSERT(zFar > 0.0f && zNear > 0.0f);
		EGOBOO_ASSERT((zNear - zFar) != 0.0f);

		float tan = std::tan(DEG_TO_RAD(fovy) * 0.5f);
		EGOBOO_ASSERT(tan != 0.0f);
		float f = 1 / tan;

		(*this)(0, 0) = f / aspect;
		(*this)(1, 0) = 0.0f;
		(*this)(2, 0) = 0.0f;
		(*this)(3, 0) = 0.0f;

		(*this)(0, 1) = 0.0f;
		(*this)(1, 1) = f;
		(*this)(2, 1) = 0.0f;
		(*this)(3, 1) = 0.0f;

		(*this)(0, 2) = 0.0f;
		(*this)(1, 2) = 0.0f;
		(*this)(2, 2) = (zFar + zNear) / (zNear - zFar);
		(*this)(3, 2) = -1;

		(*this)(0, 3) = 0.0f;
		(*this)(1, 3) = 0.0f;
		(*this)(2, 3) = (2.0f * zFar * zNear) / (zNear - zFar);
		(*this)(3, 3) = 0.0f;
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
	void setTranslation(const fvec3_t& t)
	{
		// Column 0.
		(*this)(0, 0) = 1.0f;
		(*this)(1, 0) = 0.0f;
		(*this)(2, 0) = 0.0f;
		(*this)(3, 0) = 0.0f;
		// Column 1.
		(*this)(0, 1) = 0.0f;
		(*this)(1, 1) = 1.0f;
		(*this)(2, 1) = 0.0f;
		(*this)(3, 1) = 0.0f;
		// Column 2.
		(*this)(0, 2) = 0.0f;
		(*this)(1, 2) = 0.0f;
		(*this)(2, 2) = 1.0f;
		(*this)(3, 2) = 0.0f;
		// Column 3.
		(*this)(0, 3) = t.x;
		(*this)(1, 3) = t.y;
		(*this)(2, 3) = t.z;
		(*this)(3, 3) = 1.0f;
	}

	/**
	 * @brief
	 *	Assign a matrix the values of a translation matrix.
	 * @param M
	 *	the matrix
	 * @param t
	 *	the translation vector
	 * @see
	 *	fmat_4x4_t::setTransation(const fvec3_t&)
	 */
	static void setTranslation(fmat_4x4_t& M, const fvec3_t& t)
	{
		M.setTranslation(t);
	}

	/**
	 * @brief
	 *	Assign this matrix the values of a rotation matrix for counter-clockwise rotation around the x-axis.
	 * @param a
	 *	the angle of rotation, in Radians
	 * @remark
	 *	\f[
	 *	\left[\begin{matrix}
	 *	1 & 0 &  0 & 0 \\
	 *	0 & c & -s & 0 \\
	 *	0 & s &  c & 0 \\
	 *	0 & 0 &  0 & 1 \\
	 *	\end{matrix}\right]
	 *	\f]
	 *	where \f$c=cos(a)\f$ and \f$s=sin(a)\f$.
	 */
	void setRotationX(const float a)
	{
		float c = COS(a), s = SIN(a);
		(*this) = fmat_4x4_t::identity;
		// 1st column.
		(*this)(1, 1) = +c;
		(*this)(2, 1) = +s;
		// 2nd column.
		(*this)(1, 2) = -s;
		(*this)(2, 2) = +c;
	}

	/**
	 * @brief
	 *	Assign a matrix the values of a rotation matrix for counter-clockwise rotation around the x-axis.
	 * @param M
	 *	the matrix
	 * @param a
	 *	the angle of rotation, in Radians
	 * @see
	 *	fmat_4x4_t::setRotationX(const float)
	 */
	static void setRotationX(fmat_4x4_t& M, const float a)
	{
		M.setRotationX(a);
	}

	/**
	 * @brief
	 *	Assign this matrix the values of a rotation matrix for counter-clockwise rotation around the y-axis.
	 * @param a
	 *	the angle of rotation, in Radians
	 * @remark
	 *	\f[
	 *	\left[\begin{matrix}
	 *	 c & 0 & s & 0 \\
	 *	 0 & 1 & 0 & 0 \\
	 *	-s & 0 & c & 0 \\
	 *	 0 & 0 & 0 & 1 \\
	 *	\end{matrix}\right]
	 *	\f]
	 *	where \f$c=cos(a)\f$ and \f$s=sin(a)\f$.
	 */
	void setRotationY(const float a)
	{
		float c = COS(a), s = SIN(a);
		(*this) = fmat_4x4_t::identity;
		// 0th column.
		(*this)(0, 0) = +c;
		(*this)(2, 0) = -s;
		// 2nd column.
		(*this)(0, 2) = +s;
		(*this)(2, 2) = +c;
	}

	/**
	 * @brief
	 *	Assign a matrix the values of a rotation matrix for counter-clockwise rotation around the y-axis.
	 * @param M
	 *	the matrix
	 * @param a
	 *	the angle of rotation, in Radians
	 * @see
	 *	fmat_4x4_t::setRotationY(const float)
	 */
	static void setRotationY(fmat_4x4_t& M, const float a)
	{
		M.setRotationY(a);
	}

	/**
	 * @brief
	 *	Assign this matrix the values of a rotation matrix for counter-clockwise rotation around the z-axis.
	 * @param a
	 *	the angle of rotation, in Radians
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
	 */
	void setRotationZ(const float a)
	{
		float c = COS(a), s = SIN(a);
		(*this) = fmat_4x4_t::identity;
		// 0th column.
		(*this)(0, 0) = +c;
		(*this)(1, 0) = +s;
		// 1st column.
		(*this)(0, 1) = -s;
		(*this)(1, 1) = +c;
	}

	/**
	 * @brief
	 *	Assign a matrix the values of a rotation matrix for counter-clockwise rotation around the z-axis.
	 * @param M
	 *	the matrix
	 * @param a
	 *	the angle of rotation, in Radians
	 * @see
	 *	fmat_4x4_t::setRotationZ
	 */
	static void setRotationZ(fmat_4x4_t& M, const float a)
	{
		M.setRotationZ(a);
	}

	/**
	 * @brief
	 *	Assign this matrix the values of a scaling matrix.
	 * @param self
	 *	this matrix
	 * @param s
	 *	a scaling vector
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
	void setScaling(const fvec3_t& s)
	{
		(*this) = fmat_4x4_t::identity;
		(*this)(0, 0) = s.x;
		(*this)(1, 1) = s.y;
		(*this)(2, 2) = s.z;
	}

	/**
	 * @brief
	 *	Assign a matrix the values of a scaling matrix.
	 * @param M
	 *	the matrix
	 * @param s
	 *	a scaling vector
	 * @see
	 *	fmat_4x4_t::setScaling(const fvec3_t& s)
	 */
	static void setScaling(fmat_4x4_t& M, const fvec3_t& s)
	{
		M.setScaling(s);
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
		target.x = (*this)(0, 0) * source.x + (*this)(0, 1) * source.y + (*this)(0, 2) * source.z + (*this)(0, 3) * source.w;
		target.y = (*this)(1, 0) * source.x + (*this)(1, 1) * source.y + (*this)(1, 2) * source.z + (*this)(1, 3) * source.w;
		target.z = (*this)(2, 0) * source.x + (*this)(2, 1) * source.y + (*this)(2, 2) * source.z + (*this)(2, 3) * source.w;
		target.w = (*this)(3, 0) * source.x + (*this)(3, 1) * source.y + (*this)(3, 2) * source.z + (*this)(3, 3) * source.w;
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

#if 0
// A wrapper for dmat_4x4_base_t.
/// Necessary in C so that the function return can be assigned to another matrix more simply.
struct s_dmat_4x4 { dmat_4x4_base_t v;  };
#endif
float *mat_Zero(fmat_4x4_base_t DST);
float *mat_Multiply(fmat_4x4_base_t DST, const fmat_4x4_base_t src1, const fmat_4x4_base_t src2);

float *mat_FourPoints(fmat_4x4_base_t DST, const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t upx, const float scale);
/// @param from viewer position
/// @param at look at position
/// @param world's up, usually 0,0,1
/// @param roll clockwise roll around viewing direction in Radians
void mat_View(fmat_4x4_t& self, const fvec3_t& from, const fvec3_t& at, const fvec3_t& world_up, const float roll);
// gl matrix support
void mat_gluPerspective(fmat_4x4_t& dst, const fmat_4x4_t& src, const float fovy, const float aspect, const float zNear, const float zFar);
void mat_gluLookAt(fmat_4x4_base_t &DST, const fmat_4x4_base_t &src, const float eyeX, const float eyeY, const float eyeZ, const float centerX, const float centerY, const float centerZ, const float upX, const float upY, const float upZ);
void mat_glRotate(fmat_4x4_base_t &DST, const fmat_4x4_base_t &src, const float angle, const float x, const float y, const float z);


bool mat_getChrUp(const fmat_4x4_t& mat, fvec3_t& up);
bool mat_getChrForward(const fmat_4x4_t& mat, fvec3_t& forward);
bool mat_getChrRight(const fmat_4x4_t& mat, fvec3_t& right);
bool mat_getCamUp(const fmat_4x4_t& mat, fvec3_t& up);
bool mat_getCamRight(const fmat_4x4_t& mat, fvec3_t& right);
bool mat_getCamForward(const fmat_4x4_t& mat, fvec3_t& forward);
bool mat_getTranslate(const fmat_4x4_t& mat, fvec3_t& translate);


fvec3_t mat_getTranslate_v(const fmat_4x4_base_t mat);

float *mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(fmat_4x4_base_t mat, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z);
float *mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(fmat_4x4_base_t mat, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z);


/**
 * @brief
 *	Dump a textual representation of a matrix to standard output.
 * @param a
 *	the matrix
 */
void dump_matrix(const fmat_4x4_base_t a);

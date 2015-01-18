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

typedef float fmat_4x4_base_t[16];      ///< the basic 4x4 single precision floating point ("float")  matrix type
#if 0
typedef double dmat_4x4_base_t[16];      ///< the basic 4x4 double precision floating point ("double") matrix type
#endif

#if 0
struct s_fmat_4x4;
typedef struct s_fmat_4x4  fmat_4x4_t;
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
	 *	Compute the product of two matrices.
	 * @param a, b
	 *	the matrices
	 * @return
	 *	the product <tt>a * b</tt>
	 * @todo
	 *	Add implementation.
	 */
	/**
	 * @brief
	 *	Compute the transpose of a matrix.
	 * @param a
	 *	the matrix
	 * @return
	 *	the transpose <tt>a^T</tt>
	 * @todo
	 *	Add implementation.
	 */
	/**
	 * @brief
	 *	Overloaded assignment addition operator.
	 */
	fmat_4x4_t& operator+=(const fmat_4x4_t& other)
	{
		for (size_t i = 0; i < 16; ++i)
		{
			this->v[i] += other.v[i];
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
			this->v[i] -= other.v[i];
		}
		return *this;
	}
	/**
	 * @brief
	 *	Overloaded assignment operator.
	 */
	const fmat_4x4_t& operator=(const fmat_4x4_t& other)
	{
		if (this != &other)
		{
			for (size_t i = 0; i < 16; ++i)
			{
				this->v[i] = other.v[i];
			}
		}
		return *this;
	}
	/**
	 * @brief
	 *	Set this matrix to the zero matrix.
	 * @see fmat_4x4_t::zero
	 */
	void setZero()
	{
		(*this) = fmat_4x4_t::zero;
#if 0
		for (size_t i = 0; i < 16; ++i)
		{
			this->v[i] = 0.0f;
		}
#endif
	}
	/**
	 * @brief
	 *	Set this matrix to the identity/multiplicative neutral matrix.
	 * @see fmat_4x4_t::identity
	 */
	void setIdentity()
	{
		(*this) = fmat_4x4_t::identity;
#if 0
		setZero();
		for (size_t i = 0; i < 4; ++i)
		{
			for (size_t j = 0; j < 4; ++j)
			{
				this->v2[i][j] = 1.0f;
			}
		}
#endif
	}
};

#if 0
// A wrapper for dmat_4x4_base_t.
/// Necessary in C so that the function return can be assigned to another matrix more simply.
struct s_dmat_4x4 { dmat_4x4_base_t v;  };
#endif

float *mat_Copy(fmat_4x4_base_t DST, const fmat_4x4_base_t src);
float *mat_Identity(fmat_4x4_base_t DST);
float *mat_Zero(fmat_4x4_base_t DST);
float *mat_Multiply(fmat_4x4_base_t DST, const fmat_4x4_base_t src1, const fmat_4x4_base_t src2);
float *mat_Translate(fmat_4x4_base_t DST, const float dx, const float dy, const float dz);
float *mat_RotateX(fmat_4x4_base_t DST, const float rads);
float *mat_RotateY(fmat_4x4_base_t DST, const float rads);
float *mat_RotateZ(fmat_4x4_base_t DST, const float rads);
float *mat_ScaleXYZ(fmat_4x4_base_t DST, const float sizex, const float sizey, const float sizez);
float *mat_FourPoints(fmat_4x4_base_t DST, const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t upx, const float scale);
float *mat_View(fmat_4x4_base_t DST, const fvec3_base_t   from, const fvec3_base_t   at, const fvec3_base_t   world_up, const float roll);
float *mat_Projection(fmat_4x4_base_t DST, const float near_plane, const float far_plane, const float fov, const float ar);
float *mat_Projection_orig(fmat_4x4_base_t DST, const float near_plane, const float far_plane, const float fov);
void   mat_TransformVertices(const fmat_4x4_base_t Matrix, const fvec4_t pSourceV[], fvec4_t pDestV[], const Uint32 NumVertor);

bool mat_getChrUp(const fmat_4x4_base_t mat, fvec3_t& up);
bool mat_getChrForward(const fmat_4x4_base_t mat, fvec3_t& forward);
bool mat_getChrRight(const fmat_4x4_base_t mat, fvec3_t& right);
bool mat_getCamUp(const fmat_4x4_base_t mat, fvec3_t& up);
bool mat_getCamRight(const fmat_4x4_base_t mat, fvec3_t& right);
bool mat_getCamForward(const fmat_4x4_base_t mat, fvec3_t& forward);
bool mat_getTranslate(const fmat_4x4_base_t mat, fvec3_t& translate);


fvec3_t mat_getTranslate_v(const fmat_4x4_base_t mat);

float *mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(fmat_4x4_base_t mat, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z);
float *mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(fmat_4x4_base_t mat, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z);

// gl matrix support
void mat_gluPerspective(fmat_4x4_base_t &DST, const fmat_4x4_base_t &src, const float fovy, const float aspect, const float zNear, const float zFar);
void mat_gluLookAt(fmat_4x4_base_t &DST, const fmat_4x4_base_t &src, const float eyeX, const float eyeY, const float eyeZ, const float centerX, const float centerY, const float centerZ, const float upX, const float upY, const float upZ);
void mat_glRotate(fmat_4x4_base_t &DST, const fmat_4x4_base_t &src, const float angle, const float x, const float y, const float z);

/**
 * @brief
 *	Dump a textual representation of a matrix to standard output.
 * @param a
 *	the matrix
 */
void dump_matrix(const fmat_4x4_base_t a);

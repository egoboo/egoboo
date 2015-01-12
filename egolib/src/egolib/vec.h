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

/// @file  egolib/vec.h
/// @brief 2-,3- and 4-dimensional vectors.

#pragma once

#include "egolib/typedef.h"
#include "egolib/_math.h"

typedef float fvec2_base_t[2];           ///< the basic floating point 2-vector type
typedef float fvec3_base_t[3];           ///< the basic floating point 3-vector type
typedef float fvec4_base_t[4];           ///< the basic floating point 4-vector type

/// A 2-vector type that allows more than one form of access
struct fvec2_t
{
	union
	{
		fvec2_base_t v;
		struct { float x, y; };
		struct { float s, t; };
	};
	const static fvec2_t zero;
	fvec2_t() : x(), y()
	{
	}
	fvec2_t(const fvec2_t& other) : x(other.x), y(other.y)
	{
	}
	fvec2_t(float x, float y)
	{
		this->x = x;
		this->y = y;
	}
	const fvec2_t& operator=(const fvec2_t& other)
	{
		x = other.x;
		y = other.y;
		return *this;
	}
	float& operator[](size_t const& index)
	{
		EGOBOO_ASSERT(index < 2);
		return this->v[index];
	}
	const float &operator[](size_t const& index) const
	{
		EGOBOO_ASSERT(index < 2);
		return this->v[index];
	}
};

/// A 3-vector type that allows more than one form of access
struct fvec3_t
{
	union
	{
		fvec3_base_t v;
		struct { float x, y, z; };
		struct { float r, g, b; };
	};
	const static fvec3_t zero;
	fvec3_t() : x(), y(), z()
	{
	}
	fvec3_t(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
	fvec3_t(const fvec3_t& other) : x(other.x), y(other.y), z(other.z)
	{
	}
	const fvec3_t& operator=(const fvec3_t& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		return *this;
	}
	float& operator[](size_t const& index)
	{
		EGOBOO_ASSERT(index < 3);
		return this->v[index];
	}
	const float &operator[](size_t const& index) const
	{
		EGOBOO_ASSERT(index < 3);
		return this->v[index];
	}
};

/// A 4-vector type that allows more than one form of access
struct fvec4_t
{
	union
	{
		fvec4_base_t v;
		struct { float x, y, z, w; };
		struct { float r, g, b, a; };
	};
	const static fvec3_t zero;
	fvec4_t() : x(), y(), z(), w()
	{
	}
	fvec4_t(const fvec4_t& other) : x(other.x), y(other.y), z(other.z), w(other.w)
	{
	}
	const fvec4_t& operator=(const fvec4_t& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
		return *this;
	}
	float& operator[](size_t const& index)
	{
		EGOBOO_ASSERT(index < 4);
		return this->v[index];
	}
	const float &operator[](size_t const& index) const
	{
		EGOBOO_ASSERT(index < 4);
		return this->v[index];
	}
};

// macros for initializing vectors to zero
#if 0
#define ZERO_VECT2   { {0.0f,0.0f} }
#endif
#define ZERO_VECT3   { {0.0f,0.0f,0.0f} }
#define ZERO_VECT4   { {0.0f,0.0f,0.0f,0.0f} }
#define ZERO_MAT_4X4 { {0.0f,0.0f,0.0f,0.0f, 0.0f,0.0f,0.0f,0.0f, 0.0f,0.0f,0.0f,0.0f, 0.0f,0.0f,0.0f,0.0f} }

// Macros for initializing vectors to specific values.
// Most C compilers will allow you to initialize to non-constant values, but they do complain.
#define VECT2(XX,YY) { {XX,YY} }
#define VECT3(XX,YY,ZZ) { {XX,YY,ZZ} }
#define VECT4(XX,YY,ZZ,WW) { {XX,YY,ZZ,WW} }

#if defined(TEST_NAN_RESULT)
	#define LOG_NAN_FVEC2(XX)      if( !fvec2_valid(XX) ) log_error( "**** A math operation resulted in an invalid vector result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
	#define LOG_NAN_FVEC3(XX)      if( !fvec3_valid(XX) ) log_error( "**** A math operation resulted in an invalid vector result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
	#define LOG_NAN_FVEC4(XX)      if( !fvec4_valid(XX) ) log_error( "**** A math operation resulted in an invalid vector result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#else
	#define LOG_NAN_FVEC2(XX)
	#define LOG_NAN_FVEC3(XX)
	#define LOG_NAN_FVEC4(XX)
#endif

bool   fvec2_valid(const fvec2_base_t A);
bool   fvec2_self_clear(fvec2_base_t A);
bool   fvec2_self_is_clear(const fvec2_base_t A);
bool   fvec2_base_copy(fvec2_base_t A, const fvec2_base_t B);
float  fvec2_length(const fvec2_base_t A);
float  fvec2_length_abs(const fvec2_base_t A);
float  fvec2_length_2(const fvec2_base_t A);
bool   fvec2_self_scale(fvec2_base_t A, const float B);
bool   fvec2_self_sum(fvec2_base_t A, const fvec2_base_t B);
bool   fvec2_self_normalize(fvec2_base_t A);
float  fvec2_cross_product(const fvec2_base_t A, const fvec2_base_t B);
float  fvec2_dot_product(const fvec2_base_t A, const fvec2_base_t B);
float  fvec2_dist_abs(const fvec2_base_t A, const fvec2_base_t B);
float *fvec2_sub(fvec2_base_t DST, const fvec2_base_t LHS, const fvec2_base_t RHS);
float *fvec2_add(fvec2_base_t DST, const fvec2_base_t LHS, const fvec2_base_t RHS);
float *fvec2_normalize(fvec2_base_t DST, const fvec2_base_t SRC);
float *fvec2_scale(fvec2_base_t DST, const fvec2_base_t SRC, const float B);

/**
 * @brief
 *	Construct a vector.
 * @param v
 *	the vector
 * @post
 *	the vector represents the null vector
 */
void fvec3_ctor(fvec3_base_t v);                 // @todo Remove this.
void fvec3_ctor(fvec3_t& v);
/**
 * @brief
 *	Destruct a vector.
 * @param v
 *	the vector
 */
void fvec3_dtor(fvec3_base_t v);                 // @todo Remove this.
void fvec3_dtor(fvec3_t& v);

bool fvec3_valid(const fvec3_base_t A);
/**
 * @brief
 *	Set a vector to the null vector.
 * @param v
 *	the vector
 * @post
 *	@a v was assigned the null vector
 */
bool fvec3_self_clear(fvec3_base_t v);
bool fvec3_self_is_clear(const fvec3_base_t A);
/**
 * @brief
 *	Multiply a vector by a scalar.
 * @param v
 *	the vector
 * @param s
 *	the scalar
 * @post
 *	@a v was assigned the product <tt>s*v</tt>.
 */
bool   fvec3_self_scale(fvec3_t& v, const float s);
bool   fvec3_self_scale(fvec3_base_t v, const float s); // @todo Remove this.

bool   fvec3_self_sum(fvec3_base_t A, const fvec3_base_t RHS);
float  fvec3_self_normalize(fvec3_base_t A);
float  fvec3_self_normalize_to(fvec3_base_t A, const float B);
/**
 * @brief
 *	Get the squared length of a vector
 *	(using the Euclidian metric).
 * @param v
 *	the vector
 * @return
 *	the squared length of the vector
 */
float fvec3_length_2(const fvec3_t& v);
float fvec3_length_2(const fvec3_base_t v);      // Remove this.

/**
 * @brief
 *	Get the length of a vector
 *	(using the Euclidian metric).
 * @param v
 *	the vector
 * @return
 *	the length of the vector
 */
float fvec3_length(const fvec3_t& v);
float fvec3_length(const fvec3_base_t v);        // Remove this.

/**
 * @brief
 *	Get the length of a vector
 *	(using the taxicab metric).
 * @param v
 *	the vector
 * @return
 *	the length of the vector
 */
float fvec3_length_abs(const fvec3_t& v);
float fvec3_length_abs(const fvec3_base_t v);    // Remove this.
/**
 * @brief
 *	Get the dot product of vectors.
 * @param u,v
 *	the vectors
 * @return
 *	the dot product <tt>u x v</tt>
 */
float fvec3_dot_product(const fvec3_base_t u, const fvec3_base_t v);
float fvec3_dot_product(const fvec3_t& u, const fvec3_t& v);

float  fvec3_dist_abs(const fvec3_base_t LHS, const fvec3_base_t RHS);
float  fvec3_dist_2(const fvec3_base_t LHS, const fvec3_base_t RHS);
float *fvec3_base_copy(fvec3_base_t DST, const fvec3_base_t SRC);
float *fvec3_scale(fvec3_base_t DST, const fvec3_base_t SRC, const float B);
float *fvec3_normalize(fvec3_base_t DST, const fvec3_base_t SRC);
float *fvec3_add(fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS);
float *fvec3_sub(fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS);
float *fvec3_cross_product(fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS);
float  fvec3_decompose(const fvec3_base_t src, const fvec3_base_t vnrm, fvec3_base_t vpara, fvec3_base_t vperp);

bool   fvec4_valid(const fvec4_base_t A);
bool   fvec4_self_clear(fvec4_base_t A);
bool   fvec4_self_scale(fvec4_base_t A, const float B);
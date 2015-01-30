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

/**
 * @brief
 *	Enumerated indices for the elements of vectors.
 */
enum { kX = 0, kY, kZ, kW };


typedef float fvec2_base_t[2];           ///< the basic floating point 2-vector type
typedef float fvec3_base_t[3];           ///< the basic floating point 3-vector type
typedef float fvec4_base_t[4];           ///< the basic floating point 4-vector type

/// A 2-vector type that allows more than one form of access.
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

	/**
	 * @brief
	 *	Compute the dot product of this vector and another vector.
	 * @param other
	 *	the other vector
	 * @return
	 *	the dot product <tt>(*this) * other</tt> of this vector and the other vector
	 */
	float dot(const fvec2_t& other) const
	{
		return this->v[kX] * other.v[kX]
			 + this->v[kY] * other.v[kY]
			 ;
	}

	/**
	 * @brief
	 *	Multiply this vector by a scalar.
	 * @param scalar
	 *	the scalar
	 * @post
	 *	The product <tt>scalar * (*this)</tt> was assigned to <tt>*this</tt>.
	 */
	void multiply(float scalar)
	{
		this->v[kX] *= scalar;
		this->v[kY] *= scalar;
	}

	/**
	* @brief
	*	Normalize this vector to the specified length.
	* @param length
	*	the length
	* @post
	*	If <tt>*this</tt> is the null/zero vector, then <tt>*this</tt> was assigned the null/zero vector
	*	and is assigned <tt>length * (*this) / |(*this)|</tt> otherwise.
	*/
	void normalize(float length)
	{
		float l = this->length();
		if (l > 0.0f)
		{
			multiply(length / l);
		}
	}

	/**
	 * @brief
	 *	Normalize this vector.
	 * @return
	 *	the old length of this vector
	 * @post
	 *	If <tt>*this</tt> is the null/zero vector, then <tt>*this</tt> was assigned the null/zero vector
	 *	and is assigned <tt>(*this) / l</tt> (where @a l is the old length of <tt>(*this)</tt>) otherwise.
	 */
	float normalize()
	{
		float l = length();
		if (l > 0.0f)
		{
			multiply(1.0f / l);
		}
		return l;
	}

	/**
	 * @brief
	 *	Get if this vector equals another vectors.
	 * @param other
	 *	the other vector
	 * @return
	 *	@a true if this vector equals the other vector
	 */
	bool equals(const fvec2_t& other)
	{
		return this->x == other.x
			&& this->y == other.y
			;
	}

	/**
	 * @brief
	 *	Get the squared length of this vector
	 *	(using the Euclidian metric).
	 * @return
	 *	the squared length of this vector
	 */
	float length_2() const
	{
		return this->v[kX] * this->v[kX]
	 		 + this->v[kY] * this->v[kY]
			 ;
	}

	/**
	 * @brief
	 *	Get the length of this vector
	 *	(using the Euclidian metric).
	 * @return
	 *	the length of this vector
	 */
	float length() const
	{
		return std::sqrt(length_2());
	}

	/**
	 * @brief
	 *	Get the length of this vector
	 *	(using the taxicab metric).
	 * @return
	 *	the length of this vector
	 */
	float length_abs() const
	{
		return std::abs(this->v[kX]) + std::abs(this->v[kY]);
	}

	const fvec2_t& operator=(const fvec2_t& other)
	{
		x = other.x;
		y = other.y;
		return *this;
	}

	fvec2_t operator+(const fvec2_t& other) const
	{
		return fvec2_t(x + other.x, y + other.y);
	}

	fvec2_t& operator+=(const fvec2_t& other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	fvec2_t operator-(const fvec2_t& other) const
	{
		return fvec2_t(x - other.x, y - other.y);
	}

	fvec2_t& operator-=(const fvec2_t& other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	fvec2_t operator*(const float other) const
	{
		return fvec2_t(other * v[kX], other * v[kY]);
	}

	fvec2_t& operator*=(float scalar)
	{
		multiply(scalar);
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

fvec2_t operator-(const fvec2_t& v);

/// A 3-vector type that allows more than one form of access.
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

	/**
	 * @brief
	 *	Get the component-wise absolute of this vector.
	 * @return
	 *	the component-wise absolute of this vector
	 * @remark
	 *	The component-wise absolute of a vector \f$v\in\mathbb{R}^n,n>0\f$ is defined as
	 *	\f[
	 *	abs(\vec{v}) = (abs(v_1),\ldots,abs(v_n))
	 *	\f]
	 */
	fvec3_t abs() const
	{
		return
			fvec3_t
			(
				std::abs(v[kX]),
				std::abs(v[kY]),
				std::abs(v[kZ])
			);
	}

	/**
	 * @brief
	 *	Get the component-wise minimum of this vector and another vector.
	 * @param other
	 *	the other vector
	 * @return
	 *	the component-wise minimum
	 * @remark
	 *	For two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^n,n>0\f$ the component-wise minimum is defined as
	 *	\f[
	 *	min\left(\vec{u},\vec{v}\right)=left(min(u_1,v_1),\ldots,min(u_n,v_n)\right)
	 *	\f]
	 */
	fvec3_t min(const fvec3_t& other) const
	{
		return
			fvec3_t
				(
					std::min(this->v[kX],other.v[kX]),
					std::min(this->v[kY],other.v[kY]),
					std::min(this->v[kZ],other.v[kZ])
				);
	}

	/**
	 * @brief
	 *	Get the component-wise maximum of this vector and another vector.
	 * @param other
	 *	the other vector
	 * @return
	 *	the component-wise maximum
	 * @remark
	 *	For two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^n,n>0\f$ the component-wise maximum is defined as
	 *	\f[
	 *	max\left(\vec{u},\vec{v}\right)=left(max(u_1,v_1),\ldots,max(u_n,v_n)\right)
	 *	\f]
	 */
	fvec3_t max(const fvec3_t& other) const
	{
		return
			fvec3_t
			(
				std::max(this->v[kX], other.v[kX]),
				std::max(this->v[kY], other.v[kY]),
				std::max(this->v[kZ], other.v[kZ])
			);
	}

	/**
	 * @brief
	 *	Compute the cross product of this vector and another vector.
	 * @param other
	 *	the other vector
	 * @return
	 *	the cross product <tt>(*this) x other</tt> of this vector and the other vector
	 */
	fvec3_t cross(const fvec3_t& other) const
	{
		return
			fvec3_t
			(
				this->v[kY] * other.v[kZ] - this->v[kZ] * other.v[kY],
				this->v[kZ] * other.v[kX] - this->v[kX] * other.v[kZ],
				this->v[kX] * other.v[kY] - this->v[kY] * other.v[kX]
			);
	}

	/**
	 * @brief
	 *	Compute the dot product of this vector and another vector.
	 * @param other
	 *	the other vector
	 * @return
	 *	the dot product <tt>(*this) * other</tt> of this vector and the other vector
	 */
	float dot(const fvec3_t& other) const
	{
		return this->v[kX] * other.v[kX]
			 + this->v[kY] * other.v[kY]
			 + this->v[kZ] * other.v[kZ]
			 ;
	}

	/**
	 * @brief
	 *	Multiply this vector by a scalar.
	 * @param scalar
	 *	the scalar
	 * @post
	 *	The product <tt>scalar * (*this)</tt> was assigned to <tt>*this</tt>.
	 */
	void multiply(float scalar)
	{
		this->v[kX] *= scalar;
		this->v[kY] *= scalar;
		this->v[kZ] *= scalar;
	}

	/**
	 * @brief
	 *	Normalize this vector to the specified length.
	 * @param length
	 *	the length
	 * @post
	 *	If <tt>*this</tt> is the null/zero vector, then <tt>*this</tt> was assigned the null/zero vector
	 *	and is assigned <tt>length * (*this) / |(*this)|</tt> otherwise.
	 */
	void normalize(float length)
	{
		float l = this->length();
		if (l > 0.0f)
		{
			multiply(length / l);
		}
	}

	/**
	 * @brief
	 *	Normalize this vector.
	 * @return
	 *	the <em>old</em> length of the vector
	 * @post
	 *	If <tt>*this</tt> is the null/zero vector, then <tt>*this</tt> was assigned the null/zero vector
	 *	and is assigned <tt>(*this) / |(*this)|</tt> otherwise.
	 */
	float normalize()
	{
		float l = this->length();
		if (l > 0.0f)
		{
			multiply(1.0f / l);
		}
		return l;
	}

	/**
	 * @brief
	 *	Get if this vector equals another vectors.
	 * @param other
	 *	the other vector
	 * @return
	 *	@a true if this vector equals the other vector
	 */
	bool equals(const fvec3_t& other)
	{
		return this->x == other.x
			&& this->y == other.y
			&& this->z == other.z
			;
	}

	/**
 	 * @brief
	 *	Get the squared length of this vector
	 *	(using the Euclidian metric).
	 * @return
	 *	the squared length of this vector
	 */
	float length_2() const
	{
		return this->v[kX] * this->v[kX]
			 + this->v[kY] * this->v[kY]
			 + this->v[kZ] * this->v[kZ]
			 ;
	}

	/**
	 * @brief
	 *	Get the length of this vector
	 *	(using the Euclidian metric).
	 * @return
	 *	the length of this vector
	 */
	float length() const
	{
		return std::sqrt(length_2());
	}

	/**
	 * @brief
	 *	Get the length of this vector
	 *	(using the taxicab metric).
	 * @return
	 *	the length of this vector
	 */
	float length_abs() const
	{
		return std::abs(this->v[kX]) + std::abs(this->v[kY]) + std::abs(this->v[kZ]);
	}

	const fvec3_t& operator=(const fvec3_t& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		return *this;
	}

	fvec3_t operator+(const fvec3_t& other) const
	{
		return fvec3_t(x + other.x, y + other.y, z + other.z);
	}

	fvec3_t& operator+=(const fvec3_t& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	fvec3_t operator-(const fvec3_t& other) const
	{
		return fvec3_t(x - other.x, y - other.y, z - other.z);
	}

	fvec3_t& operator-=(const fvec3_t& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	fvec3_t operator*(const float other) const
	{
		return fvec3_t(other * v[kX], other * v[kY], other * v[kZ]);
	}

	fvec3_t& operator*=(float scalar)
	{
		multiply(scalar);
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

fvec3_t operator-(const fvec3_t& v);

/// A 4-vector type that allows more than one form of access.
struct fvec4_t
{

	union
	{
		fvec4_base_t v;
		struct { float x, y, z, w; };
		struct { float r, g, b, a; };
	};
	
	const static fvec4_t zero;
	
	fvec4_t() : x(), y(), z(), w()
	{
	}

	fvec4_t(float x, float y, float z, float w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
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

	/**
	 * @brief
	 *	Multiply this vector by a scalar.
	 * @param scalar
	 *	the scalar
	 * @post
	 *	The product <tt>scalar * (*this)</tt> was assigned to <tt>*this</tt>.
	 */
	void multiply(float scalar)
	{
		this->v[kX] *= scalar;
		this->v[kY] *= scalar;
		this->v[kZ] *= scalar;
		this->v[kW] *= scalar;
	}

	/**
	 * @brief
	 *	Normalize this vector.
	 * @return
	 *	the old length of this vector
	 * @post
	 *	If <tt>*this</tt> is the null/zero vector, then <tt>*this</tt> was assigned the null/zero vector
	 *	and is assigned <tt>(*this) / l</tt> (where @a l is the old length of <tt>(*this)</tt>) otherwise.
	 */
	float normalize()
	{
		float l = length();
		if (l > 0.0f)
		{
			multiply(1.0f / l);
		}
		return l;
	}

	/**
	 * @brief
	 *	Get if this vector equals another vectors.
	 * @param other
	 *	the other vector
	 * @return
	 *	@a true if this vector equals the other vector
	 */
	bool equals(const fvec4_t& other)
	{
		return this->x == other.x
			&& this->y == other.y
			&& this->z == other.z
			&& this->w == other.w
			;
	}
	/**
	 * @brief
	 *	Get the squared length of this vector
	 *	(using the Euclidian metric).
	 * @return
	 *	the squared length of this vector
	 */
	float length_2() const
	{
		return this->v[kX] * this->v[kX]
			 + this->v[kY] * this->v[kY]
			 + this->v[kZ] * this->v[kZ]
			 + this->v[kW] * this->v[kW]
			 ;
	}

	/**
	 * @brief
	 *	Get the length of this vector
	 *	(using the Euclidian metric).
	 * @return
	 *	the length of this vector
	 */
	float length() const
	{
		return std::sqrt(length_2());
	}

	/**
	 * @brief
	 *	Get the length of this vector
	 *	(using the taxicab metric).
	 * @return
	 *	the length of this vector
	 */
	float length_abs() const
	{
		return std::abs(this->v[kX]) + std::abs(this->v[kY]) + std::abs(this->v[kZ]) + std::abs(this->v[kW]);
	}

};

fvec4_t operator-(const fvec4_t& v);

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
#if 0
float  fvec2_length(const fvec2_base_t A);
#endif
float  fvec2_length_abs(const fvec2_base_t A);
float  fvec2_length_2(const fvec2_t& v);
float  fvec2_length_2(const fvec2_base_t v);
bool   fvec2_self_scale(fvec2_base_t A, const float B);
bool   fvec2_self_sum(fvec2_base_t A, const fvec2_base_t B);
bool   fvec2_self_normalize(fvec2_base_t A);
float  fvec2_cross_product(const fvec2_base_t A, const fvec2_base_t B);
float  fvec2_dot_product(const fvec2_base_t A, const fvec2_base_t B);
#if 0
float *fvec2_sub(fvec2_base_t DST, const fvec2_base_t LHS, const fvec2_base_t RHS);
#endif
#if 0
float *fvec2_add(fvec2_base_t DST, const fvec2_base_t LHS, const fvec2_base_t RHS);
#endif
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
void fvec3_ctor(fvec3_t& v);

/**
 * @brief
 *	Destruct a vector.
 * @param v
 *	the vector
 */
void fvec3_dtor(fvec3_t& v);
#if 0
fvec3_t fvec3_scale(const fvec3_t& v, float s); ///< @todo Remove this.
#endif
bool fvec3_valid(const fvec3_base_t A); ///< @todo Remove this.
bool fvec3_self_clear(fvec3_base_t v); ///< @todo Remove this.
float *fvec3_base_copy(fvec3_base_t DST, const fvec3_base_t SRC); ///< @todo Remove this.
bool fvec3_self_is_clear(const fvec3_base_t A); ///< @todo Remove this.

/**
 * @brief
 *	Get the distance between to points
 *	(using the taxicab metric).
 * @param x,y
 *	the points
 * @return
 *	the distance between the points
 */
float fvec3_dist_abs(const fvec3_t& u, const fvec3_t& v);
#if 0
float fvec2_dist_abs(const fvec2_base_t u, const fvec2_base_t v);
#endif

/**
 * @brief
 *	Get the squared distance between two points.
 *	(using the Euclidian metric).
 * @param u,v
 *	the vectors
 * @return
 *	the squared distance between the vectors
 */
float fvec3_dist_2(const fvec3_t& u, const fvec3_t& v);





float  fvec3_decompose(const fvec3_t& src, const fvec3_t& vnrm, fvec3_t& vpara, fvec3_t& vperp);

bool   fvec4_valid(const fvec4_base_t A);
bool   fvec4_self_clear(fvec4_base_t v);
bool   fvec4_self_scale(fvec4_base_t A, const float B);
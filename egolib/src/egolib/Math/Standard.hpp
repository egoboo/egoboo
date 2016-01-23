#pragma once

#include "egolib/Math/AABB.hpp"
#include "egolib/Math/Cone3.hpp"
#include "egolib/Math/Cube.hpp"
#include "egolib/Math/Line.hpp"
#include "egolib/Math/Matrix.hpp"
#include "egolib/Math/Plane.hpp"
#include "egolib/Math/Point.hpp"
#include "egolib/Math/Sphere.h"
#include "egolib/Math/Vector.hpp"


/**
 * @brief
 *  Enumerated indices for the elements of vectors.
 * @todo
 *  Remove this.
 */
enum {
    kX = 0, kY, kZ, kW
};

typedef Ego::Math::Field<float> Fieldf;

/// A 2D vector.
typedef Ego::Math::Vector<Fieldf, 2> Vector2f;

/// A 3D vector.
typedef Ego::Math::Vector<Fieldf, 3> Vector3f;

/// A 4D vector.
typedef Ego::Math::Vector<Fieldf, 4> Vector4f;

/// A 2D vector space.
typedef Ego::Math::VectorSpace<Fieldf, 2> VectorSpace2f;

/// A 3D vector space.
typedef Ego::Math::VectorSpace<Fieldf, 3> VectorSpace3f;

/// A 4D vector space.
typedef Ego::Math::VectorSpace<Fieldf, 4> VectorSpace4f;

/// A 2D Euclidean space.
typedef Ego::Math::EuclideanSpace<VectorSpace2f> EuclideanSpace2f;

/// A 3D Euclidean space.
typedef Ego::Math::EuclideanSpace<VectorSpace3f> EuclideanSpace3f;

/// A 4D Euclidean space.
typedef Ego::Math::EuclideanSpace<VectorSpace4f> EuclideanSpace4f;

/// A 3D sphere.
typedef Ego::Math::Sphere<EuclideanSpace3f> Sphere3f;

/// A 3D AABB.
typedef Ego::Math::AABB<EuclideanSpace3f> AABB3f;

/// A 2D AABB.
typedef Ego::Math::AABB<EuclideanSpace2f> AABB2f;
/// A 2D AABB can also be considered as a rectangle.
typedef AABB2f Rectangle2f;

/// A 3D cone.
typedef Ego::Math::Cone3<float> Cone3f;

/// A 3D cube.
typedef Ego::Math::Cube<EuclideanSpace3f> Cube3f;

/// A 3D line.
typedef Ego::Math::Line<EuclideanSpace3f> Line3f;

/// A 3D point.
typedef Ego::Math::Point<VectorSpace3f> Point3f;

/// A 3D plane.
typedef Ego::Math::Plane3<float> Plane3f;

#ifdef _DEBUG
namespace Ego {
namespace Debug {

template <>
void validate<float>(const char *file, int line, const float& object);

template <>
void validate<::Vector2f>(const char *file, int line, const ::Vector2f& object);

template <>
void validate<::Vector3f>(const char *file, int line, const ::Vector3f& object);

template <>
void validate<::Vector4f>(const char *file, int line, const ::Vector4f& object);

template <>
void validate<::AABB3f>(const char *file, int line, const ::AABB3f& object);

template <>
void validate<::Sphere3f>(const char *file, int line, const ::Sphere3f& object);

template <>
void validate<::Cube3f>(const char *file, int line, const ::Cube3f& object);

} // namespace Debug
} // namespace Ego
#endif

/// A \f$4\f$-order floating point matrix.
typedef Ego::Math::Matrix<float, 4, 4> Matrix4f4f;

struct Utilities {

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
	 *  Move this into Vector4f.
	 * @todo
	 *  Add an implementation for Vector2f and Vector3f returning Matrix2f2f and Matrix3f3f.
	 * @todo
	 *  Update documentation for the Vector4f case.
	 */
	static Matrix4f4f tensor(const Vector4f& v, const Vector4f& w) {
		return
			Matrix4f4f
			(
				v[kX] * w[kX], v[kX] * w[kY], v[kX] * w[kZ], v[kX] * w[kW],
				v[kY] * w[kX], v[kY] * w[kY], v[kY] * w[kZ], v[kY] * w[kW],
				v[kZ] * w[kX], v[kZ] * w[kY], v[kZ] * w[kZ], v[kZ] * w[kW],
				v[kW] * w[kX], v[kW] * w[kY], v[kW] * w[kZ], v[kW] * w[kW]
			);
	}
	/**
	 * @brief
	 *  Get the translation vector of this matrix i.e. the vector \f$(m_{0,3},m_{1,3},m_{2,3})\f$.
	 * @return
	 *  the translation vector
	 */
	static Vector3f getTranslation(const Matrix4f4f& m) {
		return Vector3f(m(0, 3), m(1, 3), m(2, 3));
	}

	/**
	 * @brief
	 *  Transform vector.
	 * @param m
	 *  the transformation matrix
	 * @param source
	 *  the source vector
	 * @param [out] target
	 *  a vector which is assigned the transformation result
	 * @remark
	 *  \f[
	 *  \left[\begin{matrix}
	 *  m_{0,0} & m_{0,1} & m_{0,2} & m_{0,3} \\
	 *  m_{1,0} & m_{1,1} & m_{1,2} & m_{1,3} \\
	 *  m_{2,0} & m_{2,1} & m_{2,2} & m_{2,3} \\
	 *  m_{3,0} & m_{3,1} & m_{3,2} & m_{3,3} \\
	 *  \end{matrix}\right]
	 *  \cdot
	 *  \left[\begin{matrix}
	 *  v_{0} \\
	 *  v_{1} \\
	 *  v_{2} \\
	 *  v_{3} \\
	 *  \end{matrix}\right]
	 *  =
	 *  \left[\begin{matrix}
	 *  m_{0,0} \cdot v_{0} + m_{0,1} \cdot v_1 + m_{0,2} \cdot v_2 + m_{0,3} \cdot v_3 \\
	 *  m_{1,0} \cdot v_{0} + m_{1,1} \cdot v_1 + m_{1,2} \cdot v_2 + m_{1,3} \cdot v_3 \\
	 *  m_{2,0} \cdot v_{0} + m_{2,1} \cdot v_1 + m_{2,2} \cdot v_2 + m_{2,3} \cdot v_3 \\
	 *  m_{3,0} \cdot v_{0} + m_{3,1} \cdot v_1 + m_{3,2} \cdot v_2 + m_{3,3} \cdot v_3 \\
	 *  \end{matrix}\right]
	 *	=
	 *  \left[\begin{matrix}
	 *  r_0 \cdot v \\
	 *  r_1 \cdot v \\
	 *  r_2 \codt v \\
	 *  r_3 \cdot v \\
	 *  \end{matrix}\right]
	 *  \f]
	 *  where \f$r_i\f$ is the $i$-th row of the matrix.
	 */
	static void transform(const Matrix4f4f& m, const Vector4f& source, Vector4f& target) {
		target[kX] = m(0, 0) * source[kX] + m(0, 1) * source[kY] + m(0, 2) * source[kZ] + m(0, 3) * source[kW];
		target[kY] = m(1, 0) * source[kX] + m(1, 1) * source[kY] + m(1, 2) * source[kZ] + m(1, 3) * source[kW];
		target[kZ] = m(2, 0) * source[kX] + m(2, 1) * source[kY] + m(2, 2) * source[kZ] + m(2, 3) * source[kW];
		target[kW] = m(3, 0) * source[kX] + m(3, 1) * source[kY] + m(3, 2) * source[kZ] + m(3, 3) * source[kW];
	}

	/**
	 * @brief
	 *  Transform vectors.
	 * @param m
	 *  the transformation matrix
	 * @param sources
	 *  the source vectors
	 * @param [out] targets
	 *  an array of vectors which are assigned the transformation results
	 * @see
	 *  Matrix4f4f::transform(const fmat_4x4_t& const Vector4f&, Vector4f&)
	 */
	static void transform(const Matrix4f4f& m, const Vector4f sources[], Vector4f targets[], const size_t size) {
		const Vector4f *source = sources;
		Vector4f *target = targets;
		for (size_t index = 0; index < size; ++index) {
			transform(m, *source, *target);
			source++;
			target++;
		}
	}
};

void mat_FourPoints(Matrix4f4f& DST, const Vector3f& ori, const Vector3f& wid, const Vector3f& frw, const Vector3f& up, const float scale);

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
Vector3f mat_getChrUp(const Matrix4f4f& mat);

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
Vector3f mat_getChrForward(const Matrix4f4f& mat);

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
Vector3f mat_getChrRight(const Matrix4f4f& mat);


bool mat_getCamUp(const Matrix4f4f& mat, Vector3f& up);
bool mat_getCamRight(const Matrix4f4f& mat, Vector3f& right);
bool mat_getCamForward(const Matrix4f4f& mat, Vector3f& forward);


Vector3f mat_getTranslate(const Matrix4f4f& mat);

void mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(Matrix4f4f& mat, const Vector3f& scale, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const Vector3f& translate);
void mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(Matrix4f4f& mat, const Vector3f& scale, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const Vector3f& translate);


/**
 * @brief
 *	Dump a textual representation of a matrix to standard output.
 * @param a
 *	the matrix
 */
void dump_matrix(const Matrix4f4f& a);


/// @todo Remove this.
float fvec3_decompose(const Vector3f& src, const Vector3f& vnrm, Vector3f& vpara, Vector3f& vperp);




#pragma once

#include "egolib/Math/AxisAlignedBox.hpp"
#include "egolib/Math/Cone3.hpp"
#include "egolib/Math/AxisAlignedCube.hpp"
#include "egolib/Math/Discrete.hpp"
#include "egolib/Math/OrderedField.hpp"
#include "egolib/Math/OrderedRing.hpp"
#include "egolib/Math/Line.hpp"
#include "egolib/Math/Matrix.hpp"
#include "egolib/Math/Plane.hpp"
#include "egolib/Math/Point.hpp"
#include "egolib/Math/Interval.hpp"
#include "egolib/Math/Ray.hpp"
#include "egolib/Math/Sphere.hpp"
#include "egolib/Math/Vector.hpp"
#include "egolib/Math/VectorSpace.hpp"


/**
 * @brief
 *  Enumerated indices for the elements of vectors.
 * @todo
 *  Remove this.
 */
enum {
    kX = 0, kY, kZ, kW
};

/// discrete ring
using Ringi = Ego::Math::OrderedRing<int>;
/// discrete size
using Size2i = Ego::Math::Discrete::Size2<int>;
/// discrete point
using Point2i = Ego::Math::Discrete::Point2<int>;
/// discrete vector
using Vector2i = Ego::Math::Discrete::Vector2<int>;



/// single-precision floating-point field
using OrderedFieldf = Ego::Math::OrderedField<float>;



/// single-precision floating-point 2d vector space
using VectorSpace2f = Ego::Math::VectorSpace<OrderedFieldf, 2>;
/// single-precision floating-point 3d vector space
using VectorSpace3f = Ego::Math::VectorSpace<OrderedFieldf, 3>;
/// single-precision floating-point 4d vector space
using VectorSpace4f = Ego::Math::VectorSpace<OrderedFieldf, 4>;



/// single-precision floating-point 2d Euclidean space
using EuclideanSpace2f = Ego::Math::EuclideanSpace<VectorSpace2f>;
/// single-precision floating-point 3d Euclidean space
using EuclideanSpace3f = Ego::Math::EuclideanSpace<VectorSpace3f>;
/// single-precision floating-point 4d Euclidean space
using EuclideanSpace4f = Ego::Math::EuclideanSpace<VectorSpace4f>;



/// single-precision floating-point 2d vector.
using Vector2f = Ego::Math::Vector<OrderedFieldf, 2>;
/// single-precision floating-point 3d vector.
using Vector3f = Ego::Math::Vector<OrderedFieldf, 3>;
/// single-precision floating-point 4d vector.
using Vector4f = Ego::Math::Vector<OrderedFieldf, 4>;



/// single-precision floating-point 2d point.
using Point2f = Ego::Math::Point<VectorSpace2f>;
/// single-precision floating-point 3d point.
using Point3f = Ego::Math::Point<VectorSpace3f>;
/// single-precision floating-point 4d point.
using Point4f = Ego::Math::Point<VectorSpace4f>;



/// single-precision floating-point 2d axis aligned box.
using AxisAlignedBox2f = Ego::Math::AxisAlignedBox<EuclideanSpace2f>;
/// single-precision floating-point rectangle (i.e. an axis aligned box in 2d),
using Rectangle2f = AxisAlignedBox2f;
/// A 3D axis aligned box.
using AxisAlignedBox3f = Ego::Math::AxisAlignedBox<EuclideanSpace3f>;



/// single-precision floating-point 2d sphere.
using Sphere2f = Ego::Math::Sphere<EuclideanSpace2f>;
/// single-precision floating-point circle (i.e. a sphere in 2d).
using Circle2f = Sphere2f;
/// single-precision floating-point 3d sphere.
using Sphere3f = Ego::Math::Sphere<EuclideanSpace3f>;



/// single-precision floating-point 2d line.
using Line2f = Ego::Math::Line<EuclideanSpace2f>;
/// single-precision floating-point 3d line.
using Line3f = Ego::Math::Line<EuclideanSpace3f>;



/// single-precision floating-point 2d ray.
using Ray2f = Ego::Math::Ray<EuclideanSpace2f>;
/// single-precision floating-point 3d ray.
using Ray3f = Ego::Math::Ray<EuclideanSpace3f>;



/// single-precision floating-point 2d plane.
using Plane3f = Ego::Math::Plane3<EuclideanSpace3f>;



/// A 3D cone.
using Cone3f = Ego::Math::Cone3<EuclideanSpace3f>;
/// A 3D axis aligned cube.
using AxisAlignedCube3f = Ego::Math::AxisAlignedCube<EuclideanSpace3f>;



namespace Ego {
namespace Math {

/// An interval with single-precision floating-point components.
using Intervalf = Interval<float>;

/// A colour in RGB colour space with single-precision floating-point components each within the range from 0 (inclusive) to 1 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 1 indicates maximal intensity of the component.
using Colour3f =  Id::Colour<Id::RGBf>;

/// A colour in RGBA colour space with single-precision floating-point components each within the range from 0 (inclusive) to 1 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 1 indicates maximal intensity of the component.
using Colour4f = Id::Colour<Id::RGBAf>;

/// A colour in RGB colour space with unsigned integer components each within the range from 0 (inclusive) to 255 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
using Colour3b = Id::Colour<Id::RGBb>;

/// A colour in RGBA colour space with unsigned integer components each within the range from 0 (inclusive) to 255 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
using Colour4b = Id::Colour<Id::RGBAb>;

} // namespace Math
} // namespace Ego


namespace Ego {
/**
* @brief
*   A rectangle in a 2 dimensional Cartesian coordinate system
*  (positive x-axis from left to right, positive y-axis from top to bottom).
*/
template <typename Type>
struct Rectangle {
    Type _left;   ///< @brief The coordinate of the left side   of the rectangle.
                  ///< @invariant <tt>left <= right</tt>.
    Type _bottom; ///< @brief The coordinate of the bottom side of the rectangle.
                  ///< @invariant <tt>top <= bottom</tt>.
    Type _right;  ///< @brief The coordinate of the right side  of the rectangle.
    Type _top;    ///< @brief The coordinate of the top side    of the rectangle.

                  /**
                  * @brief
                  * Construct an empty rectangle.
                  */
    Rectangle() :
        _left{},
        _bottom{},
        _right{},
        _top{} {}

    /**
    * @brief
    *   Get left coordinate of this rectangle.
    * @return
    *   the left coordinate of this rectangle
    */
    inline Type getLeft() const { return _left; }

    /**
    * @brief
    *   Get top coordinate of this rectangle.
    * @return
    *   the top coordinate of this rectangle
    */
    inline Type getTop() const { return _top; }

    /**
    * @brief
    *   Get right coordinate of this rectangle.
    * @return
    *   the right coordinate of this rectangle
    */
    inline Type getRight() const { return _right; }

    /**
    * @brief
    *   Get bottom coordinate of this rectangle.
    * @return
    *   the bottom coordinate of this rectangle
    */
    inline Type getBottom() const { return _bottom; }

    /**
    * @brief
    *   Construct this rectangle with the specified sides.
    * @param left
    *   the coordinate of the left side
    * @param bottom
    *   the coordinate of the bottom side
    * @param right
    *   the coordinate of the right side
    * @param top
    *   the coordinate of the top side
    * @throws std::domain_error
    *   if <tt>left > right</tt> or <tt>bottom > top</tt>
    */
    Rectangle(const Type& left, const Type& bottom, const Type& right, const Type& top) :
        _left(left),
        _bottom(bottom),
        _right(right),
        _top(top) {
        if (!(_left <= _right)) {
            throw std::domain_error("the coordinate of the left side must be smaller than or equal to the coordinate of the right side");
        }
        if (!(_top <= _bottom)) {
            throw std::domain_error("the coordinate of the top side must be smaller than or equal to the coordinate of the bottom side");
        }
    }

    bool point_inside(const Type& x, const Type& y) const {
        EGOBOO_ASSERT(_left <= _right && _top <= _bottom);
        if (x < _left || x > _right) return false;
        if (y < _top || y > _bottom) return false;
        return true;
    }
};
};

struct ego_frect_t {
    float xmin, ymin;
    float xmax, ymax;
};

#ifdef _DEBUG
namespace Ego {
namespace Debug {

template <>
struct Validate<float> {
    void operator()(const char *file, int line, float object) const {
        if (float_bad(object)) {
            auto e = Log::Entry::create(Log::Level::Error, file, line, "invalid floating point value", Log::EndOfEntry);
            Log::get() << e;
            throw std::runtime_error(e.getText());
        }
    }
};

template <typename EuclideanSpaceType>
struct Validate<Ego::Math::Point<EuclideanSpaceType>> {
    void operator()(const char *file, int line, const Ego::Math::Point<EuclideanSpaceType>& object) const {
        static const Validate<typename EuclideanSpaceType::ScalarType> validate{};
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality(); ++i) {
            validate(file, line, object[i]);
        }
    }
};

template <typename ScalarFieldType, size_t Dimensionality>
struct Validate<Ego::Math::Vector<ScalarFieldType, Dimensionality>> {
    void operator()(const char *file, int line, const Ego::Math::Vector<ScalarFieldType, Dimensionality>& object) const {
        static const Validate<typename ScalarFieldType::ScalarType> validate{};
        for (size_t i = 0; i < Dimensionality; ++i) {
            validate(file, line, object[i]);
        }
    }
};

template <typename EuclidianSpaceType>
struct Validate<Ego::Math::AxisAlignedBox<EuclidianSpaceType>> {
    void operator()(const char *file, int line, const Ego::Math::AxisAlignedBox<EuclidianSpaceType>& object) const {
        static const MakeValidate<decltype(object.getMin())> validateMin;
        static const MakeValidate<decltype(object.getMax())> validateMax;
        validateMin(file, line, object.getMin());
        validateMax(file, line, object.getMax());
    }
};

template <typename EuclidianSpaceType>
struct Validate<Ego::Math::Sphere<EuclidianSpaceType>> {
    void operator()(const char *file, int line, const Ego::Math::Sphere<EuclidianSpaceType>& object) const {
        static const MakeValidate<decltype(object.getCenter())> validateCenter;
        static const MakeValidate<decltype(object.getRadius())> validateRadius;
        validateCenter(file, line, object.getCenter());
        validateRadius(file, line, object.getRadius());
    }
};

template <typename EuclidianSpaceType>
struct Validate<Ego::Math::AxisAlignedCube<EuclidianSpaceType>> {
    void operator()(const char *file, int line, const Ego::Math::AxisAlignedCube<EuclidianSpaceType>& object) const {
        static const MakeValidate<decltype(object.getCenter())> validateCenter;
        static const MakeValidate<decltype(object.getSize())> validateSize;
        validateCenter(file, line, object.getCenter());
        validateSize(file, line, object.getSize());
    }
};

} // namespace Debug
} // namespace Ego
#endif

/// A \f$4\f$-order floating point matrix.
using Matrix4f4f = Ego::Math::Matrix<float, 4, 4>;

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
     *  =
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

    // Calculate matrix based on positions of grip points
    static inline Matrix4f4f fromFourPoints(const Vector3f& ori, const Vector3f& wid, const Vector3f& frw, const Vector3f& up, const float scale)
    {
        Vector3f vWid = wid - ori;
        Vector3f vUp = up - ori;
        Vector3f vFor = frw - ori;

        vWid.normalize();
        vUp.normalize();
        vFor.normalize();

        Matrix4f4f dst;
        dst(0, 0) = -scale * vWid[kX];  // HUK
        dst(1, 0) = -scale * vWid[kY];  // HUK
        dst(2, 0) = -scale * vWid[kZ];  // HUK
        dst(3, 0) = 0.0f;

        dst(0, 1) = scale * vFor[kX];
        dst(1, 1) = scale * vFor[kY];
        dst(2, 1) = scale * vFor[kZ];
        dst(3, 1) = 0.0f;

        dst(0, 2) = scale * vUp[kX];
        dst(1, 2) = scale * vUp[kY];
        dst(2, 2) = scale * vUp[kZ];
        dst(3, 2) = 0.0f;

        dst(0, 3) = ori[kX];
        dst(1, 3) = ori[kY];
        dst(2, 3) = ori[kZ];
        dst(3, 3) = 1.0f;

        return dst;
    }
};

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

#include "egolib/TrigonometricTable.hpp"

Matrix4f4f mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(const Vector3f& scale, const Facing& turn_z, const Facing& turn_x, const Facing& turn_y, const Vector3f& translate);
/// @details Transpose the SpaceFixed representation and invert the angles to get the BodyFixed representation
Matrix4f4f mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(const Vector3f& scale, const Facing& turn_z, const Facing& turn_x, const Facing& turn_y, const Vector3f& translate);


/**
 * @brief
 *  Dump a textual representation of a matrix to standard output.
 * @param a
 *  the matrix
 */
void dump_matrix(const Matrix4f4f& a);


/// @todo Remove this.
float fvec3_decompose(const Vector3f& src, const Vector3f& vnrm, Vector3f& vpara, Vector3f& vperp);




#pragma once

#include "egolib/Debug.hpp"
#include "egolib/Log/_Include.hpp"
#include "egolib/integrations/math.hpp"

/**
 * @brief
 *  Enumerated indices for the elements of vectors.
 * @todo
 *  Remove this.
 */
enum {
    kX = 0, kY, kZ, kW
};

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

#ifdef _DEBUG
namespace Ego {
namespace Debug {

template <>
struct Validate<float> {
    void operator()(const char *file, int line, float object) const {
        if (idlib::is_bad(object)) {
            auto e = Log::Entry::create(Log::Level::Error, file, line, "invalid floating point value", Log::EndOfEntry);
            Log::get() << e;
            throw std::runtime_error(e.getText());
        }
    }
};

template <typename VectorType>
struct Validate<idlib::point<VectorType>> {
    void operator()(const char *file, int line, const idlib::point<VectorType>& object) const {
        static const Validate<typename VectorType::ScalarType> validate{};
        for (size_t i = 0; i < VectorType::dimensionality(); ++i) {
            validate(file, line, object[i]);
        }
    }
};

template <typename ScalarType, size_t Dimensionality>
struct Validate<idlib::vector<ScalarType, Dimensionality>> {
    void operator()(const char *file, int line, const idlib::vector<ScalarType, Dimensionality>& object) const {
        static const Validate<ScalarType> validate{};
        for (size_t i = 0; i < Dimensionality; ++i) {
            validate(file, line, object[i]);
        }
    }
};

} // namespace Debug
} // namespace Ego
#endif

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
    static Ego::Matrix4f4f tensor(const Ego::Vector4f& v, const Ego::Vector4f& w) {
        return
            Ego::Matrix4f4f
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
    static Ego::Vector3f getTranslation(const Ego::Matrix4f4f& m) {
        return Ego::Vector3f(m(0, 3), m(1, 3), m(2, 3));
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
    static void transform(const Ego::Matrix4f4f& m, const Ego::Vector4f& source, Ego::Vector4f& target) {
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
    static void transform(const Ego::Matrix4f4f& m, const Ego::Vector4f sources[], Ego::Vector4f targets[], const size_t size) {
        const Ego::Vector4f *source = sources;
        Ego::Vector4f *target = targets;
        for (size_t index = 0; index < size; ++index) {
            transform(m, *source, *target);
            source++;
            target++;
        }
    }

    // Calculate matrix based on positions of grip points
    static inline Ego::Matrix4f4f fromFourPoints(const Ego::Vector3f& ori, const Ego::Vector3f& wid, const Ego::Vector3f& frw, const Ego::Vector3f& up, const float scale)
    {
        auto vWid = Ego::normalize(wid - ori).get_vector();
		auto vUp = Ego::normalize(up - ori).get_vector();
		auto vFor = Ego::normalize(frw - ori).get_vector();

        Ego::Matrix4f4f dst;
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
Ego::Vector3f mat_getChrUp(const Ego::Matrix4f4f& m);

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
Ego::Vector3f mat_getChrForward(const Ego::Matrix4f4f& m);

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
Ego::Vector3f mat_getChrRight(const Ego::Matrix4f4f& m);


Ego::Vector3f mat_getCamUp(const Ego::Matrix4f4f& m);
Ego::Vector3f mat_getCamRight(const Ego::Matrix4f4f& m);
Ego::Vector3f mat_getCamForward(const Ego::Matrix4f4f& m);


Ego::Vector3f mat_getTranslate(const Ego::Matrix4f4f& mat);

#include "egolib/TrigonometricTable.hpp"

Ego::Matrix4f4f mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(const Ego::Vector3f& scale, const Facing& turn_z, const Facing& turn_x, const Facing& turn_y, const Ego::Vector3f& translate);
/// @details Transpose the SpaceFixed representation and invert the angles to get the BodyFixed representation
Ego::Matrix4f4f mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(const Ego::Vector3f& scale, const Facing& turn_z, const Facing& turn_x, const Facing& turn_y, const Ego::Vector3f& translate);


/**
 * @brief
 *  Dump a textual representation of a matrix to standard output.
 * @param a
 *  the matrix
 */
void dump_matrix(const Ego::Matrix4f4f& a);


/// @todo Remove this.
float fvec3_decompose(const Ego::Vector3f& src, const Ego::Vector3f& vnrm, Ego::Vector3f& vpara, Ego::Vector3f& vperp);




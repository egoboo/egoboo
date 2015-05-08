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

/// @file   egolib/Math/Vector.hpp
/// @brief  2-,3- and 4-dimensional vectors.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/TemplateUtilities.hpp"
#include "egolib/typedef.h"
#include "egolib/log.h"
#include "egolib/Float.hpp"
#include "egolib/Debug.hpp"
#include "egolib/Math/Math.hpp"

namespace Ego
{
namespace Math
{

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <size_t I, typename Type, size_t Size>
void unpack(Type(&dst)[Size]);

template <size_t I, typename  Type, size_t Size, typename Arg>
void _unpack(Type(&dst)[Size], Arg&& arg) {
    dst[I] = arg;
}

template <size_t I, typename Type, size_t Size, typename Arg, class ... Args>
void _unpack(Type(&dst)[Size], Arg&& arg, Args&& ... args) {
    dst[I] = arg;
    _unpack<I + 1, Type>(dst, args ...);
}

/**
 * @brief
 *  You can use "unpack" for storing non-type parameter packs in arrays.
 * @invariant
 *  The number of arguments must be exactly the length of the array.
 * @invariant
 *  The argument types must be the array type.
 * @remark
 *  Example usage:
 *  @code
 *  float a[3]; unpack(a,0.5,0.1)
 *  @endcode
 * @author
 *  Michael Heilmann
 */
template <typename Type, size_t Size, typename ... Args>
void unpack(Type(&dst)[Size], Args&& ...args) {
    static_assert(Size == sizeof ... (args), "wrong number of arguments");
    _unpack<0, Type, Size>(dst, args ...);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief
 *  @a std::true_type is @a _ScalarType and @a _Dimensionality fulfil the requirements for an abstract vector,
 *  @a std::false_type otherwise.
 * @detail
 *  The requirements are: @a _ScalarType must be a floating point type and @a Dimensionality must be greater than @a 0.
 * @remark
 *  MH: The following simplification should be possible:
 *  @code
 *  template <typename _ScalarType, size_t _Dimensionality>
 *  using AbstractVectorEnable =
 *      typename std::conditional<
 *              (std::is_floating_point<_ScalarType>::value && Ego::Core::greater_than<_Dimensionality, 0>::value),
 *              std::true_type,
 *              std::false_type
 *          >::type
 *  @endcode
 */
template <typename _ScalarType, size_t _Dimensionality>
struct AbstractVectorEnable
    : public std::conditional<
    (std::is_floating_point<_ScalarType>::value && Ego::Core::GreaterThan<_Dimensionality, 0>::value),
    std::true_type,
    std::false_type
    >::type
{};

template <typename _ScalarType, size_t _Dimensionality, typename ... ArgTypes>
struct AbstractVectorConstructorEnable
    : public std::conditional<
    (Ego::Core::EqualTo<sizeof...(ArgTypes), _Dimensionality - 1>::value),
    std::true_type,
    std::false_type
    >::type
{};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename _ScalarType, size_t _Dimensionality, typename _Enabled = void>
struct AbstractVector;

template <typename _ScalarType, size_t _Dimensionality>
struct AbstractVector<_ScalarType, _Dimensionality, typename std::enable_if<AbstractVectorEnable<_ScalarType, _Dimensionality>::value>::type>
{

public:

    /**
     * @brief
     *  @a ScalarType is the type of the underlaying scalars.
     */
    typedef _ScalarType ScalarType;
    /**
     * @brief
     *  @a MyType is the type of the vector..
     */
    typedef AbstractVector<_ScalarType, _Dimensionality> MyType;
    /**
     * @brief
     *  The dimensionality of this vector.
     */
    static const size_t Dimensionality;

private:
    /// @invariant the scalar type must be a floating point type.
    static_assert(std::is_floating_point<_ScalarType>::value, "_ScalarType must be a floating point type");
    /// @invariant the dimensionality be a positive integral constant
    static_assert(std::integral_constant<size_t, _Dimensionality>::value > 0, "_Dimensionality must be a positive integral constant");
    /**
     * @brief
     *  The elements of this vector.
     */
    _ScalarType _elements[_Dimensionality];

public:

    /// @todo Only enable this
    /// a) if the parameter pack is of length N=_Dimensionality -1 and
    /// b) if all N parameters are convertible to _ScalarType.
    template<typename ... ArgTypes, typename = typename std::enable_if<AbstractVectorConstructorEnable<_ScalarType, _Dimensionality, ArgTypes ...>::value>::type>
    AbstractVector(_ScalarType v, ArgTypes&& ... args) {
        static_assert(_Dimensionality - 1 == sizeof ... (args), "wrong number of arguments");
        unpack<float, _Dimensionality>(_elements, std::forward<_ScalarType>(v), args ...);
    }

    AbstractVector() :
        _elements()
    {}

    AbstractVector(const MyType& other) {
        for (size_t i = 0; i < Dimensionality; ++i) {
            _elements[i] = other._elements[i];
        }
    }
    
public:

    /**
     * @brief
     *  Compute the dot product of this vector and another vector.
     * @param other
     *  the other vector
     * @return
     *  the dot product <tt>(*this) * other</tt> of this vector and the other vector
     */
    ScalarType dot(const MyType& other) const {
        ScalarType t = _elements[0] * other._elements[0];
        for (size_t i = 1; i < Dimensionality; ++i) {
            t += _elements[i] * other._elements[i];
        }
        return t;
    }
    
public:

    /** 
     * @brief
     *  Assign another vector to this vector.
     * @param other
     *  the other vector
     */
    void assign(const MyType& other) {
        for (size_t i = 0; i < Dimensionality; ++i) {
            _elements[i] = other._elements[i];
        }
    }

    /**
     * @brief
     *  Multiply this vector by a scalar,
     *  assign the result to this vector.
     * @param scalar
     *  the scalar
     * @post
     *  The product <tt>scalar * (*this)</tt> was assigned to <tt>*this</tt>.
     */
    void multiply(ScalarType scalar) {
        for (size_t i = 0; i < Dimensionality; ++i) {
            _elements[i] *= scalar;
        }
    }
    
    /**
     * @brief
     *  Add another vector to this vector,
     *  assign the result to this vector.
     * @param other
     *  the other vector
     * @post
     *  The sum <tt>(*this) + other</tt> was assigned to <tt>*this</tt>.
     */
    void add(const MyType& other) {
        for (size_t i = 0; i < Dimensionality; ++i) {
            _elements[i] += other._elements[i];
        }
    }

    /**
     * @brief
     *  Subtract another vector from this vector,
     *  assign the result to this vector.
     * @param other
     *  the other vector
     * @post
     *  The difference <tt>(*this) - other</tt> was assigned to <tt>*this</tt>.
     */
    void sub(const MyType& other) {
        for (size_t i = 0; i < Dimensionality; ++i) {
            _elements[i] -= other._elements[i];
        }
    }

    /**
     * @brief
     *  Normalize this vector to the specified length.
     * @param length
     *  the length
     * @post
     *  If <tt>*this</tt> is the null/zero vector, then <tt>*this</tt> was assigned the null/zero vector
     *  and is assigned <tt>length * (*this) / |(*this)|</tt> otherwise.
     */
    void normalize(ScalarType length) {
        ScalarType l = this->length();
        if (l > 0.0f) {
            multiply(length / l);
        }
    }

    /**
     * @brief
     *  Normalize this vector.
     * @return
     *  the old length of this vector
     * @post
     *  If <tt>*this</tt> is the null/zero vector, then <tt>*this</tt> was assigned the null/zero vector
     *  and is assigned <tt>(*this) / l</tt> (where @a l is the old length of <tt>(*this)</tt>) otherwise.
     */
    ScalarType normalize() {
        ScalarType l = length();
        if (l > 0.0f) {
            multiply(1.0f / l);
        }
        return l;
    }

    /**
     * @brief
     *  Get if this vector equals another vectors.
     * @param other
     *  the other vector
     * @return
     *  @a true if this vector equals the other vector
     */
    bool equals(const MyType& other) const {
        for (size_t i = 0; i < Dimensionality; ++i) {
            if (_elements[i] != other._elements[i]) {
                return false;
            }
        }
        return true;
    }

private:

    /** @internal */
    template <size_t ... Index>
    MyType abs(Ego::Core::index_sequence<Index ...>) const {
        return MyType(std::abs(_elements[Index]) ...);
    }

public:

    /**
     * @brief
     *  Get the component-wise absolute of this vector.
     * @return
     *  the component-wise absolute of this vector
     * @remark
     *  The component-wise absolute of a vector \f$v\in\mathbb{R}^n,n>0\f$ is defined as
     *  \f[
     *  abs(\vec{v}) = (abs(v_1),\ldots,abs(v_n))
     *  \f]
     */
    MyType abs() const {
        return abs(Ego::Core::make_index_sequence < _Dimensionality > {});
    }

private:

    /** @internal */
    template <size_t ... Index>
    MyType min(Ego::Core::index_sequence<Index ...>, const MyType& other) const {
        return MyType(std::min(_elements[Index],other._elements[Index]) ...);
    }

public:
    /**
     * @brief
     *    Get the component-wise minimum of this vector and another vector.
     * @param other
     *    the other vector
     * @return
     *    the component-wise minimum
     * @remark
     *    For two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^n,n>0\f$ the component-wise minimum is defined as
     *    \f[
     *    min\left(\vec{u},\vec{v}\right)=left(min(u_1,v_1),\ldots,min(u_n,v_n)\right)
     *    \f]
     */
    MyType min(const MyType& other) const {
        return min(Ego::Core::make_index_sequence<_Dimensionality>{}, other);
    }

private:

    /** @internal */
    template <size_t ... Index>
    MyType max(Ego::Core::index_sequence<Index ...>, const MyType& other) const {
        return MyType(std::max(_elements[Index],other._elements[Index]) ...);
    }

public:
    /**
     * @brief
     *    Get the component-wise maximum of this vector and another vector.
     * @param other
     *    the other vector
     * @return
     *    the component-wise maximum
     * @remark
     *    For two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^n,n>0\f$ the component-wise maximum is defined as
     *    \f[
     *    max\left(\vec{u},\vec{v}\right)=left(max(u_1,v_1),\ldots,max(u_n,v_n)\right)
     *    \f]
     */
    MyType max(const MyType& other) const {
        return max(Ego::Core::make_index_sequence<_Dimensionality>{},other);
    }

public:

    /**
     * @brief
     *  Get the squared length of this vector
     *  (using the Euclidian metric).
     * @return
     *  the squared length of this vector
     */
    ScalarType length_2() const {
        ScalarType t = _elements[0] * _elements[0];
        for (size_t i = 1; i < Dimensionality; ++i) {
            t += _elements[i] * _elements[i];
        }
        return t;
    }

    /**
     * @brief
     *  Get the length of this vector
     *  (using the Euclidian metric).
     * @return
     *  the length of this vector
     */
    ScalarType length() const {
        return std::sqrt(length_2());
    }

    /**
     * @brief
     *  Get the length of this vector
     *  (using the Manhattan metric).
     * @return
     *  the length of this vector
     */
    ScalarType length_abs() const {
        ScalarType t = std::abs(_elements[0]);
        for (size_t i = 1; i < Dimensionality; ++i) {
            t += std::abs(_elements[i]);
        }
        return t;
    }

    /**
     * @brief
     *  Get the length of this vector
     *  (using the Maximum metric).
     * @return
     *  the length of this vector
     */
    ScalarType length_max() const {
        return *std::max_element(_elements, _elements + Dimensionality);
    }

public:

    bool operator==(const MyType& other) const {
        return equals(other);
    }

    bool operator!=(const MyType& other) const {
        return !equals(other);
    }

public:

    const MyType& operator=(const MyType& other) {
        assign(other);
        return *this;
    }

public:

    MyType& operator+=(const MyType& other) {
        add(other);
        return *this;
    }

    MyType& operator-=(const MyType& other) {
        sub(other);
        return *this;
    }

    MyType& operator*=(ScalarType scalar) {
        multiply(scalar);
        return *this;
    }

public:

    MyType operator+(const MyType& other) const {
        MyType t(*this);
        t += other;
        return t;
    }
    
    MyType operator-(const MyType& other) const {
        MyType t(*this);
        t -= other;
        return t;
    }
    
    MyType operator*(ScalarType other) const {
        MyType t(*this);
        t *= other;
        return t;
    }


public:

    ScalarType& operator[](size_t const& index) {
    #ifdef _DEBUG
        EGOBOO_ASSERT(index < Dimensionality);
    #endif
        return _elements[index];
    }

    const ScalarType& operator[](size_t const& index) const {
    #ifdef _DEBUG
        EGOBOO_ASSERT(index < Dimensionality);
    #endif
        return _elements[index];
    }

    MyType operator-() const {
        return (*this) * -1;
    }


public:

    /**
     * @brief
     *  Get if this vector is a unit vector.
     * @return
     *  @a true if this vector is a unit vector, @a false otherwise
     */
    bool isUnit() const {
        ScalarType t = length_2();
        return 0.99 < t && t < 1.01;
    }

    /**
     * @brief
     *  Get if this vector is a zero vector.
     * @return
     *  @a true if this vector is a zero vector, @a false otherwise
     */
    bool isZero() const {
        ScalarType t = length();
        return t < 0.01f;
    }


public:

    /**
     * @brief
     *  Get the zero vector.
     * @return
     *  the zero vector
     */
    static const MyType& zero();

public:

    /**
     * @brief
     *  Compute the cross product of this vector and another vector.
     * @param other
     *  the other vector
     * @return
     *  the cross product <tt>(*this) x other</tt> of this vector and the other vector
     * @remark
     *  For any two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^3\f$ the cross product is defined as
     *  \f[
     *  \vec{u} \times \vec{v} =
     *  \left[\begin{matrix}
     *  u_y v_z - u_z v_y\\
     *  u_z v_x - u_x v_z\\
     *  u_x v_y - u_y v_x
     *  \end{matrix}\right]
     *  \f]
     * @remark
     *  The cross product is distributive of vector addition i.e.
     *  \f[
     *  \vec{u} \times \left(\vec{v} + \vec{w}\right) = \vec{u} \times \vec{v} + \vec{u} \times \vec{w}
     *  \f]
     *  holds for any three vector \f$\vec{u},\vec{v},\vec{w}\in\mathbb{R}^3\f$.
     *  This follows from
     *  \f[
     *  &\vec{u} \times (\vec{v} + \vec{w})\\
     * =&\left[\begin{matrix}
     *   u_y (v_z + w_z) - u_z (v_y + w_y)\\
     *   u_z (v_x + w_x) - u_x (v_z + w_z)\\
     *   u_x (v_y + w_y) - u_y (v_x + w_x)
     *   \end{matrix}\right]\\
     * =&\left[\begin{matrix}
     *   (u_y v_z - u_z v_y) + (u_y w_z - u_z w_y)\\
     *   (u_z v_x - u_x v_z) + (u_z w_x - u_x w_z)\\
     *   (u_x v_y - u_y v_x) + (u_x w_y - u_y w_x)
     *   \end{matrix}\right]\\
     * =&\left[\begin{matrix}
     *   u_y v_z - u_z v_y\\
     *   u_z v_x - u_x v_z\\
     *   u_x v_y - u_y v_x
     *   \end{matrix}\right]
     *  +
     *   \left[\begin{matrix}
     *   u_y w_z - u_z w_y\\
     *   u_z w_x - u_x w_z\\
     *   u_x w_y - u_y w_x
     *   \end{matrix}\right]\\
     * =&\vec{u} \times \vec{v} + \vec{u} \times \vec{w}
     *  \f]
     * @remark
     *  The cross product is compatible with scalar multiplication i.e.
     *  \f[
     *  \left(s\vec{u}\right) \times \vec{v} = \vec{u} \times \left(s\vec{v}\right) = s \left(\vec{u} \times \vec{v}\right)
     *  \f]
     *  holds for any two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^3\f$ and any scalar \f$s\in\mathbb{R}\f$.
     *  This follows from
     *  \f[
     *  \left(s\vec{u}\right) \times \vec{v}
     * =\left[\begin{matrix}
     *  (s u_y) v_z - (s u_z) v_y\\
     *  (s u_z) v_x - (s u_x) v_z\\
     *  (s u_x) v_y - (s u_y) v_x
     *  \end{matrix}\right]
     * =\left[\begin{matrix}
     *  u_y (s v_z) - u_z (s v_y)\\
     *  u_z (s v_x) - u_x (s v_z)\\
     *  u_x (s v_y) - u_y (s v_x)
     *  \end{matrix}\right]
     *  =\vec{u} \times (s \vec{v})
     *  \f]
     *  and
     *  \f[
     *  \left(s\vec{u}\right) \times \vec{v}
     * =\left[\begin{matrix}
     *  (s u_y) v_z - (s u_z) v_y\\
     *  (s u_z) v_x - (s u_x) v_z\\
     *  (s u_x) v_y - (s u_y) v_x
     *  \end{matrix}\right]
     * =s \left[\begin{matrix}
     *  u_y v_z - u_z v_y\\
     *  u_z v_x - u_x v_z\\
     *  u_x v_y - u_y v_x
     *  \end{matrix}\right]
     * = s (\vec{u} \times \vec{v})
     * @remark
     *  \f[
     *  \vec{v} \times \vec{v} = \vec{0}
     *  \f]
     *  holds by
     *  \f[
     *  \vec{v} \times \vec{v} =
     *  \left[\begin{matrix}
     *  v_y v_z - v_z v_y\\
     *  v_z v_x - v_x v_z\\
     *  v_x v_y - v_y v_x
     *  \end{matrix}\right]
     *  = \vec{0}
     *  \f]
     */
    template<size_t _Dummy = _Dimensionality>
    typename std::enable_if<_Dummy == 3 && _Dimensionality == 3, AbstractVector<_ScalarType, _Dimensionality>>::type
    cross(const MyType& other) const {
        return
            MyType
            (
                _elements[kY] * other._elements[kZ] - _elements[kZ] * other._elements[kY],
                _elements[kZ] * other._elements[kX] - _elements[kX] * other._elements[kZ],
                _elements[kX] * other._elements[kY] - _elements[kY] * other._elements[kX]
            );
    }
};

/**
 * @brief
 *  Get the zero vector.
 * @return
 *  the zero vector
 */
template <typename _ScalarType, size_t _Dimensionality>
const AbstractVector<_ScalarType, _Dimensionality>&
AbstractVector< _ScalarType, _Dimensionality, typename std::enable_if<AbstractVectorEnable<_ScalarType, _Dimensionality>::value>::type>::zero() {
    static const auto ZERO = AbstractVector<_ScalarType, _Dimensionality, typename std::enable_if<AbstractVectorEnable<_ScalarType, _Dimensionality>::value>::type>();
        return ZERO;
    }

template <typename _ScalarType, size_t _Dimensionality>
const size_t AbstractVector<_ScalarType, _Dimensionality, typename std::enable_if<AbstractVectorEnable<_ScalarType, _Dimensionality>::value>::type>::Dimensionality
    = _Dimensionality;

} // namespace Math
} // namespace Ego

/**
 * @brief
 *    Enumerated indices for the elements of vectors.
 */
enum { kX = 0, kY, kZ, kW };

/// A 2-vector type that allows more than one form of access.
typedef Ego::Math::AbstractVector<float, 2> fvec2_t;

#ifdef _DEBUG
namespace Ego
{
    namespace Debug
    {
        template <>
        void validate<::fvec2_t>(const char *file, int line, const ::fvec2_t& object);
    }
}
#endif

typedef float fvec3_base_t[3]; ///< the basic floating point 3-vector type

/// A 3-vector type that allows more than one form of access.
struct fvec3_t
{

    union
    {
        fvec3_base_t v;
        struct { float x, y, z; };
    };

    static const fvec3_t& zero()
    {
        static const fvec3_t ZERO = fvec3_t(0.0f, 0.0f, 0.0f);
        return ZERO;
    }

    fvec3_t() :
        x(), y(), z()
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
     *    Get the component-wise absolute of this vector.
     * @return
     *    the component-wise absolute of this vector
     * @remark
     *    The component-wise absolute of a vector \f$v\in\mathbb{R}^n,n>0\f$ is defined as
     *    \f[
     *    abs(\vec{v}) = (abs(v_1),\ldots,abs(v_n))
     *    \f]
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
     *    Get the component-wise minimum of this vector and another vector.
     * @param other
     *    the other vector
     * @return
     *    the component-wise minimum
     * @remark
     *    For two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^n,n>0\f$ the component-wise minimum is defined as
     *    \f[
     *    min\left(\vec{u},\vec{v}\right)=left(min(u_1,v_1),\ldots,min(u_n,v_n)\right)
     *    \f]
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
     *    Get the component-wise maximum of this vector and another vector.
     * @param other
     *    the other vector
     * @return
     *    the component-wise maximum
     * @remark
     *    For two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^n,n>0\f$ the component-wise maximum is defined as
     *    \f[
     *    max\left(\vec{u},\vec{v}\right)=left(max(u_1,v_1),\ldots,max(u_n,v_n)\right)
     *    \f]
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
     *    Compute the cross product of this vector and another vector.
     * @param other
     *    the other vector
     * @return
     *    the cross product <tt>(*this) x other</tt> of this vector and the other vector
     * @remark
     *  For any two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^3\f$ the cross product is defined as
     *  \f[
     *  \vec{u} \times \vec{v} =
     *  \left[\begin{matrix}
     *  u_y v_z - u_z v_y\\
     *  u_z v_x - u_x v_z\\
     *  u_x v_y - u_y v_x
     *  \end{matrix}\right]
     *  \f]
     * @remark
     *  The cross product is distributive of vector addition i.e.
     *  \f[
     *  \vec{u} \times \left(\vec{v} + \vec{w}\right) = \vec{u} \times \vec{v} + \vec{u} \times \vec{w}
     *  \f]
     *  holds for any three vector \f$\vec{u},\vec{v},\vec{w}\in\mathbb{R}^3\f$.
     *  This follows from
     *  \f[
     *  &\vec{u} \times (\vec{v} + \vec{w})\\
     * =&\left[\begin{matrix}
     *   u_y (v_z + w_z) - u_z (v_y + w_y)\\
     *   u_z (v_x + w_x) - u_x (v_z + w_z)\\
     *   u_x (v_y + w_y) - u_y (v_x + w_x) 
     *   \end{matrix}\right]\\
     * =&\left[\begin{matrix}
     *   (u_y v_z - u_z v_y) + (u_y w_z - u_z w_y)\\
     *   (u_z v_x - u_x v_z) + (u_z w_x - u_x w_z)\\
     *   (u_x v_y - u_y v_x) + (u_x w_y - u_y w_x)
     *   \end{matrix}\right]\\
     * =&\left[\begin{matrix}
     *   u_y v_z - u_z v_y\\
     *   u_z v_x - u_x v_z\\
     *   u_x v_y - u_y v_x
     *   \end{matrix}\right]
     *  +
     *   \left[\begin{matrix}
     *   u_y w_z - u_z w_y\\
     *   u_z w_x - u_x w_z\\
     *   u_x w_y - u_y w_x
     *   \end{matrix}\right]\\
     * =&\vec{u} \times \vec{v} + \vec{u} \times \vec{w}
     *  \f]
     * @remark
     *  The cross product is compatible with scalar multiplication i.e.
     *  \f[
     *  \left(s\vec{u}\right) \times \vec{v} = \vec{u} \times \left(s\vec{v}\right) = s \left(\vec{u} \times \vec{v}\right) 
     *  \f]
     *  holds for any two vectors \f$\vec{u},\vec{v}\in\mathbb{R}^3\f$ and any scalar \f$s\in\mathbb{R}\f$.
     *  This follows from
     *  \f[
     *  \left(s\vec{u}\right) \times \vec{v}
     * =\left[\begin{matrix}
     *  (s u_y) v_z - (s u_z) v_y\\
     *  (s u_z) v_x - (s u_x) v_z\\
     *  (s u_x) v_y - (s u_y) v_x
     *  \end{matrix}\right]
     * =\left[\begin{matrix}
     *  u_y (s v_z) - u_z (s v_y)\\
     *  u_z (s v_x) - u_x (s v_z)\\
     *  u_x (s v_y) - u_y (s v_x)
     *  \end{matrix}\right]
     *  =\vec{u} \times (s \vec{v}) 
     *  \f]
     *  and
     *  \f[
     *  \left(s\vec{u}\right) \times \vec{v}
     * =\left[\begin{matrix}
     *  (s u_y) v_z - (s u_z) v_y\\
     *  (s u_z) v_x - (s u_x) v_z\\
     *  (s u_x) v_y - (s u_y) v_x
     *  \end{matrix}\right]
     * =s \left[\begin{matrix}
     *  u_y v_z - u_z v_y\\
     *  u_z v_x - u_x v_z\\
     *  u_x v_y - u_y v_x
     *  \end{matrix}\right]
     * = s (\vec{u} \times \vec{v})
     * @remark
     *  \f[
     *  \vec{v} \times \vec{v} = \vec{0}
     *  \f]
     *  holds by
     *  \f[
     *  \vec{v} \times \vec{v} =
     *  \left[\begin{matrix}
     *  v_y v_z - v_z v_y\\
     *  v_z v_x - v_x v_z\\
     *  v_x v_y - v_y v_x
     *  \end{matrix}\right]
     *  = \vec{0}
     *  \f]
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
     *    Compute the dot product of this vector and another vector.
     * @param other
     *    the other vector
     * @return
     *    the dot product <tt>(*this) * other</tt> of this vector and the other vector
     */
    float dot(const fvec3_t& other) const
    {
        float dot = v[0] * other.v[0];
        for (size_t i = 1; i < 3; ++i)
        {
            dot += v[i] * other.v[i];
        }
        return dot;
    }

    /**
     * @brief
     *    Multiply this vector by a scalar.
     * @param scalar
     *    the scalar
     * @post
     *    The product <tt>scalar * (*this)</tt> was assigned to <tt>*this</tt>.
     */
    void multiply(float scalar)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            v[i] *= scalar;
        }
    }

    /**
     * @brief
     *    Normalize this vector to the specified length.
     * @param length
     *    the length
     * @post
     *    If <tt>*this</tt> is the null/zero vector, then <tt>*this</tt> was assigned the null/zero vector
     *    and is assigned <tt>length * (*this) / |(*this)|</tt> otherwise.
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
     *    Normalize this vector.
     * @return
     *    the <em>old</em> length of the vector
     * @post
     *    If <tt>*this</tt> is the null/zero vector, then <tt>*this</tt> was assigned the null/zero vector
     *    and is assigned <tt>(*this) / |(*this)|</tt> otherwise.
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
     *    Get if this vector equals another vectors.
     * @param other
     *    the other vector
     * @return
     *    @a true if this vector equals the other vector
     */
    bool equals(const fvec3_t& other) const
    {
        for (size_t i = 0; i < 3; ++i)
        {
            if (v[i] != other.v[i])
            {
                return false;
            }
        }
        return true;
    }

    /**
      * @brief
     *  Get the squared length of this vector
     *  (using the Euclidian metric).
     * @return
     *  the squared length of this vector
     */
    float length_2() const
    {
        float length = v[0] * v[0];
        for (size_t i = 1; i < 3; ++i)
        {
            length += v[i] * v[i];
        }
        return length;
    }

    /**
     * @brief
     *  Get the length of this vector
     *  (using the Euclidian metric).
     * @return
     *  the length of this vector
     */
    float length() const
    {
        return std::sqrt(length_2());
    }

    /**
     * @brief
     *  Get Euclidian distance along the xy-axes between this vector and another vectors.
     * @param other
     *  the other vector
     * @return
     *  the Euclidian distance along the xy-axes between this vector and the other vector
     */
    float xy_distance(const fvec3_t &other) const
    {
        return std::sqrt(Ego::Math::sq(x-other.x) + Ego::Math::sq(y-other.y));
    }

    /**
     * @brief
     *  Get the length of this vector
     *  (using the Manhattan metric).
     * @return
     *  the length of this vector
     */
    float length_abs() const
    {
        float length = std::abs(v[0]);
        for (size_t i = 1; i < 3; ++i)
        {
            length += std::abs(v[i]);
        }
        return length;
    }

    /**
     * @brief
     *  Get the length of this vector
     *  (using the Maximum metric).
     * @return
     *  the length of this vector
     */
    float length_max() const
    {
        float length = std::abs(v[0]);
        for (size_t i = 1; i < 3; ++i)
        {
            length = std::max(length, std::abs(v[i]));
        }
        return length;
    }

    const fvec3_t& operator=(const fvec3_t& other)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            v[i] = other.v[i];
        }
        return *this;
    }

    fvec3_t operator+(const fvec3_t& other) const
    {
        return fvec3_t(x + other.x, y + other.y, z + other.z);
    }

    fvec3_t& operator+=(const fvec3_t& other)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            v[i] += other.v[i];
        }
        return *this;
    }

    fvec3_t operator-(const fvec3_t& other) const
    {
        return fvec3_t(x - other.x, y - other.y, z - other.z);
    }

    fvec3_t& operator-=(const fvec3_t& other)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            v[i] -= other.v[i];
        }
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

    fvec3_t operator-() const
    {
        return fvec3_t(-x, -y, -z);
    }

    bool operator==(const fvec3_t& other) const
    {
        return equals(other);
    }

    bool operator!=(const fvec3_t& other) const
    {
        return !equals(other);
    }

    float& operator[](size_t const& index)
    {
#ifdef _DEBUG
        EGOBOO_ASSERT(index < 3);
#endif
        return v[index];
    }

    const float &operator[](size_t const& index) const
    {
#ifdef _DEBUG
        EGOBOO_ASSERT(index < 3);
#endif
        return v[index];
    }

    /**
     * @brief
     *  Get if this vector is a unit vector.
     * @return
     *  @a true if this vector is a unit vector, @a false otherwise
     */
    bool isUnit() const
    {
        float t = length();
        return 0.9f < t && t < 1.1f;
    }

    /**
     * @brief
     *  Get if this vector is a zero vector.
     * @return
     *  @a true if this vector is a zero vector, @a false otherwise
     */
    bool isZero() const
    {
        float t = length();
        return t < 0.01f;
    }

};

#ifdef _DEBUG
namespace Ego
{
    namespace Debug
    {
        template <>
        void validate<::fvec3_t>(const char *file, int line, const ::fvec3_t& object);
    }
}
#endif

/// A 4-vector type that allows more than one form of access.
typedef Ego::Math::AbstractVector<float, 4> fvec4_t;

#ifdef _DEBUG
namespace Ego
{
    namespace Debug
    {
        template <>
        void validate<::fvec4_t>(const char *file, int line, const ::fvec4_t& object);
    }
}
#endif

float fvec3_decompose(const fvec3_t& src, const fvec3_t& vnrm, fvec3_t& vpara, fvec3_t& vperp);

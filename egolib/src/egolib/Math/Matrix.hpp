//********************************************************************************************
//*
//*  This file is part of Egoboo.
//*
//*  Egoboo is free software: you can redistribute it and/or modify it
//*  under the terms of the GNU General Public License as published by
//*  the Free Software Foundation, either version 3 of the License, or
//*  (at your option) any later version.
//*
//*  Egoboo is distributed in the hope that it will be useful, but
//*  WITHOUT ANY WARRANTY; without even the implied warranty of
//*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*  General Public License for more details.
//*
//*  You should have received a copy of the GNU General Public License
//*  along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file   egolib/Math/Matrix.hpp
/// @brief  Generic matrix.
/// @author Michael Heilmann

#pragma once

#include "egolib/typedef.h"
#include "egolib/Math/TemplateUtilities.hpp"

/// @brief Egoboo uses a row-major matrix layout.
#define Ego_Math_Matrix_Layout_RowMajor (1)
#define Ego_Math_Matrix_Layout_ColumnMajor (2)
#define Ego_Math_Matrix_Layout Ego_Math_Matrix_Layout_RowMajor

namespace Ego {
namespace Math {

namespace Internal {

template <typename _ElementType, size_t _NumberOfRows, size_t _NumberOfColumns>
struct MatrixEnable
    : public std::conditional <
    (std::is_floating_point<_ElementType>::value) &&
    (_NumberOfRows > 0) &&
    (_NumberOfColumns > 0),
    std::true_type,
    std::false_type
    >::type
{};

/**
 * @brief
 *  Derived from @a std::true_type if @a _ElementType, @a _NumberOfRows, @a _NumberOfColumns and @a ArgTypes
 *  fulfil the requirements for a constructor of a matrix, and derived from @a std::false_type otherwise.
 * @param _ElementType
 *  the element type of the matrix type
 * @param _NumberOfRows
 *  the number of rows of the matrix type
 * @param _NumberOfColumns
 *  the number of columns of the matrix type
 * @param _ArgTypes
 *  @a ArgTypes must have <tt>_NumberOfRows * _NumberOfColumns - 1</tt> elements which are all convertible
 *  into values of type @a _ElementType
 * @author
 *  Michael Heilmann
 * @todo
 *  Fast-fail if the parameters are not convertible into @a _VectorSpaceType::ScalarType.
 */
template <typename _ElementType, size_t _NumberOfRows, size_t _NumberOfColumns, typename ... ArgTypes>
struct MatrixConstructorEnable
    : public std::conditional
             <
               sizeof...(ArgTypes) == _NumberOfRows * _NumberOfColumns - 1,
               std::true_type,
               std::false_type
             >::type
{};

} // namespace Internal

template <typename _ElementType, size_t _NumberOfRows, size_t _NumberOfColumns, typename _Enabled = void>
struct Matrix;

template <typename _ElementType, size_t _NumberOfRows, size_t _NumberOfColumns>
struct Matrix<_ElementType, _NumberOfRows, _NumberOfColumns,
              std::enable_if_t<Internal::MatrixEnable<_ElementType, _NumberOfRows, _NumberOfColumns>::value>>
    : public id::equal_to_expr<Matrix<_ElementType, _NumberOfRows, _NumberOfColumns>>,
      public id::plus_expr<Matrix<_ElementType, _NumberOfRows, _NumberOfColumns>>,
      public id::minus_expr<Matrix<_ElementType, _NumberOfRows, _NumberOfColumns>>,
      public id::unary_plus_expr<Matrix<_ElementType, _NumberOfRows, _NumberOfColumns>>,
      public id::unary_minus_expr<Matrix<_ElementType, _NumberOfRows, _NumberOfColumns>> {
    /**
     * @brief
     *  Get if this matrix type is square matrix type.
     * @return
     *  @a true if this matrix type is a square matrix type,
     *  @a false otherwise
     */
    constexpr static bool isSquare() {
        return numberOfRows() == numberOfColumns();
    }

    /**
     * @brief
     *  Get the the number of rows of this type of matrix.
     * @return
     *  the number of rows of this type of matrix
     */
    constexpr static size_t numberOfRows() {
        return _NumberOfRows;
    }

    /**
     * @brief
     *  Get the number of columns of this type of matrix.
     * @return
     *  the number of columns of this type of matrix
     */
    constexpr static size_t numberOfColumns() {
        return _NumberOfColumns;
    }

    /**
     * @brief
     *  Get the number of elements of this type of matrix
     *  i.e. the product of its number of columns and its number of rows.
     * @return
     *  the number of elements of this type of matrix
     */
    constexpr static size_t numberOfElements() {
        return numberOfRows() * numberOfColumns();
    }

    /**
     * @brief
     *  @a MyType is the type of this template/template specialization.
     */
    using MyType = Matrix<_ElementType, _NumberOfRows, _NumberOfColumns>;

    /**
     * @brief
     *  The element type.
     */
    using ElementType = _ElementType;

    union {
        /**@{*/
        _ElementType _v[numberOfElements()];
        /**
         * @brief
         *  The union of a two-dimensional array and a one-dimensional array.
         * @remark
         *  C++ has a native row-major order:
         *  A two dimensional array \f$a_{n,m}\f$ is layed out in memory
         *  \f{align*}{
         *  a_{0,0}   a_{0,1}   a_{0,2}   a_{0,3}   \ldots a_{0,m-2}   a_{0,m-1}
         *  a_{1,0}   a_{1,1}   a_{1,2}   a_{1,3}   \ldots a_{1,m-2}   a_{1,m-1}  \
         *  \vdots
         *  a_{n-2,0} a_{n-2,1} a_{n-2,2} a_{n-2,3} \ldots a_{n-2,m-2} a_{n-2,m-1}
         *  a_{n-1,0} a_{n-1,1} a_{n-1,2} a_{n-1,3} \ldots a_{n-1,m-2} a_{n-1,m-1}
         *  \}
         *  and an one dimensional array \f$a_{n \cdot m}\f$ is layed out in memory as
         *  \f{align*}{
         *  a_{0} a_{1} a_{2} a_{3} \ldots a_{n \cdot m - 2} a_{n \cdot m - 1}
         *  \f}
         *  The element \f$a_{i,j}\f$ of the two dimensional array maps
         *  to the element \f$a_{i * m + j}\f$ of the one-dimensional array.
         *
         *  The two dimensional array index \f$(i,j)\f$ maps to the one dimensional array index \f$(i \cdot m + j)\f$.
         *  The one dimensional array index \f$(k)\f$ maps to the two dimensional array index \f$(\lfloor k / m \rfloor, k \mod m)\f$.
         */
        _ElementType _v2[numberOfRows()][numberOfColumns()];
        /**@}*/
    };

    /**
     * @brief
     *  Get the identity (aka multiplicative neutral) matrix.
     * @return
     *  the identity matrix
     * @remark
     *  The \f$4 \times 4\f$ identity (aka multiplicative neutral) matrix is defined as
     *  \f[
     *  \left[\begin{matrix}
     *  1 & 0 & 0 & 0 \\
     *  0 & 1 & 0 & 0 \\
     *  0 & 0 & 1 & 0 \\
     *  0 & 0 & 0 & 1
     *  \end{matrix}\right]
     *  \f]
     */
    static std::enable_if_t<MyType::isSquare(), MyType>
    identity() {
        MyType identity = zero();
        for (size_t i = 0; i < order(); ++i) {
            identity(i, i) = 1.0f;
        }
        return identity;
    }

    /**
     * @brief
     *  Get the zero (aka additive neutral) matrix.
     * @return
     *  the zero matrix
     * @remark
     *  The \f$4 \times 4\f$ zero (aka additive neutral) matrix is defined as
     *  \f[
     *  \left[\begin{matrix}
     *  0 & 0 & 0 & 0 \\
     *  0 & 0 & 0 & 0 \\
     *  0 & 0 & 0 & 0 \\
     *  0 & 0 & 0 & 0
     *  \end{matrix}\right]
     *  \f]
     */
    static const MyType& zero() {
        static const MyType zero;
        return zero;
    }

    template<typename ... ArgTypes, typename = std::enable_if_t<Internal::MatrixConstructorEnable<_ElementType, _NumberOfRows, _NumberOfColumns, ArgTypes ...>::value>>
    Matrix(_ElementType v, ArgTypes&& ... args) {
        static_assert(_NumberOfRows * _NumberOfColumns - 1 == sizeof ... (args), "wrong number of arguments");
        Internal::unpack<_ElementType, _NumberOfRows * _NumberOfColumns>(_v, std::forward<_ElementType>(v), args ...);
    }

    /**
     * @brief
     *  Construct this matrix with default element values.
     */
    Matrix() {
        for (size_t i = 0; i < numberOfElements(); ++i) {
            _v[i] = _ElementType();
        }
    }

    /**
     * @brief
     *  Construct this matrix with the element values of another matrix.
     * @param other
     *  the other matrix
     */
    Matrix(const MyType& other) {
        for (size_t i = 0; i < numberOfElements(); ++i) {
            _v[i] = other._v[i];
        }
    }


protected:

    /**
     * @brief
     *  Unary minus.
     */
    void arithmeticNegation() {
        for (size_t i = 0; i < numberOfElements(); ++i) {
            at(i) = -at(i);
        }
    }

    /**
     * @brief
     *  Multiply this matrix with a scalar.
     * @param other
     *  the scalar
     * @post
     *  This matrix was assigned the product of this matrix and the scalar
     */
    void mul(const ElementType& other) {
        for (size_t i = 0; i < numberOfElements(); ++i) {
            at(i) *= other;
        }
    }

    /**
     * @brief
     *  Assign this matrix with the values of another matrix.
     * @param other
     *  the other matrix
     * @post
     *  This matrix was assigned the values of another matrix.
     */
    void assign(const MyType& other) {
        for (size_t i = 0; i < numberOfElements(); ++i) {
            _v[i] = other._v[i];
        }
    }

    /**
     * @brief
     *  Get the matrix element at the specified index.
     * @param i
     *  the index
     * @return
     *  the matrix element
     */
    ElementType& at(const size_t i) {
    #ifdef _DEBUG
        EGOBOO_ASSERT(i < numberOfElements());
    #endif
        return _v[i];
    }

    /**
     * @brief
     *  Get the matrix element at the specified index.
     * @param i
     *  the index
     * @return
     *  the matrix element
     */
    const ElementType& at(const size_t i) const {
    #ifdef _DEBUG
        EGOBOO_ASSERT(i < numberOfElements());
    #endif
        return _v[i];
    }

    /**
     * @brief
     *  Get the matrix element at the specified index.
     * @param i
     *  the row index
     * @param j
     *  the column index
     * @return
     *  the matrix element
     */
    ElementType& at(const size_t i, const size_t j) {
    #ifdef _DEBUG
        EGOBOO_ASSERT(i < numberOfRows());
        EGOBOO_ASSERT(j < numberOfColumns());
    #endif
        return _v2[i][j];
    }

    /**
     * @brief
     *  Get the matrix element at the specified index.
     * @param i
     *  the row index
     * @param j
     *  the column index
     * @return
     *  the matrix element
     */
    const ElementType& at(const size_t i, const size_t j) const {
    #ifdef _DEBUG
        EGOBOO_ASSERT(i < numberOfRows());
        EGOBOO_ASSERT(j < numberOfColumns());
    #endif
        return _v2[i][j];
    }

public:
    // CRTP
    bool equal_to(const MyType& other) const {
        for (size_t i = 0; i < numberOfElements(); ++i) {
            if (at(i) != other.at(i)) {
                return false;
            }
        }
        return true;
    }

    // CRTP
    void add(const MyType& other)
    {
        for (size_t i = 0; i < numberOfElements(); ++i)
        {
            at(i) += other.at(i);
        }
    }

    // CRTP
    void subtract(const MyType& other)
    {
        for (size_t i = 0; i < numberOfElements(); ++i)
        {
            at(i) -= other.at(i);
        }
    }

    // CRTP
    MyType unary_plus() const
    {
        for (size_t i = 0; i < numberOfElements(); ++i)
        {
            at(i) = +at(i);
        }
    }

    // CRTP
    MyType unary_minus() const
    {
        for (size_t i = 0; i < numberOfElements(); ++i)
        {
            at(i) = -at(i);
        }
    }

public:

    /**
     * @brief
     *  Get the order of this type of matrix.
     * @return
     *  the order of this type of matrix
     */
    constexpr static std::enable_if_t<MyType::isSquare(),size_t>
    order() {
        return _NumberOfRows;
    }

    /**
     * @brief
     *  Compute the trace of a square matrix.
     * @return
     *  the trace of a square matrix
     * @remark
     *  The trace of a square matrix \f$M\f$ of order \f$n\f$ is defined as
     *  \f[
     *  sum_{i=0}^{n-1} M_{i,i}
     *  \f]
     */
    std::enable_if_t<MyType::isSquare(), ElementType>
    trace() const {
        ElementType t = 0;
        for (size_t i = 0; i < order(); ++i) {
            t += at(i, i);
        }
        return t;
    }

    /**
     * @brief
     *  The type of transposed matrices of this matrix type.
     */
    using TransposeType = Matrix<ElementType, MyType::numberOfColumns(), MyType::numberOfRows()>;

    /**
     * @brief
     *  Compute the transpose of a matrix.
     * @param a
     *  the matrix
     * @return
     *  the transpose <tt>a^T</tt>
     * @remark
     *  The transpose \f$M^T\f$ of a matrix \f$M\f$ is defined as
     *  \f[
     *  M^T_{j,i} = M_{i,j}
     *  \f]
     */
    TransposeType getTranspose() const {
        TransposeType result;
        for (size_t i = 0; i < numberOfRows(); ++i) {
            for (size_t j = 0; j < numberOfColumns(); ++j) {
                result.at(j, i) = at(i, j);
            }
        }
        return result;
    }

public:

    const ElementType& operator()(const size_t i) const {
        return at(i);
    }

    ElementType& operator()(const size_t i) {
        return at(i);
    }

    const ElementType& operator()(const size_t i, const size_t j) const {
        return at(i, j);
    }

    ElementType& operator()(const size_t i, const size_t j) {
        return at(i, j);
    }

public:

    /**
     * @brief
     *  Assign this matrix the values of another matrix.
     * @param other
     *  the other matrix
     * @return
     *  this matrix
     * @post
     *  This matrix was assigned the values of another matrix.
     */
    MyType& operator=(const MyType& other) {
        assign(other);
        return *this;
    }

public:

    /**
     * @brief
     *  Overloaded multiplication operator.
     */
    MyType operator*(const ElementType& scalar) const {
        MyType result = *this;
        return result *= scalar;
    }

    /**
     * @brief
     *  Overloaded assignment multiplication operator.
     */
    MyType& operator*=(const ElementType& other) {
        mul(other);
        return *this;
    }

    /**
     * @brief
     *  Compute the product of this matrix and another matrix.
     * @param other
     *  the other matrix
     * @return
     *  the product <tt>(*this) * other</tt>
     * @remark
     *  The product \f$C = A \cdot B\f$ of two \f$4 \times 4\f$ matrices \f$A\f$ and \f$B\f$ is defined as
     *  \f[
     *  C_{i,j} = \sum_{i=0}^3 A_{i,k} \cdot B_{k,j}
     *  \f]
     */
    template <size_t _OtherNumberOfColumns>
    Matrix<ElementType, _NumberOfRows, _OtherNumberOfColumns>
    mul(const Matrix<ElementType, _NumberOfColumns, _OtherNumberOfColumns>& other) const {
        Matrix<ElementType, _NumberOfRows, _OtherNumberOfColumns> result;
        for (size_t i = 0; i < _NumberOfRows; ++i) {
            for (size_t j = 0; j < _OtherNumberOfColumns; ++j) {
                for (size_t k = 0; k < _NumberOfColumns; ++k) {
                    result(i, j) += (*this)(i, k) * other(k, j);
                }
            }
        }
        return result;
    }

    /**
     * Overloaded multiplication operator.
     */
    template <size_t _OtherNumberOfColumns>
    Matrix<_ElementType, _NumberOfRows, _OtherNumberOfColumns>
    operator*(const Matrix<_ElementType, _NumberOfColumns, _OtherNumberOfColumns>& other) const {
        return mul(other);
    }

    /**
     * Overloaded assignment multiplication operator.
     */
    MyType& operator*=(const MyType& other) {
        *this = mul(other);
        return *this;
    }

    /**
    * @brief
    *   Calculates the determinant for a matrix. Fully optimized and inlined 
    *   for 2x2, 3x3 and 4x4 matrices.
    * @source
    *   http://www.euclideanspace.com/maths/algebra/matrix/functions/determinant/fourD/index.htm
    **/
    ElementType det() const {
        static_assert(MyType::isSquare(), "Determinant on non-square matrix");

        if(MyType::numberOfColumns() == 2 && MyType::numberOfRows() == 2) {
            return _v2[0][0] * _v2[1][1] - _v2[0][1] * _v2[1][0];
        }

        if(MyType::numberOfColumns() == 3 && MyType::numberOfRows() == 3) {
            return _v2[0][0]*(_v2[1][1]*_v2[2][2]-_v2[2][1]*_v2[1][2]) -
                   _v2[0][1]*(_v2[1][0]*_v2[2][2]-_v2[1][2]*_v2[2][0]) +
                   _v2[0][2]*(_v2[1][0]*_v2[2][1]-_v2[1][1]*_v2[2][0]);            
        }

        if(MyType::numberOfColumns() == 4 && MyType::numberOfRows() == 4) {
          return
             _v2[0][3] * _v2[1][2] * _v2[2][1] * _v2[3][0] - _v2[0][2] * _v2[1][3] * _v2[2][1] * _v2[3][0] -
             _v2[0][3] * _v2[1][1] * _v2[2][2] * _v2[3][0] + _v2[0][1] * _v2[1][3] * _v2[2][2] * _v2[3][0] +
             _v2[0][2] * _v2[1][1] * _v2[2][3] * _v2[3][0] - _v2[0][1] * _v2[1][2] * _v2[2][3] * _v2[3][0] -
             _v2[0][3] * _v2[1][2] * _v2[2][0] * _v2[3][1] + _v2[0][2] * _v2[1][3] * _v2[2][0] * _v2[3][1] +
             _v2[0][3] * _v2[1][0] * _v2[2][2] * _v2[3][1] - _v2[0][0] * _v2[1][3] * _v2[2][2] * _v2[3][1] -
             _v2[0][2] * _v2[1][0] * _v2[2][3] * _v2[3][1] + _v2[0][0] * _v2[1][2] * _v2[2][3] * _v2[3][1] +
             _v2[0][3] * _v2[1][1] * _v2[2][0] * _v2[3][2] - _v2[0][1] * _v2[1][3] * _v2[2][0] * _v2[3][2] -
             _v2[0][3] * _v2[1][0] * _v2[2][1] * _v2[3][2] + _v2[0][0] * _v2[1][3] * _v2[2][1] * _v2[3][2] +
             _v2[0][1] * _v2[1][0] * _v2[2][3] * _v2[3][2] - _v2[0][0] * _v2[1][1] * _v2[2][3] * _v2[3][2] -
             _v2[0][2] * _v2[1][1] * _v2[2][0] * _v2[3][3] + _v2[0][1] * _v2[1][2] * _v2[2][0] * _v2[3][3] +
             _v2[0][2] * _v2[1][0] * _v2[2][1] * _v2[3][3] - _v2[0][0] * _v2[1][2] * _v2[2][1] * _v2[3][3] -
             _v2[0][1] * _v2[1][0] * _v2[2][2] * _v2[3][3] + _v2[0][0] * _v2[1][1] * _v2[2][2] * _v2[3][3];
        }

        //TODO: Not implemented for NxN matrices
        throw std::logic_error("Determinant for bigger than 4x4 matrixes not implemented");
    }

    MyType inverse() const {
        MyType result;

        //Inversion of 2x2 matrix is trivial
        if(MyType::numberOfColumns() == 2 && MyType::numberOfRows() == 2) {
            const ElementType determinant = det();
            result._v2[0][0] =  _v2[1][1] / determinant;
            result._v2[0][1] = -_v2[0][1] / determinant;
            result._v2[1][0] = -_v2[1][0] / determinant;
            result._v2[1][1] =  _v2[0][0] / determinant;
            return result;
        }

        //Optimized 3x3 matrix inversion
        if(MyType::numberOfColumns() == 3 && MyType::numberOfRows() == 3) {
            const ElementType determinant = det();
            result._v2[0][0] = (_v2[1][1] * _v2[2][2] - _v2[2][1] * _v2[1][2]) / determinant;
            result._v2[0][1] = (_v2[0][2] * _v2[2][1] - _v2[0][1] * _v2[2][2]) / determinant;
            result._v2[0][2] = (_v2[0][1] * _v2[1][2] - _v2[0][2] * _v2[1][1]) / determinant;
            result._v2[1][0] = (_v2[1][2] * _v2[2][0] - _v2[1][0] * _v2[2][2]) / determinant;
            result._v2[1][1] = (_v2[0][0] * _v2[2][2] - _v2[0][2] * _v2[2][0]) / determinant;
            result._v2[1][2] = (_v2[1][0] * _v2[0][2] - _v2[0][0] * _v2[1][2]) / determinant;
            result._v2[2][0] = (_v2[1][0] * _v2[2][1] - _v2[2][0] * _v2[1][1]) / determinant;
            result._v2[2][1] = (_v2[2][0] * _v2[0][1] - _v2[0][0] * _v2[2][1]) / determinant;
            result._v2[2][2] = (_v2[0][0] * _v2[1][1] - _v2[1][0] * _v2[0][1]) / determinant;
            return result;
        }

        //Optimized 4x4 matrix inversion (from MESA implementation of the GLU library)
        if(MyType::numberOfColumns() == 4 && MyType::numberOfRows() == 4) {
            result._v[0] = _v[5]  * _v[10] * _v[15] - 
                           _v[5]  * _v[11] * _v[14] - 
                           _v[9]  * _v[6]  * _v[15] + 
                           _v[9]  * _v[7]  * _v[14] +
                           _v[13] * _v[6]  * _v[11] - 
                           _v[13] * _v[7]  * _v[10];

            result._v[4] = -_v[4]  * _v[10] * _v[15] + 
                            _v[4]  * _v[11] * _v[14] + 
                            _v[8]  * _v[6]  * _v[15] - 
                            _v[8]  * _v[7]  * _v[14] - 
                            _v[12] * _v[6]  * _v[11] + 
                            _v[12] * _v[7]  * _v[10];

            result._v[8] = _v[4]  * _v[9] * _v[15] - 
                           _v[4]  * _v[11] * _v[13] - 
                           _v[8]  * _v[5] * _v[15] + 
                           _v[8]  * _v[7] * _v[13] + 
                           _v[12] * _v[5] * _v[11] - 
                           _v[12] * _v[7] * _v[9];

            result._v[12] = -_v[4]  * _v[9] * _v[14] + 
                             _v[4]  * _v[10] * _v[13] +
                             _v[8]  * _v[5] * _v[14] - 
                             _v[8]  * _v[6] * _v[13] - 
                             _v[12] * _v[5] * _v[10] + 
                             _v[12] * _v[6] * _v[9];

            result._v[1] = -_v[1]  * _v[10] * _v[15] + 
                            _v[1]  * _v[11] * _v[14] + 
                            _v[9]  * _v[2] * _v[15] - 
                            _v[9]  * _v[3] * _v[14] - 
                            _v[13] * _v[2] * _v[11] + 
                            _v[13] * _v[3] * _v[10];

            result._v[5] = _v[0]  * _v[10] * _v[15] - 
                           _v[0]  * _v[11] * _v[14] - 
                           _v[8]  * _v[2] * _v[15] + 
                           _v[8]  * _v[3] * _v[14] + 
                           _v[12] * _v[2] * _v[11] - 
                           _v[12] * _v[3] * _v[10];

            result._v[9] = -_v[0]  * _v[9] * _v[15] + 
                            _v[0]  * _v[11] * _v[13] + 
                            _v[8]  * _v[1] * _v[15] - 
                            _v[8]  * _v[3] * _v[13] - 
                            _v[12] * _v[1] * _v[11] + 
                            _v[12] * _v[3] * _v[9];

            result._v[13] = _v[0]  * _v[9] * _v[14] - 
                            _v[0]  * _v[10] * _v[13] - 
                            _v[8]  * _v[1] * _v[14] + 
                            _v[8]  * _v[2] * _v[13] + 
                            _v[12] * _v[1] * _v[10] - 
                            _v[12] * _v[2] * _v[9];

            result._v[2] = _v[1]  * _v[6] * _v[15] - 
                           _v[1]  * _v[7] * _v[14] - 
                           _v[5]  * _v[2] * _v[15] + 
                           _v[5]  * _v[3] * _v[14] + 
                           _v[13] * _v[2] * _v[7] - 
                           _v[13] * _v[3] * _v[6];

            result._v[6] = -_v[0]  * _v[6] * _v[15] + 
                            _v[0]  * _v[7] * _v[14] + 
                            _v[4]  * _v[2] * _v[15] - 
                            _v[4]  * _v[3] * _v[14] - 
                            _v[12] * _v[2] * _v[7] + 
                            _v[12] * _v[3] * _v[6];

            result._v[10] = _v[0]  * _v[5] * _v[15] - 
                            _v[0]  * _v[7] * _v[13] - 
                            _v[4]  * _v[1] * _v[15] + 
                            _v[4]  * _v[3] * _v[13] + 
                            _v[12] * _v[1] * _v[7] - 
                            _v[12] * _v[3] * _v[5];

            result._v[14] = -_v[0]  * _v[5] * _v[14] + 
                             _v[0]  * _v[6] * _v[13] + 
                             _v[4]  * _v[1] * _v[14] - 
                             _v[4]  * _v[2] * _v[13] - 
                             _v[12] * _v[1] * _v[6] + 
                             _v[12] * _v[2] * _v[5];

            result._v[3] = -_v[1] * _v[6] * _v[11] + 
                            _v[1] * _v[7] * _v[10] + 
                            _v[5] * _v[2] * _v[11] - 
                            _v[5] * _v[3] * _v[10] - 
                            _v[9] * _v[2] * _v[7] + 
                            _v[9] * _v[3] * _v[6];

            result._v[7] = _v[0] * _v[6] * _v[11] - 
                           _v[0] * _v[7] * _v[10] - 
                           _v[4] * _v[2] * _v[11] + 
                           _v[4] * _v[3] * _v[10] + 
                           _v[8] * _v[2] * _v[7] - 
                           _v[8] * _v[3] * _v[6];

            result._v[11] = -_v[0] * _v[5] * _v[11] + 
                             _v[0] * _v[7] * _v[9] + 
                             _v[4] * _v[1] * _v[11] - 
                             _v[4] * _v[3] * _v[9] - 
                             _v[8] * _v[1] * _v[7] + 
                             _v[8] * _v[3] * _v[5];

            result._v[15] = _v[0] * _v[5] * _v[10] - 
                            _v[0] * _v[6] * _v[9] - 
                            _v[4] * _v[1] * _v[10] + 
                            _v[4] * _v[2] * _v[9] + 
                            _v[8] * _v[1] * _v[6] - 
                            _v[8] * _v[2] * _v[5];

            //Efficient determinant calculation
            ElementType det = _v[0] * result._v[0] + _v[1] * result._v[4] + _v[2] * result._v[8] + _v[3] * result._v[12];
            if (det == 0) {
                throw std::domain_error("Cannot inverse matrix with 0 determinant");
            }

            //Finally multiply by the inverse determinant
            det = static_cast<ElementType>(1) / det;
            for (size_t i = 0; i < 16; i++) {
                result[i] = result._v[i] * det;
            }

            return result;
        }

        //TODO: Not implemented for NxN matrices
        throw std::logic_error("Inverse for bigger than 4x4 matrixes not implemented");
    }
};

} // namespace Math
} // namespace Ego

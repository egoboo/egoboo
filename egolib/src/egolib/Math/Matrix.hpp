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

/// @brief Egoboo currently uses column-major format. This will change to column major.
#define fmat_4x4_layout_RowMajor (1)
#define fmat_4x4_layout_ColumnMajor (2)
#define fmat_4x4_layout fmat_4x4_layout_ColumnMajor

namespace Ego {
namespace Math {

namespace Internal {

template <typename _ElementType, size_t _Rows, size_t _Columns>
struct MatrixEnable
    : public std::conditional <
    (std::is_floating_point<_ElementType>::value) &&
    (Ego::Core::GreaterThan<_Rows, 0>::value) &&
    (Ego::Core::GreaterThan<_Columns, 0>::value),
    std::true_type,
    std::false_type
    >::type
{};

} // namespace Internal

template <typename _ElementType, size_t _Rows, size_t _Columns, typename _Enabled = void>
struct Matrix;

template <typename _ElementType, size_t _Rows, size_t _Columns>
struct Matrix<_ElementType, _Rows, _Columns, typename std::enable_if<Internal::MatrixEnable<_ElementType, _Rows, _Columns>::value>::type> {

    /**
     * @brief
     *  @a MyType is the type of this template/template specialization.
     */
    typedef Matrix<_ElementType, _Rows, _Columns> MyType;

    /**
     * @brief
     *  The element type.
     */
    typedef _ElementType ScalarType;

    union {
        /**@{*/
        _ElementType v[_Rows * _Columns];
        /**
         * @brief
         *  The union of a two-dimensional array and a one-dimensional array.
         * @remark
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
        _ElementType v2[_Rows][_Columns];
        /**@}*/
    };

    /**
     * @brief
     *  Compute the sum of this matrix (the augend) and another matrix (the addend), assign there result to this matrix.
     * @param other
     *  the other matrix, the addend
     * @post
     *  This matrix was assigned the sum of this matrix and the other matrix.
     */
    void add(const MyType& other) {
        for (size_t i = 0; i < _Rows * _Columns; ++i) {
            at(i) += other.at(i);
        }
    }

    /**
     * @brief
     *  Compute the difference of this matrix (the minuend) and another matrix (the subtrahend), assign the result to this matrix.
     * @param other
     *  the other matrix, the subtrahend
     * @post
     *  This matrix was assigned the difference of this matrix and the other matrix.
     */
    void sub(const MyType& other) {
        for (size_t i = 0; i < _Rows * _Columns; ++i) {
            at(i) -= other.at(i);
        }
    }

    /**
     * @brief
     *  Assign this matrix with the values of another matrix.
     * @param other
     *  the other matrix
     */
    void assign(const MyType& other) {
        for (size_t i = 0; i < _Rows * _Columns; ++i) {
            v[i] = other.v[i];
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
    ScalarType& at(const size_t i) {
    #ifdef _DEBUG
        EGOBOO_ASSERT(i < _Rows * _Columns);
    #endif
        return v[i];
    }

    /**
     * @brief
     *  Get the matrix element at the specified index.
     * @param i
     *  the index
     * @return
     *  the matrix element
     */
    const ScalarType& at(const size_t i) const {
    #ifdef _DEBUG
        EGOBOO_ASSERT(i < _Rows * _Columns);
    #endif
        return v[i];
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
    ScalarType& at(const size_t i, const size_t j) {
    #ifdef _DEBUG
        EGOBOO_ASSERT(i < _Rows);
        EGOBOO_ASSERT(j < _Columns);
    #endif
    #if fmat_4x4_layout == fmat_4x4_layout_RowMajor
        return v2[i][j];
    #elif fmat_4x4_layout == fmat_4x4_layout_ColumnMajor
        return v2[j][i];
    #else
        #error(fmat_4x4_layout must be either fmat_4x4_layout_RowMajor or fmat_4x4_layout_ColumnMajor)
    #endif
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
    const ScalarType& at(const size_t i, const size_t j) const {
    #ifdef _DEBUG
        EGOBOO_ASSERT(i < _Rows);
        EGOBOO_ASSERT(j < _Columns);
    #endif
    #if fmat_4x4_layout == fmat_4x4_layout_RowMajor
        return v2[i][j];
    #elif fmat_4x4_layout == fmat_4x4_layout_ColumnMajor
        return v2[j][i];
    #else
        #error(fmat_4x4_layout must be either fmat_4x4_layout_RowMajor or fmat_4x4_layout_ColumnMajor)
    #endif
    }

    /**
     * @brief
     *  Get if this matrix is equal to another matrix.
     * @param other
     *  the other matrix
     * @return
     *  @a true if this matrix is equal to the other matrix
     */
    bool equalTo(const MyType& other) const {
        for (size_t i = 0; i < _Rows * _Columns; ++i) {
            if (at(i) != other.at(i)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief
     *  Get if this matrix is not equal to another matrix.
     * @param other
     *  the other matrix
     * @return
     *  @a true if this matrix is not equal to the other matrix
     */
    bool notEqualTo(const MyType& other) const {
        for (size_t i = 0; i < _Rows * _Columns; ++i) {
            if (at(i) != other.at(i)) {
                return true;
            }
        }
        return false;
    }

};

} // namespace Math
} // namespace Ego

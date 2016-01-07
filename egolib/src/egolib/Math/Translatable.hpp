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

/// @file   egolib/Math/Translatable.h
/// @brief  Miscellaneous utilities for template metaprogramming
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Vector.hpp"

namespace Ego {
namespace Math {

/** 
 * @brief
 *  The interface of an entity that can be translated.
 * @tparam _VectorSpaceType
 *  the vector space type
 */
template <typename _VectorSpaceType>
struct Translatable {
    /**
     * @brief
     *  The vector space type.
     */
    typedef _VectorSpaceType VectorSpaceType;
    /** 
     * @brief
     *  The vector type.
     */
    typedef Vector<typename VectorSpaceType::ScalarFieldType, VectorSpaceType::dimensionality()> VectorType;
    /** 
     * @brief
     *  Translate this entity.
     * @param t
     *  the translation vector
     */
    virtual void translate(const VectorType& t) = 0;
};

} // namespace Math
} // namespace Ego

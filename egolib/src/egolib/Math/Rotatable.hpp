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

/// @file   egolib/Math/Rotatable.h
/// @brief  Poor-man substitute for concepts.
///         Concepts are not yet available in current C++ revisions.
/// @author Michael Heilmann


#pragma once

#include "egolib/Math/Vector.hpp"

namespace Ego {
namespace Math {

/** 
 * @brief
 *  The interface of an entity that can be rotated.
 */
template <typename _VectorSpaceType>
struct Rotatable {
    /**
     * @brief
     *  The vector space type.
     */
    typedef _VectorSpaceType VectorSpaceType;
    /**
     * @brief
     *  The scalar field type.
     */
    typedef typename VectorSpaceType::ScalarFieldType ScalarFieldType;
    /** 
     * @brief
     *  The vector type.
     */
    typedef typename VectorSpaceType::VectorType VectorType;
    /**
     * @brief
     *  Rotate this entity.
     * @param axis
     *  the rotation axis
     * @param angle
     *  the rotation angle
     * @throw std::invalid_argument
     *  if the rotation axis is the zero vector
     */
    virtual void rotate(const VectorType& axis, float angle) = 0;
};

} // namespace Math
} // namespace Ego

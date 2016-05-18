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

/// @file  egolib/Math/Rect2.hpp
/// @brief 2D rectangle.

#include "egolib/Math/EuclideanSpace.hpp"

namespace Ego {
namespace Math {

template <typename _EuclideanSpaceType>
class Rect2 : public Translatable<typename _EuclideanSpaceType::VectorSpaceType> {
public:
    Ego_Math_EuclideanSpace_CommonDefinitions(Rect2);
};

} // namespace Math
} // namespace Ego

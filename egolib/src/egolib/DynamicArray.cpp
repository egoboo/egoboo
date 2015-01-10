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

/// @file  egolib/DynamicAray.cpp
/// @brief generic dynamic array structure

#include "egolib/DynamicArray.hpp"

// Implementations of dynamic arrays of value types.
IMPLEMENT_DYNAMIC_ARY(char_ary, char);
IMPLEMENT_DYNAMIC_ARY(short_ary, short);
IMPLEMENT_DYNAMIC_ARY(int_ary, int);
IMPLEMENT_DYNAMIC_ARY(float_ary, float);
IMPLEMENT_DYNAMIC_ARY(double_ary, double);

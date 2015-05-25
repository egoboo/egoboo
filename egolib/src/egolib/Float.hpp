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
#pragma once

#include "egolib/typedef.h"
#include "egolib/Debug.hpp"
#include "egolib/log.h"

#define IEEE32_FRACTION 0x007FFFFFL
#define IEEE32_EXPONENT 0x7F800000L
#define IEEE32_SIGN     0x80000000L

/**
 * @brief
 *	Get the raw bits of a @a float value.
 * @param x
 *	the @a float value
 * @return
 *	the @a Uint32 value
 */
Uint32 float_toBits(float x);

float float_fromBits(Uint32 x);
bool float_infinite(float x);
bool float_nan(float x);
bool float_bad(float x);

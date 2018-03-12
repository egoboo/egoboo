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

/// @file   egolib/Script/Interpreter/SafeCast.hpp
/// @brief  An safe and extendable cast function.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego::Script::Interpreter {

// Forward declaration.
struct TaggedValue;

/**
 * @brief This function casts a value of a source @a S into a value of target type @a T.
 * @throw Id::Exception this or a derived exception is raised if the value @a value of type @a S
 * can not be cast into the target type @a T.
 * @param v the value of the source type @a S
 * @return the value of the target type @a T
 */
template <typename T, typename S>
T safeCast(const S& v);

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Identity casts of fundamental types

// Helper macro to define identity conversions.
#define define(type) template<> type safeCast<type,type>(const type& v);

define(bool)

define(char) // signed char, unsigned char and char are three distinct types (http://en.cppreference.com/w/cpp/language/types)
define(signed char)
define(unsigned char)


define(short)
define(int)
define(long)
define(long long)

define(unsigned short)
define(unsigned int)
define(unsigned long)
define(unsigned long long)

// Usually a typedef for some fundamental type.
/*define(size_t)*/

// Usually a typedef for some fundamental type.
/*define(int8_t)*/
/*define(int16_t)*/
/*define(int32_t)*/
/*define(int64_t)*/

// Usually a typedef for some fundamental type.
/*define(uint8_t)*/
/*define(uint16_t)*/
/*define(uint32_t)*/
/*define(uint64_t)*/

define(float)
define(double)

#undef define

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Casts from tagged value to fundamental types

// Helper macro to define TaggedValue conversions.
#define define(targetType) template<> targetType safeCast<targetType,TaggedValue>(const TaggedValue& v);

define(bool)

// TODO: TaggedValue -> (signed) (short | int | long |long long)
// TODO: TaggedValue -> signed? char

define(unsigned char)
define(unsigned short)
define(unsigned int)
define(unsigned long)
define(unsigned long long)

define(float)
define(double)

#undef define

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Narrowing casts between unsigned integers

#define define(targetType, sourceType) \
    template<> targetType safeCast<targetType, sourceType>(const sourceType& v);

define(unsigned char, unsigned short)
define(unsigned char, unsigned int)
define(unsigned char, unsigned long)
define(unsigned char, unsigned long long)


define(unsigned short, unsigned int)
define(unsigned short, unsigned long)
define(unsigned short, unsigned long long)


define(unsigned int, unsigned long)
define(unsigned int, unsigned long long)

define(unsigned long, unsigned long long)

#undef define

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Narrowing casts between signed integers

#define define(targetType, sourceType) \
    template<> targetType safeCast<targetType, sourceType>(const sourceType& v);

define(signed char, signed short)
define(signed char, signed int)
define(signed char, signed long)
define(signed char, signed long long)

define(signed short, signed int)
define(signed short, signed long)
define(signed short, signed long long)

define(signed int, signed long)
define(signed int, signed long long)

define(signed long, signed long long)

#undef define

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Widening casts between unsigned integers

#define define(targetType, sourceType) \
    template<> targetType safeCast<targetType, sourceType>(const sourceType& v);

define(unsigned short, unsigned char)

define(unsigned int, unsigned short)
define(unsigned int, unsigned char)

define(unsigned long, unsigned int)
define(unsigned long, unsigned short)
define(unsigned long, unsigned char)

define(unsigned long long, unsigned long)
define(unsigned long long, unsigned int)
define(unsigned long long, unsigned short)
define(unsigned long long, unsigned char)

#undef define

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Widening casts between floating points

#define define(targetType, sourceType) \
    template<> targetType safeCast<targetType, sourceType>(const sourceType& v);

define(double, float)

#undef define

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Casts from signed integers to floating points

#define define(targetType, sourceType) \
    template<> targetType safeCast<targetType, sourceType>(const sourceType& v);

define(float, signed char)
define(float, signed short)
define(float, signed int)
define(float, signed long)
define(float, signed long long)

#undef define

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Casts from signed integers to unsigned integers

#define define(targetType, sourceType) \
    template<> targetType safeCast<targetType, sourceType>(const sourceType& v);

    define(unsigned char, signed char)
    define(unsigned short, signed short)
    define(unsigned int, signed int)
    define(unsigned long, signed long)
    define(unsigned long long, signed long long)

#undef define


template <>
inline unsigned char safeCast<unsigned char, int>(const int& v) {
    return safeCast<unsigned char>(safeCast<unsigned int>(v));
}

template <>
inline unsigned short safeCast<unsigned short, int>(const int& v) {
    return safeCast<unsigned short>(safeCast<unsigned int>(v));
}

template <>
inline unsigned long safeCast<unsigned long, int>(const int& v) {
    return safeCast<unsigned long>(safeCast<unsigned int>(v));
}

template <>
inline unsigned long long safeCast<unsigned long long, int>(const int& v) {
    return safeCast<unsigned long long>(safeCast<unsigned int>(v));
}

} // namespace Ego::Script::Interpreter

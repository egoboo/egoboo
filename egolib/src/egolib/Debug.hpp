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

/// @file egolib/Debug.hpp
/// @brief Miscellaneous Debug Utilities

#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Debug {
#ifdef _DEBUG

/**
 * @remark
 *	Validation functionality via template specialization.
 *  See the specialization for Ego::Math::Vector for an example.
 * @brief
 *	Assert that an object is valid.
 * @param object
 *	the object
 * @post
 *	If the object is not valid, an error is logged.
 */
template <typename ... T>
struct Validate;

template <typename T>
using MakeValidate = Validate<typename std::remove_const<typename std::remove_reference<T>::type>::type>;

#endif
#ifdef _DEBUG
    #define EGO_DEBUG_VALIDATE(_object_) { \
        Ego::Debug::MakeValidate<decltype(_object_)> validate; \
        validate(__FILE__, __LINE__, _object_); \
    }
#else
    #define EGO_DEBUG_VALIDATE(_object_)
#endif
    }
}


/// @todo Remove this. Use ID_ASSERT.
#define EGOBOO_ASSERT(assertion) ID_ASSERT(assertion)

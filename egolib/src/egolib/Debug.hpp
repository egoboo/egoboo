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

#include "egolib/Core/AssertionFailedException.hpp"

namespace Ego
{
    namespace Debug
    {
#ifdef _DEBUG
        /**
            * @remark
            *	Validation functionality via template specialization.
            *   See Ego::Math::Vector3 for an example.
            * @brief
            *	Assert that an object is valid.
            * @param object
            *	the object
            * @post
            *	If the object is not valid, an error is logged.
            */
        template <typename Type>
        void validate(const char *file, int line, const Type& object);
#endif
#ifdef _DEBUG
    #define EGO_DEBUG_VALIDATE(_object_) Ego::Debug::validate(__FILE__,__LINE__,_object_)
#else
    #define EGO_DEBUG_VALIDATE(_object_)
#endif
    }
}


/**
 * @brief
 *  Raise an exception if an assertion fails.
 * @param assertion
 *	the assertion
 * @throw Ego::Core::AssertionFailed
 *  if the assertion fails
 * @remark
 *	This macro evaluates to the empty statement if #_DEBUG is not defined.
 */
#if defined(_DEBUG)
    #define EGOBOO_ASSERT(assertion, ...) \
	    if(!(assertion)) { \
		    throw Ego::Core::AssertionFailed(__FILE__, __LINE__, #assertion); \
        }
#else
    #define EGOBOO_ASSERT(assertion) /* Empty statement. */;
#endif
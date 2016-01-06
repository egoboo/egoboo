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

/// @file   IdLib/DebugAssert.hpp
/// @brief  Debug assertion functionality.
/// @author Michael Heilmann

#pragma once

#include "IdLib/AssertionFailedException.hpp"

/**
 * @brief
 *  Macro raising an exception if an assertion fails.
 * @param assertion
 *	the assertion
 * @throw Id::AssertionFailedException
 *  if the assertion fails
 * @remark
 *	This macro evaluates to the empty statement if #_DEBUG is not defined.
 * @todo
 *  Add Id::DebugAssertionFailedException and use here.
 */
#if defined(_DEBUG)
    #define ID_ASSERT(assertion, ...) \
	    if(!(assertion)) { \
		    throw Id::AssertionFailedException(__FILE__, __LINE__, #assertion); \
        }
#else
    #define ID_ASSERT(assertion) /* Empty statement. */;
#endif
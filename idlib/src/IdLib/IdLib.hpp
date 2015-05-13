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

/**
 * @defgroup IdLib
 * @brief    Essential Utilities for Making Games
 * @details  IdLib contains essential utilities for making games.
 *           IdLib is used by Egolib and subsequently Cartman and Egoboo.
 * @author   Michael Heilmann
 */

/**
 * @brief
 *  printf format specifier for size_t.
 * @remark
 *  This is necessary because of Redmon Retards' (aka Microsoft) Visual C++.
 */
#if defined(_MSC_VER)
    #define PRIuZ "Iu"
#else
    #define PRIuZ "zu"
#endif

/**
 * @brief
 *  printf format specifier for ssize_t.
 * @remark
 *  This is necessary because of Redmon Retards' (aka Microsoft) Visual C++.
 */
#if defined(_MSC_VER)
    #define PRIdZ "Id"
#else
    #define PRIdZ "zd"
#endif

/**
 * @brief
 *  A macro alias for <tt>constexpr</tt>.
 * @remark
 *  This is necessary because of Redmon Retards' (aka Microsoft) Visual C++.
 */
#if defined(_MSC_VER)
    #define CONSTEXPR const
#else
    #define CONSTEXPR constexpr
#endif

/**
 * @brief
 *  A macro alias for Linux-flavored functions for MSVC.
 * @remark
 *  This is necessary because of Redmon Retards' (aka Microsoft) Visual C++ / Windows.
 */
#if defined(_MSC_VER)
    #define snprintf _snprintf
    #define stricmp _stricmp
    #define strlwr _strlwr
    #define strupr _strupr

    // This isn't needed anymore since MSCV 2013 and causes errors.
    #if !(_MSC_VER >= 1800)
        #define isnan _isnan
    #endif

    // This isn't needed anymore since MSVC 2008 and causes errors.
    #if (_MSC_VER < 1500)
        #define vsnprintf _vsnprintf
    #endif
#endif

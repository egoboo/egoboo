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


/**
 * @ingroup  IdLib
 * @file     IdLib/Platform.hpp
 * @brief    Functionality protecting the programmer from platform and compiler idiosyncrasies.
 * @author   Michael Heilmann
 */

#pragma once

#include "IdLib/Target.hpp"

/**
* @ingroup IdLib
* @brief
*  Format attributes for printf-style functions for GCC/Clang.
* @see
*  https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes
*/
#if defined(__GNUC__)
    #define GCC_PRINTF_FUNC(fmtargnum) __attribute__ (( format( __printf__, fmtargnum, fmtargnum+1 ) ))
#else
    #define GCC_PRINTF_FUNC(fmtargnum)
#endif

/**
* @ingroup IdLib
* @brief
*  Format attributes for scanf-style functions for GCC/Clang.
* @see
*  https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes
*/
#if defined(__GNUC__)
    #define GCC_SCANF_FUNC(fmtargnum) __attribute__ (( format( __scanf__, fmtargnum, fmtargnum+1 ) ))
#else
    #define GCC_SCANF_FUNC(fmtargnum)
#endif


// Common C and C++ headers.
#if defined(__cplusplus)
    #include <cstdlib>
#else
    #include <stdlib.h>
#endif

#if defined(__cplusplus)
    #include <cstddef>
#else
    #include <stddef.h>
#endif

#if defined(__cplusplus)
    #include <cctype>
#else
    #include <ctype.h>
#endif

#if defined(__cplusplus)
    #include <cstdarg>
#else
    #include <stdarg.h>
#endif

#if defined(__cplusplus)
    #include <cstdio>
#else
    #include <stdio.h>
#endif

#if defined(__cplusplus)
    #include <cassert>
#else
    #include <assert.h>
#endif

#if defined(__cplusplus)
    #include <cmath>
#else
    #include <math.h>
#endif

#if defined(__cplusplus)
    #include <cfloat>
#else
    #include <float.h>
#endif

#if defined(__cplusplus)
    #include <ctime>
#else
    #include <time.h>
#endif

#if defined(__cplusplus)
    #include <memory>
#else
    #include <memory.h>
#endif

#if defined(__cplusplus)
    #include <cstring>
#else
    #include <string.h>
#endif

#if defined(__cplusplus)
    #include <cstdbool>
#else
    #include <stdbool.h>
#endif

#if defined(__cplusplus)
    #include <cerrno>
#else
    #include <errno.h>
#endif

#if defined(__cplusplus)
    #include <cstdint>
#else
    #include <stdint.h>
#endif

// C++ exxclusive headers from here on (in alphabetic order).
#include <array>
#include <algorithm>
#include <atomic>
#include <bitset>
#include <exception>
#include <forward_list>
#include <functional>
#include <iomanip>
#include <iostream>
#include <locale>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <random>
#include <stdexcept>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
* @ingroup IdLib
* @brief
*  printf/scanf-style format specifier for size_t.
* @remark
*  This is necessary because of Redmon Retards' (aka Microsoft's) Visual C++.
*/
#if defined(_MSC_VER) || defined(__MINGW) || defined(__MINGW32__)
    #define PRIuZ "Iu"
#else
    #define PRIuZ "zx"
#endif

/**
 * @ingroup IdLib
 * @brief
 *  printf/scanf-style format specifier for ssize_t.
 * @remark
 *  This is necessary because of Redmon Retards' (aka Microsoft's) Visual C++.
 */
#if defined(_MSC_VER) || defined(__MINGW) || defined(__MINGW32__)
    #define PRIdZ "Id"
#else
    #define PRIdZ "zd"
#endif

/**
 * @ingroup IdLib
 * @brief
 *  A macro alias for <tt>constexpr</tt>.
 * @remark
 *  This is necessary because of Redmon Retards' (aka Microsoft) Visual C++.
 * @warning
 *  Obviously this macro can not add the semantics of the constexpr keyword
 *  to a compiler with pseudo-C++ 11 support, however, in some cases it can
 *  make up for it and it can be used to mark expressions to be rewritten
 *  as soon as the RR are able to "go native" and provide some "conformance"
 *  in their "industrial strength" "compiler".
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

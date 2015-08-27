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

#if defined(__cplusplus)
    #include <cinttypes>
#else
    #include <inttypes.h>
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
#include <condition_variable>
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
#include <chrono>
#include <set>

/**
* @ingroup IdLib
* @brief
*  printf/scanf-style format specifier for size_t.
* @remark
*  This is necessary because of Redmon Retards' (aka Microsoft's) Visual C++.
*/
#if defined(_MSC_VER) || defined(ID_MINGW)
    #define PRIuZ "Iu"
#else
    #define PRIuZ "zu"
#endif

/**
 * @ingroup IdLib
 * @brief
 *  printf/scanf-style format specifier for ssize_t.
 * @remark
 *  This is necessary because of Redmon Retards' (aka Microsoft's) Visual C++.
 */
#if defined(_MSC_VER) || defined(ID_MINGW)
    #define PRIdZ "Id"
#else
    #define PRIdZ "zd"
#endif

/**
 * @brief
 *  A macro alias for Linux-flavored functions for MSVC.
 * @remark
 *  This is necessary because of Redmon Retards' (aka Microsoft) Visual C++ / Windows.
 */
#if defined(_MSC_VER)
#define stricmp _stricmp
#define strlwr _strlwr
#define strupr _strupr

// This isn't needed anymore since MSVC 2015 and causes errors.
#if (_MSC_VER < 1900)
#define snprintf _snprintf
#endif

// This isn't needed anymore since MSVC 2013 and causes errors.
#if !(_MSC_VER >= 1800)
#define isnan _isnan
#endif

// This isn't needed anymore since MSVC 2008 and causes errors.
#if (_MSC_VER < 1500)
#define vsnprintf _vsnprintf
#endif
#endif

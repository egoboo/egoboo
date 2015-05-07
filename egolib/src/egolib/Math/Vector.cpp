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

/// @file   egolib/Math/Vector.cpp
/// @brief  2-,3- and 4-dimensional vectors.
/// @author Michael Heilmann et al.

#include "egolib/Math/Vector.hpp"
#include "egolib/log.h"
#include "egolib/_math.h"

const fvec4_t fvec4_t::zero(0.0f, 0.0f, 0.0f, 0.0f);

fvec3_t operator-(const fvec3_t& v)
{
    return fvec3_t(-v.x, -v.y, -v.z);
}

fvec4_t operator-(const fvec4_t& v)
{
    return fvec4_t(-v.x, -v.y, -v.z, -v.w);
}

#ifdef _DEBUG
namespace Ego
{
    namespace Debug
    {
        template <>
        void validate<::fvec2_t>(const char *file, int line, const ::fvec2_t& object)
        {
            for (size_t i = 0; i < 2; ++i)
            {
                if (float_bad(object[i]))
                {
                    log_error("%s:%d: invalid vector component of 2D vector\n", file, line);
                }
            }
        }

    }
}
#endif

#ifdef _DEBUG
namespace Ego
{
    namespace Debug
    {
        template <>
        void validate<::fvec3_t>(const char *file, int line, const ::fvec3_t& object)
        {
            for (size_t i = 0; i < 3; ++i)
            {
                if (float_bad(object[i]))
                {
                    log_error("%s:%d: invalid vector component of 3D vector\n", file, line);
                }
            }
        }
    }
}
#endif

#ifdef _DEBUG
namespace Ego
{
    namespace Debug
    {
        template <>
        void validate<::fvec4_t>(const char *file, int line, const ::fvec4_t& object)
        {
            for (size_t i = 0; i < 4; ++i)
            {
                if (float_bad(object[i]))
                {
                    log_error("%s:%d: invalid vector component of 4D vector\n", file, line);
                }
            }
        }
    }
}
#endif

float fvec3_decompose(const fvec3_t& A, const fvec3_t& vnrm, fvec3_t& vpara, fvec3_t& vperp)
{
    /// @author BB
    /// @details the normal (vnrm) is assumed to be normalized. Try to get this as optimized as possible.

    float dot;

    // if this is true, there is no reason to run this function
    dot = A.dot(vnrm);

    if (0.0f == dot)
    {
        {
            vpara[kX] = 0.0f;
            vpara[kY] = 0.0f;
            vpara[kZ] = 0.0f;

            vperp[kX] = A[kX];
            vperp[kY] = A[kY];
            vperp[kZ] = A[kZ];
        }
    }
    else
    {
        {
            vpara[kX] = dot * vnrm[kX];
            vpara[kY] = dot * vnrm[kY];
            vpara[kZ] = dot * vnrm[kZ];

            vperp[kX] = A[kX] - vpara[kX];
            vperp[kY] = A[kY] - vpara[kY];
            vperp[kZ] = A[kZ] - vpara[kZ];

        }
    }

    return dot;
}

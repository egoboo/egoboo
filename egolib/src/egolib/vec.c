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

/// @file  egolib/vec.c
/// @brief 2-,3- and 4-dimensional vectors.
#include "egolib/vec.h"
#include "egolib/log.h"
#include "egolib/_math.h"

const fvec2_t fvec2_t::zero(0.0f, 0.0f);

const fvec3_t fvec3_t::zero(0.0f, 0.0f, 0.0f);

const fvec4_t fvec4_t::zero(0.0f, 0.0f, 0.0f, 0.0f);

fvec2_t operator-(const fvec2_t& v)
{
	return fvec2_t(-v.x, -v.y);
}

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

float fvec2_dot_product(const fvec2_base_t A, const fvec2_base_t B)
{
	return A[kX] * B[kX] + A[kY] * B[kY];
}
//--------------------------------------------------------------------------------------------
void fvec3_ctor(fvec3_t& v)
{
	v[kX] = v[kY] = v[kZ] = 0.0f;
}

//--------------------------------------------------------------------------------------------
void fvec3_dtor(fvec3_t& v)
{
	v[kX] = v[kY] = v[kZ] = 0.0f;
}
//--------------------------------------------------------------------------------------------
float fvec3_decompose(const fvec3_t& A, const fvec3_t& vnrm, fvec3_t& vpara, fvec3_t& vperp)
{
	/// @author BB
	/// @details the normal (vnrm) is assumed to be normalized. Try to get this as optimized as possible.

	float dot;
#if 0
	// error trapping
	if (NULL == A || NULL == vnrm) return 0.0f;
#endif
	// if this is true, there is no reason to run this function
	dot = A.dot(vnrm);

	if (0.0f == dot)
	{
#if 0
		// handle optional parameters
		if (NULL == vpara && NULL == vperp)
		{
			// no point in doing anything
			return 0.0f;
		}
		else if (NULL == vpara)
		{
			vperp[kX] = A[kX];
			vperp[kY] = A[kY];
			vperp[kZ] = A[kZ];

			LOG_NAN_FVEC3(vperp);
		}
		else if (NULL == vperp)
		{
			vpara[kX] = 0.0f;
			vpara[kY] = 0.0f;
			vpara[kZ] = 0.0f;

			LOG_NAN_FVEC3(vpara);
		}
		else
#endif
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
#if 0
		// handle optional parameters
		if (NULL == vpara && NULL == vperp)
		{
			// no point in doing anything
			return 0.0f;
		}
		else if (NULL == vpara)
		{
			vperp[kX] = A[kX] - dot * vnrm[kX];
			vperp[kY] = A[kY] - dot * vnrm[kY];
			vperp[kZ] = A[kZ] - dot * vnrm[kZ];

			LOG_NAN_FVEC3(vperp);
		}
		else if (NULL == vperp)
		{
			vpara[kX] = dot * vnrm[kX];
			vpara[kY] = dot * vnrm[kY];
			vpara[kZ] = dot * vnrm[kZ];

			LOG_NAN_FVEC3(vpara);
		}
		else
#endif
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

//--------------------------------------------------------------------------------------------
float fvec3_dist_abs(const fvec3_t& u, const fvec3_t& v)
{
	float retval = ABS(u[kX] - v[kX]) + ABS(u[kY] - v[kY]) + ABS(u[kZ] - v[kZ]);
	LOG_NAN(retval);
	return retval;
}

//--------------------------------------------------------------------------------------------
float fvec3_dist_2(const fvec3_t& u, const fvec3_t& v)
{
	float retval = 0.0f, ftmp;

	ftmp = u[kX] - v[kX];
	retval += ftmp * ftmp;

	ftmp = u[kY] - v[kY];
	retval += ftmp * ftmp;

	ftmp = u[kZ] - v[kZ];
	retval += ftmp * ftmp;

	LOG_NAN(retval);

	return retval;
}
//--------------------------------------------------------------------------------------------
#if 0
bool fvec4_valid(const fvec4_base_t v)
{
	if (nullptr == v)
	{
		return false;
	}
	for (size_t cnt = 0; cnt < 4; cnt++)
	{
		if (float_bad(v[cnt]))
		{
			return false;
		}
	}
	return true;
}
#endif
//--------------------------------------------------------------------------------------------
bool fvec4_self_clear(fvec4_base_t v)
{
	if (nullptr == v)
	{
		return false;
	}
	v[kX] = v[kY] = v[kZ] = 0.0f;
	v[kW] = 1.0f;

	return true;
}

//--------------------------------------------------------------------------------------------
bool fvec4_self_scale(fvec4_base_t v, const float s)
{
	if (nullptr == v)
	{
		return false;
	}
	v[kX] *= s;
	v[kY] *= s;
	v[kZ] *= s;
	v[kW] *= s;
	return true;
}
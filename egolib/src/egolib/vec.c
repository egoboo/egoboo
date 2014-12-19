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

/// @file egolib/vec.c
/// @brief 2-,3- and 4-dimensional vectors
#include "egolib/vec.h"
#include "egolib/log.h"

bool fvec2_valid(const fvec2_base_t A)
{
	int cnt;

	if (NULL == A) return false;

	for (cnt = 0; cnt < 2; cnt++)
	{
		if (ieee32_bad(A[cnt])) return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool fvec2_self_clear(fvec2_base_t A)
{
	if (NULL == A) return false;

	A[kX] = A[kY] = 0.0f;

	return true;
}

//--------------------------------------------------------------------------------------------
bool fvec2_self_is_clear(const fvec2_base_t A)
{
	if (NULL == A) return true;
	return A[kX] == A[kY] == 0.0f;
}

//--------------------------------------------------------------------------------------------
bool fvec2_base_copy(fvec2_base_t A, const fvec2_base_t B)
{
	if (NULL == A) return false;

	if (NULL == B) return fvec2_self_clear(A);

	A[kX] = B[kX];
	A[kY] = B[kY];

	return true;
}

//--------------------------------------------------------------------------------------------
bool fvec2_self_scale(fvec2_base_t A, const float B)
{
	if (NULL == A) return false;

	A[kX] *= B;
	A[kY] *= B;

	return true;
}

//--------------------------------------------------------------------------------------------
bool fvec2_self_sum(fvec2_base_t A, const fvec2_base_t B)
{
	if (NULL == A || NULL == B) return false;

	A[kX] += B[kX];
	A[kY] += B[kY];

	LOG_NAN_FVEC2(A);

	return true;
}

//--------------------------------------------------------------------------------------------
float fvec2_length_abs(const fvec2_base_t A)
{
	if (NULL == A) return 0.0f;

	return ABS(A[kX]) + ABS(A[kY]);
}

//--------------------------------------------------------------------------------------------
float fvec2_length_2(const fvec2_base_t A)
{
	float A2;

	if (NULL == A) return 0.0f;

	A2 = A[kX] * A[kX] + A[kY] * A[kY];

	return A2;
}

//--------------------------------------------------------------------------------------------
float fvec2_length(const fvec2_base_t A)
{
	float A2;

	if (NULL == A) return 0.0f;

	A2 = A[kX] * A[kX] + A[kY] * A[kY];

	return std::sqrt(A2);
}

//--------------------------------------------------------------------------------------------
float *fvec2_sub(fvec2_base_t DST, const fvec2_base_t LHS, const fvec2_base_t RHS)
{
	if (NULL == DST)
	{
		return NULL;
	}
	else if (NULL == LHS && NULL == RHS)
	{
		fvec2_self_clear(DST);
	}
	else if (NULL == LHS)
	{
		DST[kX] = -RHS[kX];
		DST[kY] = -RHS[kY];
	}
	else if (NULL == RHS)
	{
		DST[kX] = LHS[kX];
		DST[kY] = LHS[kY];
	}
	else
	{
		DST[kX] = LHS[kX] - RHS[kX];
		DST[kY] = LHS[kY] - RHS[kY];
	}

	return DST;
}

//--------------------------------------------------------------------------------------------
float *fvec2_add(fvec2_base_t DST, const fvec2_base_t LHS, const fvec2_base_t RHS)
{
	if (NULL == DST)
	{
		return NULL;
	}
	else if (NULL == LHS && NULL == RHS)
	{
		fvec2_self_clear(DST);
	}
	else if (NULL == LHS)
	{
		fvec2_base_copy(DST, RHS);
	}
	else if (NULL == RHS)
	{
		fvec2_base_copy(DST, LHS);
	}
	else
	{
		DST[kX] = LHS[kX] - RHS[kX];
		DST[kY] = LHS[kY] - RHS[kY];
	}

	return DST;
}

//--------------------------------------------------------------------------------------------
float fvec2_dist_abs(const fvec2_base_t A, const fvec2_base_t B)
{
	return ABS(A[kX] - B[kX]) + ABS(A[kY] - B[kY]);
}

//--------------------------------------------------------------------------------------------
float * fvec2_scale(fvec2_base_t DST, const fvec2_base_t SRC, const float B)
{
	if (NULL == DST) return NULL;

	if (NULL == SRC || 0.0f == B)
	{
		fvec2_self_clear(DST);
	}
	else
	{
		DST[kX] = SRC[kX] * B;
		DST[kY] = SRC[kY] * B;
	}

	return DST;
}

//--------------------------------------------------------------------------------------------
float * fvec2_normalize(fvec2_base_t DST, const fvec2_base_t SRC)
{
	if (NULL == DST)
	{
		return NULL;
	}

	if (NULL == SRC)
	{
		fvec2_self_clear(DST);
	}
	else if (0.0f == ABS(SRC[kX]) + ABS(SRC[kY]))
	{
		fvec2_self_clear(DST);
	}
	else
	{
		float len2 = SRC[kX] * SRC[kX] + SRC[kY] * SRC[kY];

		if (0.0f != len2)
		{
			float inv_len = 1.0f / std::sqrt(len2);
			LOG_NAN(inv_len);

			DST[kX] = SRC[kX] * inv_len;
			LOG_NAN(DST[kX]);

			DST[kY] = SRC[kY] * inv_len;
			LOG_NAN(DST[kY]);
		}
	}

	return DST;
}

//--------------------------------------------------------------------------------------------
bool  fvec2_self_normalize(fvec2_base_t A)
{
#if 0
	float len2;
	float inv_len;
#endif
	if (NULL == A) return false;

	if (0.0f == fvec2_length_abs(A)) return false;

	float len2 = A[kX] * A[kX] + A[kY] * A[kY];
	float inv_len = 1.0f / std::sqrt(len2);

	A[kX] *= inv_len;
	A[kY] *= inv_len;

	return true;
}

//--------------------------------------------------------------------------------------------
float fvec2_cross_product(const fvec2_base_t A, const fvec2_base_t B)
{
	return A[kX] * B[kY] - A[kY] * B[kX];
}

//--------------------------------------------------------------------------------------------
float   fvec2_dot_product(const fvec2_base_t A, const fvec2_base_t B)
{
	return A[kX] * B[kX] + A[kY] * B[kY];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
void fvec3_ctor(fvec3_base_t A)
{
	A[kX] = A[kY] = A[kZ] = 0.0f;
}

//--------------------------------------------------------------------------------------------
void fvec3_dtor(fvec3_base_t A)
{
	A[kX] = A[kY] = A[kZ] = 0.0f;
}
//--------------------------------------------------------------------------------------------
bool fvec3_valid(const fvec3_base_t A)
{
	int cnt;

	if (NULL == A) return false;

	for (cnt = 0; cnt < 3; cnt++)
	{
		if (ieee32_bad(A[cnt])) return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool fvec3_self_clear(fvec3_base_t A)
{
	if (NULL == A) return false;

	A[kX] = A[kY] = A[kZ] = 0.0f;

	return true;
}

//--------------------------------------------------------------------------------------------
bool fvec3_self_is_clear(const fvec2_base_t A)
{
	if (NULL == A) return true;
	return A[kX] == A[kY] == A[kZ] == 0.0f;
}


//--------------------------------------------------------------------------------------------
float * fvec3_base_copy(fvec3_base_t DST, const fvec3_base_t SRC)
{
	if (NULL == DST) return NULL;

	if (NULL == SRC)
	{
		fvec3_self_clear(DST);
	}
	else
	{
		DST[kX] = SRC[kX];
		DST[kY] = SRC[kY];
		DST[kZ] = SRC[kZ];

		LOG_NAN_FVEC3(DST);
	}

	return DST;
}

//--------------------------------------------------------------------------------------------
bool fvec3_self_scale(fvec3_base_t A, const float B)
{
	if (NULL == A) return false;

	A[kX] *= B;
	A[kY] *= B;
	A[kZ] *= B;

	LOG_NAN_FVEC3(A);

	return true;
}

//--------------------------------------------------------------------------------------------
bool fvec3_self_sum(fvec3_base_t A, const fvec3_base_t B)
{
	if (NULL == A || NULL == B) return false;

	A[kX] += B[kX];
	A[kY] += B[kY];
	A[kZ] += B[kZ];

	LOG_NAN_FVEC3(A);

	return true;
}

//--------------------------------------------------------------------------------------------
float fvec3_length_abs(const fvec3_base_t A)
{
	float retval;

	if (NULL == A) return 0.0f;

	retval = ABS(A[kX]) + ABS(A[kY]) + ABS(A[kZ]);

#if 0
    // Not sure why this one's different
	//DEBUG
	if (ieee32_bad(retval))
	{
		log_debug("Game decided to crash, but I refuse! (%s - %d)\n", __FILE__, __LINE__);
		retval = 0.00f;
	}
	//DEBUG END
#endif
    
    LOG_NAN( retval );

	return retval;
}

//--------------------------------------------------------------------------------------------
float fvec3_length_2(const fvec3_base_t A)
{
	float A2;

	if (NULL == A) return 0.0f;

	A2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];

	LOG_NAN(A2);

	return A2;
}

//--------------------------------------------------------------------------------------------
float fvec3_length(const fvec3_base_t A)
{
	if (NULL == A) return 0.0f;

	float A2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];

	LOG_NAN(A2);

	return std::sqrt(A2);
}

//--------------------------------------------------------------------------------------------
float * fvec3_add(fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS)
{
	if (NULL == DST)
	{
		return NULL;
	}

	if (NULL == LHS && NULL == RHS)
	{
		fvec2_self_clear(DST);
	}
	else if (NULL == LHS)
	{
		fvec3_base_copy(DST, RHS);

		LOG_NAN_FVEC3(DST);
	}
	else if (NULL == RHS)
	{
		fvec3_base_copy(DST, LHS);

		LOG_NAN_FVEC3(DST);
	}
	else
	{
		DST[kX] = LHS[kX] + RHS[kX];
		DST[kY] = LHS[kY] + RHS[kY];
		DST[kZ] = LHS[kZ] + RHS[kZ];

		LOG_NAN_FVEC3(DST);
	}

	return DST;
}

//--------------------------------------------------------------------------------------------
float * fvec3_sub(fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS)
{
	if (NULL == DST)
	{
		return NULL;
	}

	if (NULL == LHS && NULL == RHS)
	{
		fvec2_self_clear(DST);
	}
	else if (NULL == LHS)
	{
		DST[kX] = -RHS[kX];
		DST[kY] = -RHS[kY];
		DST[kZ] = -RHS[kZ];

		LOG_NAN_FVEC3(DST);
	}
	else if (NULL == RHS)
	{
		DST[kX] = LHS[kX];
		DST[kY] = LHS[kY];
		DST[kZ] = LHS[kZ];

		LOG_NAN_FVEC3(DST);
	}
	else
	{
		DST[kX] = LHS[kX] - RHS[kX];
		DST[kY] = LHS[kY] - RHS[kY];
		DST[kZ] = LHS[kZ] - RHS[kZ];

		LOG_NAN_FVEC3(DST);
	}

	return DST;
}

//--------------------------------------------------------------------------------------------
float * fvec3_scale(fvec3_base_t DST, const fvec3_base_t SRC, const float B)
{

	if (NULL == DST)
	{
		return NULL;
	}

	if (NULL == SRC)
	{
		fvec3_self_clear(DST);
	}
	else if (0.0f == B)
	{
		fvec3_self_clear(DST);
	}
	else
	{
		DST[kX] = SRC[kX] * B;
		DST[kY] = SRC[kY] * B;
		DST[kZ] = SRC[kZ] * B;

		LOG_NAN_FVEC3(DST);
	}

	return DST;
}

//--------------------------------------------------------------------------------------------
float * fvec3_normalize(fvec3_base_t DST, const fvec3_base_t SRC)
{
	if (NULL == DST)
	{
		return NULL;
	}

	if (NULL == SRC)
	{
		fvec3_self_clear(DST);
	}
	else
	{
		float len2 = SRC[kX] * SRC[kX] + SRC[kY] * SRC[kY] + SRC[kZ] * SRC[kZ];

		if (0.0f == len2)
		{
			fvec3_self_clear(DST);
		}
		else
		{
			float inv_len = 1.0f / std::sqrt(len2);
			LOG_NAN(inv_len);

			DST[kX] = SRC[kX] * inv_len;
			DST[kY] = SRC[kY] * inv_len;
			DST[kZ] = SRC[kZ] * inv_len;

			LOG_NAN_FVEC3(DST);
		}
	}

	return DST;
}

//--------------------------------------------------------------------------------------------
float fvec3_self_normalize(fvec3_base_t A)
{
	float len = -1.0f;

	if (NULL == A) return len;

	if (0.0f != fvec3_length_abs(A))
	{
		float len2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];
		len = std::sqrt(len2);
		float inv_len = 1.0f / len;

		LOG_NAN(inv_len);

		A[kX] *= inv_len;
		A[kY] *= inv_len;
		A[kZ] *= inv_len;
	}

	LOG_NAN_FVEC3(A);

	return len;
}

//--------------------------------------------------------------------------------------------
float fvec3_self_normalize_to(fvec3_base_t vec, const float B)
{
	float len = -1.0f;

	if (NULL == vec) return len;

	if (0.0f == B)
	{
		fvec3_self_clear(vec);

		len = 0.0f;
	}
	else if (0.0f != fvec3_length_abs(vec))
	{
#if 0
		float len2, inv_len;
#endif
		float len2 = vec[kX] * vec[kX] + vec[kY] * vec[kY] + vec[kZ] * vec[kZ];
		len = std::abs(len2);
		float inv_len = B / len;

		LOG_NAN(inv_len);

		vec[kX] *= inv_len;
		vec[kY] *= inv_len;
		vec[kZ] *= inv_len;
	}

	LOG_NAN_FVEC3(vec);

	return len;
}

//--------------------------------------------------------------------------------------------
float * fvec3_cross_product(fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS)
{
	if (NULL == DST)
	{
		return NULL;
	}

	if (NULL == LHS || NULL == RHS)
	{
		fvec3_self_clear(DST);
	}
	else
	{
		DST[kX] = LHS[kY] * RHS[kZ] - LHS[kZ] * RHS[kY];
		DST[kY] = LHS[kZ] * RHS[kX] - LHS[kX] * RHS[kZ];
		DST[kZ] = LHS[kX] * RHS[kY] - LHS[kY] * RHS[kX];

		LOG_NAN_FVEC3(DST);
	}

	return DST;
}

//--------------------------------------------------------------------------------------------
float fvec3_decompose(const fvec3_base_t A, const fvec3_base_t vnrm, fvec3_base_t vpara, fvec3_base_t vperp)
{
	/// @author BB
	/// @details the normal (vnrm) is assumed to be normalized. Try to get this as optimized as possible.

	float dot;

	// error trapping
	if (NULL == A || NULL == vnrm) return 0.0f;

	// if this is true, there is no reason to run this function
	dot = fvec3_dot_product(A, vnrm);

	if (0.0f == dot)
	{
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
		{
			vpara[kX] = 0.0f;
			vpara[kY] = 0.0f;
			vpara[kZ] = 0.0f;

			vperp[kX] = A[kX];
			vperp[kY] = A[kY];
			vperp[kZ] = A[kZ];

			LOG_NAN_FVEC3(vperp);
		}
	}
	else
	{
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
		{
			vpara[kX] = dot * vnrm[kX];
			vpara[kY] = dot * vnrm[kY];
			vpara[kZ] = dot * vnrm[kZ];

			LOG_NAN_FVEC3(vpara);

			vperp[kX] = A[kX] - vpara[kX];
			vperp[kY] = A[kY] - vpara[kY];
			vperp[kZ] = A[kZ] - vpara[kZ];

			LOG_NAN_FVEC3(vperp);
		}
	}

	return dot;
}

//--------------------------------------------------------------------------------------------
float fvec3_dist_abs(const fvec3_base_t A, const fvec3_base_t B)
{
	float retval;

	if (NULL == A || NULL == B) return 0.0f;

	retval = ABS(A[kX] - B[kX]) + ABS(A[kY] - B[kY]) + ABS(A[kZ] - B[kZ]);

	LOG_NAN(retval);

	return retval;
}

//--------------------------------------------------------------------------------------------
float fvec3_dist_2(const fvec3_base_t LHS, const fvec3_base_t RHS)
{
	float retval = 0.0f, ftmp;

	if (NULL == LHS || NULL == LHS) return 0.0f;

	ftmp = LHS[kX] - RHS[kX];
	retval += ftmp * ftmp;

	ftmp = LHS[kY] - RHS[kY];
	retval += ftmp * ftmp;

	ftmp = LHS[kZ] - RHS[kZ];
	retval += ftmp * ftmp;

	LOG_NAN(retval);

	return retval;
}

//--------------------------------------------------------------------------------------------
float   fvec3_dot_product(const fvec3_base_t A, const fvec3_base_t B)
{
	float retval;

	if (NULL == A || NULL == B) return 0.0f;

	retval = A[kX] * B[kX] + A[kY] * B[kY] + A[kZ] * B[kZ];

	LOG_NAN(retval);

	return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool fvec4_valid(const fvec4_base_t A)
{
	int cnt;

	if (NULL == A) return false;

	for (cnt = 0; cnt < 4; cnt++)
	{
		if (ieee32_bad(A[cnt])) return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool fvec4_self_clear(fvec4_base_t A)
{
	if (NULL == A) return false;

	A[kX] = A[kY] = A[kZ] = 0.0f;
	A[kW] = 1.0f;

	return true;
}

//--------------------------------------------------------------------------------------------
bool fvec4_self_scale(fvec4_base_t A, const float B)
{
	if (NULL == A) return false;

	A[kX] *= B;
	A[kY] *= B;
	A[kZ] *= B;
	A[kW] *= B;

	LOG_NAN_FVEC4(A);

	return true;
}
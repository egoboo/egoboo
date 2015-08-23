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

/// @file game/lighting.c
/// @brief Code for controlling the character and mesh lighting
/// @details

#include "game/lighting.h"

#include "egolib/_math.h"
#include "egolib/Math/Vector.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float light_a = 0.0f,
      light_d = 0.0f;
Vector3f light_nrm = Vector3f::zero();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool lighting_sum_project( lighting_cache_t * dst, const lighting_cache_t * src, const Vector3f& vec, const int dir );

static float  lighting_evaluate_cache_base( const lighting_cache_base_t * lvec, const Vector3f& nrm, float * amb );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void lighting_vector_evaluate( const std::array<float, LIGHTING_VEC_SIZE> &lvec, const Vector3f& nrm, float * dir, float * amb )
{
    float loc_dir, loc_amb;

    if ( NULL == dir ) dir = &loc_dir;
    if ( NULL == amb ) amb = &loc_amb;

    *dir = 0.0f;
    *amb = 0.0f;

    if ( nrm[kX] > 0.0f )
    {
        *dir += nrm[kX] * lvec[LVEC_PX];
    }
    else if ( nrm[kX] < 0.0f )
    {
        *dir -= nrm[kX] * lvec[LVEC_MX];
    }

    if ( nrm[kY] > 0.0f )
    {
        *dir += nrm[kY] * lvec[LVEC_PY];
    }
    else if ( nrm[kY] < 0.0f )
    {
        *dir -= nrm[kY] * lvec[LVEC_MY];
    }

    if ( nrm[kZ] > 0.0f )
    {
        *dir += nrm[kZ] * lvec[LVEC_PZ];
    }
    else if ( nrm[kZ] < 0.0f )
    {
        *dir -= nrm[kZ] * lvec[LVEC_MZ];
    }

    // the ambient is not summed
    *amb += lvec[LVEC_AMB];
}

//--------------------------------------------------------------------------------------------
void lighting_vector_sum( std::array<float, LIGHTING_VEC_SIZE> &lvec, const Vector3f& nrm, const float direct, const float ambient )
{
    if ( nrm[kX] > 0.0f )
    {
        lvec[LVEC_PX] += nrm[kX] * direct;
    }
    else if ( nrm[kX] < 0.0f )
    {
        lvec[LVEC_MX] -= nrm[kX] * direct;
    }

    if ( nrm[kY] > 0.0f )
    {
        lvec[LVEC_PY] += nrm[kY] * direct;
    }
    else if ( nrm[kY] < 0.0f )
    {
        lvec[LVEC_MY] -= nrm[kY] * direct;
    }

    if ( nrm[kZ] > 0.0f )
    {
        lvec[LVEC_PZ] += nrm[kZ] * direct;
    }
    else if ( nrm[kZ] < 0.0f )
    {
        lvec[LVEC_MZ] -= nrm[kZ] * direct;
    }

    // the ambient is not summed
    lvec[LVEC_AMB] += ambient;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
lighting_cache_base_t *lighting_cache_base_t::init(lighting_cache_base_t *self)
{
	if (!self) {
		return nullptr;
	}
	BLANK_STRUCT_PTR(self);
	return self;
}

//--------------------------------------------------------------------------------------------
bool lighting_cache_base_t::max_light(lighting_cache_base_t *self)
{
	if (!self) {
		return false;
	}
    // determine the lighting extents
	float max_light = std::abs(self->_lighting[0]);
    for (size_t i = 1; i < (size_t)LIGHTING_VEC_SIZE - 1; ++i) {
		max_light = std::max(max_light, std::abs(self->_lighting[i]));
    }
	self->_max_light = max_light;
    return true;
}

//--------------------------------------------------------------------------------------------
bool lighting_cache_base_blend( lighting_cache_base_t * cold, const lighting_cache_base_t * cnew, float keep )
{
    int tnc;
    float max_delta;

    if ( NULL == cold || NULL == cnew ) return false;

    // blend this in with the existing lighting
    if ( 1.0f == keep )
    {
        // no change from last time
        max_delta = 0.0f;
    }
    else if ( 0.0f == keep )
    {
        max_delta = 0.0f;
        for ( tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++ )
        {
            float delta = std::abs( cnew->_lighting[tnc] - cold->_lighting[tnc] );

            cold->_lighting[tnc] = cnew->_lighting[tnc];

            max_delta = std::max( max_delta, delta );
        }
    }
    else
    {
        max_delta = 0.0f;
        for ( tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++ )
        {
            float ftmp;

            ftmp = cold->_lighting[tnc];
            cold->_lighting[tnc] = ftmp * keep + cnew->_lighting[tnc] * ( 1.0f - keep );
            max_delta = std::max( max_delta, std::abs( cold->_lighting[tnc] - ftmp ) );
        }
    }

    cold->_max_delta = max_delta;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
lighting_cache_t * lighting_cache_t::init(lighting_cache_t *self)
{
	if (!self) {
		return self;
	}
	lighting_cache_base_t::init(&(self->low));
	lighting_cache_base_t::init(&(self->hgh));

	self->_max_delta = 0.0f;
	self->_max_light = 0.0f;

	return self;
}

//--------------------------------------------------------------------------------------------
bool lighting_cache_t::max_light(lighting_cache_t * self)
{
	if (NULL == self) return false;

    // determine the lighting extents
	lighting_cache_base_t::max_light(&(self->low));
	lighting_cache_base_t::max_light(&(self->hgh));

    // set the maximum direct light
	self->_max_light = std::max(self->low._max_light, self->hgh._max_light);

    return true;
}

//--------------------------------------------------------------------------------------------
bool lighting_cache_t::blend( lighting_cache_t * cache, lighting_cache_t * cnew, float keep )
{
    if ( NULL == cache || NULL == cnew ) return false;

    // find deltas
    lighting_cache_base_blend( &( cache->low ), ( &cnew->low ), keep );
    lighting_cache_base_blend( &( cache->hgh ), ( &cnew->hgh ), keep );

    // find the absolute maximum delta
    cache->_max_delta = std::max( cache->low._max_delta, cache->hgh._max_delta );

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool lighting_project_cache( lighting_cache_t * dst, const lighting_cache_t * src, const fmat_4x4_t& mat )
{
	Vector3f fwd, right, up;

    if ( NULL == src ) return false;

    // blank the destination lighting
    if ( NULL == lighting_cache_t::init( dst ) ) return false;

    // do the ambient
    dst->low._lighting[LVEC_AMB] = src->low._lighting[LVEC_AMB];
    dst->hgh._lighting[LVEC_AMB] = src->hgh._lighting[LVEC_AMB];

    if ( src->_max_light == 0.0f ) return true;

    // grab the character directions
    fwd = mat_getChrForward(mat);
    right = mat_getChrRight(mat);
    up = mat_getChrUp(mat);

    fwd.normalize();
    right.normalize();
    up.normalize();

    // split the lighting cache up
    lighting_sum_project( dst, src, right, 0 );
    lighting_sum_project( dst, src, fwd,   2 );
    lighting_sum_project( dst, src, up,    4 );

    // determine the lighting extents
    lighting_cache_t::max_light( dst );

    return true;
}

//--------------------------------------------------------------------------------------------
bool lighting_cache_interpolate( lighting_cache_t * dst, const lighting_cache_t * src[], const float u, const float v )
{
    int   tnc;
    float wt_sum;
    float loc_u, loc_v;

    if ( NULL == src ) return false;

    if ( NULL == lighting_cache_t::init( dst ) ) return false;

    loc_u = CLIP( u, 0.0f, 1.0f );
    loc_v = CLIP( v, 0.0f, 1.0f );

    wt_sum = 0.0f;

    if ( NULL != src[0] )
    {
        float wt = ( 1.0f - loc_u ) * ( 1.0f - loc_v );
        for ( tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++ )
        {
            dst->low._lighting[tnc] += src[0]->low._lighting[tnc] * wt;
            dst->hgh._lighting[tnc] += src[0]->hgh._lighting[tnc] * wt;
        }
        wt_sum += wt;
    }

    if ( NULL != src[1] )
    {
        float wt = loc_u * ( 1.0f - loc_v );
        for ( tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++ )
        {
            dst->low._lighting[tnc] += src[1]->low._lighting[tnc] * wt;
            dst->hgh._lighting[tnc] += src[1]->hgh._lighting[tnc] * wt;
        }
        wt_sum += wt;
    }

    if ( NULL != src[2] )
    {
        float wt = ( 1.0f - loc_u ) * loc_v;
        for ( tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++ )
        {
            dst->low._lighting[tnc] += src[2]->low._lighting[tnc] * wt;
            dst->hgh._lighting[tnc] += src[2]->hgh._lighting[tnc] * wt;
        }
        wt_sum += wt;
    }

    if ( NULL != src[3] )
    {
        float wt = loc_u * loc_v;
        for ( tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++ )
        {
            dst->low._lighting[tnc] += src[3]->low._lighting[tnc] * wt;
            dst->hgh._lighting[tnc] += src[3]->hgh._lighting[tnc] * wt;
        }
        wt_sum += wt;
    }

    if ( wt_sum > 0.0f )
    {
        if ( wt_sum != 1.0f )
        {
            for ( tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++ )
            {
                dst->low._lighting[tnc] /= wt_sum;
                dst->hgh._lighting[tnc] /= wt_sum;
            }
        }

        // determine the lighting extents
        lighting_cache_t::max_light( dst );
    }

    return wt_sum > 0.0f;
}

//--------------------------------------------------------------------------------------------
float lighting_cache_test( const lighting_cache_t * src[], const float u, const float v, float * low_delta, float * hgh_delta )
{
    /// @author BB
    /// @details estimate the maximum change in the lighting at this point from the
    ///               measured delta values

    float delta, wt_sum;
    float loc_low_delta, loc_hgh_delta;
    float loc_u, loc_v;

    delta = 0.0f;

    if ( NULL == src ) return delta;

    // handle the optional parameters
    if ( NULL == low_delta ) low_delta = &loc_low_delta;
    if ( NULL == hgh_delta ) hgh_delta = &loc_hgh_delta;

    loc_u = CLIP( u, 0.0f, 1.0f );
    loc_v = CLIP( v, 0.0f, 1.0f );

    wt_sum = 0.0f;

    if ( NULL != src[0] )
    {
        float wt = ( 1.0f - loc_u ) * ( 1.0f - loc_v );

        delta      += wt * src[0]->_max_delta;
        *low_delta += wt * src[0]->low._max_delta;
        *hgh_delta += wt * src[0]->hgh._max_delta;

        wt_sum += wt;
    }

    if ( NULL != src[1] )
    {
        float wt = loc_u * ( 1.0f - loc_v );

        delta      += wt * src[1]->_max_delta;
        *low_delta += wt * src[1]->low._max_delta;
        *hgh_delta += wt * src[1]->hgh._max_delta;

        wt_sum += wt;
    }

    if ( NULL != src[2] )
    {
        float wt = ( 1.0f - loc_u ) * loc_v;

        delta      += wt * src[2]->_max_delta;
        *low_delta += wt * src[2]->low._max_delta;
        *hgh_delta += wt * src[2]->hgh._max_delta;

        wt_sum += wt;
    }

    if ( NULL != src[3] )
    {
        float wt = loc_u * loc_v;

        delta      += wt * src[3]->_max_delta;
        *low_delta += wt * src[3]->low._max_delta;
        *hgh_delta += wt * src[3]->hgh._max_delta;

        wt_sum += wt;
    }

    if ( wt_sum > 0.0f )
    {
        delta      /= wt_sum;
        *low_delta /= wt_sum;
        *hgh_delta /= wt_sum;
    }

    return delta;
}

//--------------------------------------------------------------------------------------------
bool lighting_sum_project( lighting_cache_t * dst, const lighting_cache_t * src, const Vector3f& vec, const int dir )
{
    if ( NULL == src || NULL == dst ) return false;

    if ( dir < 0 || dir > 4 || 0 != ( dir&1 ) )
        return false;

    if ( vec[kX] > 0 )
    {
        dst->low._lighting[dir+0] += vec[kX] * src->low._lighting[LVEC_PX];
        dst->low._lighting[dir+1] += vec[kX] * src->low._lighting[LVEC_MX];

        dst->hgh._lighting[dir+0] += vec[kX] * src->hgh._lighting[LVEC_PX];
        dst->hgh._lighting[dir+1] += vec[kX] * src->hgh._lighting[LVEC_MX];
    }
    else if ( vec[kX] < 0 )
    {
        dst->low._lighting[dir+0] -= vec[kX] * src->low._lighting[LVEC_MX];
        dst->low._lighting[dir+1] -= vec[kX] * src->low._lighting[LVEC_PX];

        dst->hgh._lighting[dir+0] -= vec[kX] * src->hgh._lighting[LVEC_MX];
        dst->hgh._lighting[dir+1] -= vec[kX] * src->hgh._lighting[LVEC_PX];
    }

    if ( vec[kY] > 0 )
    {
        dst->low._lighting[dir+0] += vec[kY] * src->low._lighting[LVEC_PY];
        dst->low._lighting[dir+1] += vec[kY] * src->low._lighting[LVEC_MY];

		dst->hgh._lighting[dir+0] += vec[kY] * src->hgh._lighting[LVEC_PY];
        dst->hgh._lighting[dir+1] += vec[kY] * src->hgh._lighting[LVEC_MY];
    }
    else if ( vec[kY] < 0 )
    {
        dst->low._lighting[dir+0] -= vec[kY] * src->low._lighting[LVEC_MY];
        dst->low._lighting[dir+1] -= vec[kY] * src->low._lighting[LVEC_PY];

        dst->hgh._lighting[dir+0] -= vec[kY] * src->hgh._lighting[LVEC_MY];
        dst->hgh._lighting[dir+1] -= vec[kY] * src->hgh._lighting[LVEC_PY];
    }

    if ( vec[kZ] > 0 )
    {
        dst->low._lighting[dir+0] += vec[kZ] * src->low._lighting[LVEC_PZ];
        dst->low._lighting[dir+1] += vec[kZ] * src->low._lighting[LVEC_MZ];

        dst->hgh._lighting[dir+0] += vec[kZ] * src->hgh._lighting[LVEC_PZ];
        dst->hgh._lighting[dir+1] += vec[kZ] * src->hgh._lighting[LVEC_MZ];
    }
    else if ( vec[kZ] < 0 )
    {
        dst->low._lighting[dir+0] -= vec[kZ] * src->low._lighting[LVEC_MZ];
        dst->low._lighting[dir+1] -= vec[kZ] * src->low._lighting[LVEC_PZ];

        dst->hgh._lighting[dir+0] -= vec[kZ] * src->hgh._lighting[LVEC_MZ];
        dst->hgh._lighting[dir+1] -= vec[kZ] * src->hgh._lighting[LVEC_PZ];
    }

    return true;
}

//--------------------------------------------------------------------------------------------
float lighting_evaluate_cache_base( const lighting_cache_base_t * lcache, const Vector3f& nrm, float * amb )
{
    float dir;
    float local_amb;

    // handle the optional parameter
    if ( NULL == amb ) amb = &local_amb;

    // check for valid data
    if ( NULL == lcache )
    {
        *amb = 0.0f;
        return 0.0f;
    }

    // evaluate the dir vector
    if ( 0.0f == lcache->_max_light )
    {
        // only ambient light, or black
        dir  = 0.0f;
        *amb = lcache->_lighting[LVEC_AMB];
    }
    else
    {
        lighting_vector_evaluate( lcache->_lighting, nrm, &dir, amb );
    }

    return dir + *amb;
}

//--------------------------------------------------------------------------------------------
float lighting_evaluate_cache( const lighting_cache_t * src, const Vector3f& nrm, const float z, const AABB3f& bbox, float * light_amb, float * light_dir )
{
    float loc_light_amb = 0.0f, loc_light_dir = 0.0f;
    float light_tot;
    float hgh_wt, low_wt, amb ;

    // check for valid parameters
    if ( NULL == src ) return 0.0f;

    // handle optional arguments
    if ( NULL == light_amb ) light_amb = &loc_light_amb;
    if ( NULL == light_dir ) light_dir = &loc_light_dir;

    // determine the weighting
    hgh_wt = ( z - bbox.getMin()[kZ] ) / ( bbox.getMax()[kZ] - bbox.getMin()[kZ] );
    hgh_wt = CLIP( hgh_wt, 0.0f, 1.0f );
    low_wt = 1.0f - hgh_wt;

    // initialize the output
    light_tot  = 0.0f;
    *light_amb = 0.0f;
    *light_dir = 0.0f;

    // optimize the use of the lighting_evaluate_cache_base() function
    if ( low_wt > 0.0f )
    {
        light_tot  += low_wt * lighting_evaluate_cache_base( &( src->low ), nrm, &amb );
        *light_amb += low_wt * amb;
    }

    // optimize the use of the lighting_evaluate_cache_base() function
    if ( hgh_wt > 0.0f )
    {
        light_tot  += hgh_wt * lighting_evaluate_cache_base( &( src->hgh ), nrm, &amb );
        *light_amb += hgh_wt * amb;
    }

    *light_dir = light_tot - ( *light_amb );

    return light_tot;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float dyna_lighting_intensity( const dynalight_data_t * pdyna, const Vector3f& diff )
{
    if ( NULL == pdyna || 0.0f == pdyna->level ) return 0.0f;

    float rho_sqr  = diff[kX] * diff[kX] + diff[kY] * diff[kY];
    float y2 = rho_sqr * 2.0f / 765.0f / pdyna->falloff;

    if ( y2 > 1.0f ) return false;

    float level = 1.0f - 0.5f * y2 * ( 3.0f - y2 * y2 );
    level *= pdyna->level;

    return level;
}

//--------------------------------------------------------------------------------------------
bool sum_dyna_lighting( const dynalight_data_t * pdyna, std::array<float, LIGHTING_VEC_SIZE> &lighting, const Vector3f& nrm )
{
    if ( NULL == pdyna ) return false;

    float level = 255 * dyna_lighting_intensity( pdyna, nrm );
    if ( 0.0f == level ) return true;

    // allow negative lighting, or blind spots will not work properly
	float rad_sqr = nrm.length_2();

    // make a local copy of the normal so we do not normalize the data in the calling function
	Vector3f local_nrm = nrm;

    // do the normalization
    if ( 1.0f != rad_sqr && 0.0f != rad_sqr )
    {
        float rad = std::sqrt( rad_sqr );
        local_nrm[kX] /= rad;
        local_nrm[kY] /= rad;
        local_nrm[kZ] /= rad;
    }

    // sum the lighting
    lighting_vector_sum( lighting, local_nrm, level, 0.0f );

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
dynalight_data_t * dynalight_data_t::init(dynalight_data_t *self)
{
	if (NULL == self) {
		return nullptr;
	}
	self->distance = 1000.0f;
	self->falloff = 255.0f;
	self->level = 0.0f;
	self->pos = Vector3f::zero();

	return self;
}
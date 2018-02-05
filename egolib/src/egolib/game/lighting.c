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

/// @file egolib/game/lighting.c
/// @brief Code for controlling the character and mesh lighting
/// @details

#include "egolib/game/lighting.h"

#include "egolib/egolib.h"

//--------------------------------------------------------------------------------------------

float light_a = 0.0f,
      light_d = 0.0f;
Vector3f light_nrm = idlib::zero<Vector3f>();

//--------------------------------------------------------------------------------------------

static bool lighting_sum_project( lighting_cache_t& dst, const lighting_cache_t& src, const Vector3f& vec, const int dir );

//--------------------------------------------------------------------------------------------

void lighting_vector_evaluate(const LightingVector& lvec, const Vector3f& nrm, float& dir, float& amb)
{
    dir = 0.0f;
    amb = 0.0f;

    if ( nrm[kX] > 0.0f )
    {
        dir += nrm[kX] * lvec[LVEC_PX];
    }
    else if ( nrm[kX] < 0.0f )
    {
        dir -= nrm[kX] * lvec[LVEC_MX];
    }

    if ( nrm[kY] > 0.0f )
    {
        dir += nrm[kY] * lvec[LVEC_PY];
    }
    else if ( nrm[kY] < 0.0f )
    {
        dir -= nrm[kY] * lvec[LVEC_MY];
    }

    if ( nrm[kZ] > 0.0f )
    {
        dir += nrm[kZ] * lvec[LVEC_PZ];
    }
    else if ( nrm[kZ] < 0.0f )
    {
        dir -= nrm[kZ] * lvec[LVEC_MZ];
    }

    // the ambient is not summed
    amb += lvec[LVEC_AMB];
}

void lighting_vector_sum(LightingVector& lvec, const Vector3f& nrm, const float direct, const float ambient)
{
    if ( nrm.x() > 0.0f )
    {
        lvec[LVEC_PX] += nrm.x() * direct;
    }
    else if ( nrm.x() < 0.0f )
    {
        lvec[LVEC_MX] -= nrm.x() * direct;
    }

    if ( nrm.y() > 0.0f )
    {
        lvec[LVEC_PY] += nrm.y() * direct;
    }
    else if ( nrm.y() < 0.0f )
    {
        lvec[LVEC_MY] -= nrm.y() * direct;
    }

    if ( nrm.z() > 0.0f )
    {
        lvec[LVEC_PZ] += nrm.z() * direct;
    }
    else if ( nrm.z() < 0.0f )
    {
        lvec[LVEC_MZ] -= nrm.z() * direct;
    }

    // the ambient is not summed
    lvec[LVEC_AMB] += ambient;
}

//--------------------------------------------------------------------------------------------

void lighting_cache_base_t::init()
{
	_max_delta = 0.0f;
	_max_light = 0.0f;
    _lighting.fill(0.0f);
}

void lighting_cache_base_t::max_light()
{
	float temporary = std::abs(_lighting[0]);
    for (size_t i = 1; i < (size_t)LIGHTING_VEC_SIZE - 1; ++i) {
        temporary = std::max(temporary, std::abs(_lighting[i]));
    }
	_max_light = temporary;
}

void lighting_cache_base_t::blend( lighting_cache_base_t& self, const lighting_cache_base_t& other, float keep )
{
    float max_delta;

    // blend this in with the existing lighting
    if ( 1.0f == keep )
    {
        // no change from last time
        max_delta = 0.0f;
    }
    else if ( 0.0f == keep )
    {
        max_delta = 0.0f;
        for (size_t i = 0; i < LIGHTING_VEC_SIZE; i++ )
        {
            float delta = std::abs(other._lighting[i] - self._lighting[i] );

			self._lighting[i] = other._lighting[i];

            max_delta = std::max( max_delta, delta );
        }
    }
    else
    {
        max_delta = 0.0f;
        for ( size_t i = 0; i < LIGHTING_VEC_SIZE; i++ )
        {
            float ftmp = self._lighting[i];
			self._lighting[i] = ftmp * keep + other._lighting[i] * ( 1.0f - keep );
            max_delta = std::max( max_delta, std::abs(self._lighting[i] - ftmp ) );
        }
    }

	self._max_delta = max_delta;
}

//--------------------------------------------------------------------------------------------

void lighting_cache_t::init()
{
	low.init();
	hgh.init();
	_max_delta = 0.0f;
	_max_light = 0.0f;
}

void lighting_cache_t::max_light()
{
	low.max_light();
	hgh.max_light();
	_max_light = std::max(low._max_light, hgh._max_light);
}

void lighting_cache_t::blend(lighting_cache_t& self, lighting_cache_t& other, float keep)
{
    // find deltas
    lighting_cache_base_t::blend( self.low, other.low, keep );
    lighting_cache_base_t::blend( self.hgh, other.hgh, keep );

    // find the absolute maximum delta
	self._max_delta = std::max(self.low._max_delta, self.hgh._max_delta);
}

void lighting_cache_t::lighting_project_cache( lighting_cache_t& dst, const lighting_cache_t& src, const Matrix4f4f& mat )
{
    // blank the destination lighting
    dst.init();

    // do the ambient
    dst.low._lighting[LVEC_AMB] = src.low._lighting[LVEC_AMB];
    dst.hgh._lighting[LVEC_AMB] = src.hgh._lighting[LVEC_AMB];

	if (src._max_light == 0.0f) {
		return;
	}

    // grab the character directions
    Vector3f fwd = mat_getChrForward(mat);
    Vector3f right = mat_getChrRight(mat);
    Vector3f up = mat_getChrUp(mat);

    fwd = normalize(fwd).get_vector();
    right = normalize(right).get_vector();
    up = normalize(up).get_vector();

    // split the lighting cache up
    lighting_sum_project( dst, src, right, 0 );
    lighting_sum_project( dst, src, fwd,   2 );
    lighting_sum_project( dst, src, up,    4 );

    // determine the lighting extents
    dst.max_light();
}

bool lighting_cache_t::lighting_cache_interpolate( lighting_cache_t& dst, const std::array<const lighting_cache_t *, 4>& src, const float u, const float v )
{
    dst.init();

	float loc_u = Ego::Math::constrain(u, 0.0f, 1.0f);
    float loc_v = Ego::Math::constrain(v, 0.0f, 1.0f);

	float wt_sum = 0.0f;

	if (NULL != src[0])
	{
		float wt = (1.0f - loc_u) * (1.0f - loc_v);
		for (size_t tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++)
		{
			dst.low._lighting[tnc] += src[0]->low._lighting[tnc] * wt;
			dst.hgh._lighting[tnc] += src[0]->hgh._lighting[tnc] * wt;
		}
		wt_sum += wt;
	}

	if (NULL != src[1])
	{
		float wt = loc_u * (1.0f - loc_v);
		for (size_t tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++)
		{
			dst.low._lighting[tnc] += src[1]->low._lighting[tnc] * wt;
			dst.hgh._lighting[tnc] += src[1]->hgh._lighting[tnc] * wt;
		}
		wt_sum += wt;
	}

	if (NULL != src[2])
	{
		float wt = (1.0f - loc_u) * loc_v;
		for (size_t tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++)
		{
			dst.low._lighting[tnc] += src[2]->low._lighting[tnc] * wt;
			dst.hgh._lighting[tnc] += src[2]->hgh._lighting[tnc] * wt;
		}
		wt_sum += wt;
	}

	if (NULL != src[3])
	{
		float wt = loc_u * loc_v;
		for (size_t tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++)
		{
			dst.low._lighting[tnc] += src[3]->low._lighting[tnc] * wt;
			dst.hgh._lighting[tnc] += src[3]->hgh._lighting[tnc] * wt;
		}
		wt_sum += wt;
	}

	if (wt_sum > 0.0f)
	{
		if (wt_sum != 1.0f)
		{
			for (size_t tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++)
			{
				dst.low._lighting[tnc] /= wt_sum;
				dst.hgh._lighting[tnc] /= wt_sum;
			}
		}

		// determine the lighting extents
		dst.max_light();
	}

    return wt_sum > 0.0f;
}

//--------------------------------------------------------------------------------------------
float lighting_cache_test( const lighting_cache_t * src[], const float u, const float v, float& low_delta, float& hgh_delta )
{
    /// @author BB
    /// @details estimate the maximum change in the lighting at this point from the
    ///               measured delta values

    float delta, wt_sum;
    float loc_u, loc_v;

    delta = 0.0f;

    if ( NULL == src ) return delta;


    loc_u = Ego::Math::constrain( u, 0.0f, 1.0f );
    loc_v = Ego::Math::constrain( v, 0.0f, 1.0f );

    wt_sum = 0.0f;

    if ( NULL != src[0] )
    {
        float wt = ( 1.0f - loc_u ) * ( 1.0f - loc_v );

        delta      += wt * src[0]->_max_delta;
        low_delta += wt * src[0]->low._max_delta;
        hgh_delta += wt * src[0]->hgh._max_delta;

        wt_sum += wt;
    }

    if ( NULL != src[1] )
    {
        float wt = loc_u * ( 1.0f - loc_v );

        delta      += wt * src[1]->_max_delta;
        low_delta += wt * src[1]->low._max_delta;
        hgh_delta += wt * src[1]->hgh._max_delta;

        wt_sum += wt;
    }

    if ( NULL != src[2] )
    {
        float wt = ( 1.0f - loc_u ) * loc_v;

        delta     += wt * src[2]->_max_delta;
        low_delta += wt * src[2]->low._max_delta;
        hgh_delta += wt * src[2]->hgh._max_delta;

        wt_sum += wt;
    }

    if ( NULL != src[3] )
    {
        float wt = loc_u * loc_v;

        delta     += wt * src[3]->_max_delta;
        low_delta += wt * src[3]->low._max_delta;
        hgh_delta += wt * src[3]->hgh._max_delta;

        wt_sum += wt;
    }

    if ( wt_sum > 0.0f )
    {
        delta     /= wt_sum;
        low_delta /= wt_sum;
        hgh_delta /= wt_sum;
    }

    return delta;
}

//--------------------------------------------------------------------------------------------
bool lighting_sum_project( lighting_cache_t& dst, const lighting_cache_t& src, const Vector3f& vec, const int dir )
{
    if ( dir < 0 || dir > 4 || 0 != ( dir&1 ) )
        return false;

    if ( vec[kX] > 0 )
    {
        dst.low._lighting[dir+0] += vec[kX] * src.low._lighting[LVEC_PX];
        dst.low._lighting[dir+1] += vec[kX] * src.low._lighting[LVEC_MX];

        dst.hgh._lighting[dir+0] += vec[kX] * src.hgh._lighting[LVEC_PX];
        dst.hgh._lighting[dir+1] += vec[kX] * src.hgh._lighting[LVEC_MX];
    }
    else if ( vec[kX] < 0 )
    {
        dst.low._lighting[dir+0] -= vec[kX] * src.low._lighting[LVEC_MX];
        dst.low._lighting[dir+1] -= vec[kX] * src.low._lighting[LVEC_PX];

        dst.hgh._lighting[dir+0] -= vec[kX] * src.hgh._lighting[LVEC_MX];
        dst.hgh._lighting[dir+1] -= vec[kX] * src.hgh._lighting[LVEC_PX];
    }

    if ( vec[kY] > 0 )
    {
        dst.low._lighting[dir+0] += vec[kY] * src.low._lighting[LVEC_PY];
        dst.low._lighting[dir+1] += vec[kY] * src.low._lighting[LVEC_MY];

		dst.hgh._lighting[dir+0] += vec[kY] * src.hgh._lighting[LVEC_PY];
        dst.hgh._lighting[dir+1] += vec[kY] * src.hgh._lighting[LVEC_MY];
    }
    else if ( vec[kY] < 0 )
    {
        dst.low._lighting[dir+0] -= vec[kY] * src.low._lighting[LVEC_MY];
        dst.low._lighting[dir+1] -= vec[kY] * src.low._lighting[LVEC_PY];

        dst.hgh._lighting[dir+0] -= vec[kY] * src.hgh._lighting[LVEC_MY];
        dst.hgh._lighting[dir+1] -= vec[kY] * src.hgh._lighting[LVEC_PY];
    }

    if ( vec[kZ] > 0 )
    {
        dst.low._lighting[dir+0] += vec[kZ] * src.low._lighting[LVEC_PZ];
        dst.low._lighting[dir+1] += vec[kZ] * src.low._lighting[LVEC_MZ];

        dst.hgh._lighting[dir+0] += vec[kZ] * src.hgh._lighting[LVEC_PZ];
        dst.hgh._lighting[dir+1] += vec[kZ] * src.hgh._lighting[LVEC_MZ];
    }
    else if ( vec[kZ] < 0 )
    {
        dst.low._lighting[dir+0] -= vec[kZ] * src.low._lighting[LVEC_MZ];
        dst.low._lighting[dir+1] -= vec[kZ] * src.low._lighting[LVEC_PZ];

        dst.hgh._lighting[dir+0] -= vec[kZ] * src.hgh._lighting[LVEC_MZ];
        dst.hgh._lighting[dir+1] -= vec[kZ] * src.hgh._lighting[LVEC_PZ];
    }

    return true;
}

//--------------------------------------------------------------------------------------------
float lighting_cache_base_t::evaluate( const lighting_cache_base_t& self, const Vector3f& nrm, float& amb )
{
    float dir;

    // evaluate the dir vector
    if ( 0.0f == self._max_light )
    {
        // only ambient light, or black
        dir  = 0.0f;
        amb = self._lighting[LVEC_AMB];
    }
    else
    {
		lighting_vector_evaluate(self._lighting, nrm, dir, amb );
    }

    return dir + amb;
}

float lighting_cache_t::lighting_evaluate_cache( const lighting_cache_t& src, const Vector3f& nrm, const float z, const AxisAlignedBox3f& bbox, float * light_amb, float * light_dir )
{
    float loc_light_amb = 0.0f, loc_light_dir = 0.0f;
    float light_tot;
    float hgh_wt, low_wt, amb ;


    // handle optional arguments
    if ( NULL == light_amb ) light_amb = &loc_light_amb;
    if ( NULL == light_dir ) light_dir = &loc_light_dir;

    // determine the weighting
    hgh_wt = ( z - bbox.get_min()[kZ] ) / ( bbox.get_max()[kZ] - bbox.get_min()[kZ] );
    hgh_wt = Ego::Math::constrain( hgh_wt, 0.0f, 1.0f );
    low_wt = 1.0f - hgh_wt;

    // initialize the output
    light_tot  = 0.0f;
    *light_amb = 0.0f;
    *light_dir = 0.0f;

    // optimize the use of the lighting_cache_base_t::evaluate() function
    if ( low_wt > 0.0f )
    {
        light_tot  += low_wt * lighting_cache_base_t::evaluate( src.low, nrm, amb );
        *light_amb += low_wt * amb;
    }

    // optimize the use of the lighting_cache_base_t::evaluate() function
    if ( hgh_wt > 0.0f )
    {
        light_tot  += hgh_wt * lighting_cache_base_t::evaluate( src.hgh, nrm, amb );
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
bool sum_dyna_lighting( const dynalight_data_t * pdyna, LightingVector& lighting, const Vector3f& nrm )
{
    if ( NULL == pdyna ) return false;

    float level = 255.0f * dyna_lighting_intensity( pdyna, nrm );
    if ( 0.0f == level ) return true;

    // allow negative lighting, or blind spots will not work properly
	float rad_sqr = idlib::squared_euclidean_norm(nrm);

    // make a local copy of the normal so we do not normalize the data in the calling function
	Vector3f local_nrm = nrm;

    // do the normalization
    if ( 1.0f != rad_sqr && 0.0f != rad_sqr )
    {
        float rad = std::sqrt( rad_sqr );
        local_nrm.x() /= rad;
        local_nrm.y() /= rad;
        local_nrm.z() /= rad;
    }

    // sum the lighting
    lighting_vector_sum(lighting, local_nrm, level, 0.0f);

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void dynalight_data_t::init(dynalight_data_t& self)
{
	self.distance = 1000.0f;
	self.falloff = 255.0f;
	self.level = 0.0f;
	self.pos = idlib::zero<Vector3f>();
}

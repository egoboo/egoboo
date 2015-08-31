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

/// @file game/CharacterMatrix.c

#include "game/CharacterMatrix.h"
#include "game/graphic_mad.h"
#include "game/renderer_3d.h"
#include "game/Entities/_Include.hpp"

static int get_grip_verts( Uint16 grip_verts[], const CHR_REF imount, int vrt_offset );

static egolib_rv matrix_cache_needs_update( Object * pchr, matrix_cache_t * pmc );
static bool apply_matrix_cache( Object * pchr, matrix_cache_t * mc_tmp );
static bool chr_get_matrix_cache( Object * pchr, matrix_cache_t * mc_tmp );

static bool apply_one_character_matrix( Object * pchr, matrix_cache_t * mcache );
static bool apply_one_weapon_matrix( Object * pweap, matrix_cache_t * mcache );

static int convert_grip_to_local_points( Object * pholder, Uint16 grip_verts[], Vector4f   dst_point[] );
static int convert_grip_to_global_points( const CHR_REF iholder, Uint16 grip_verts[], Vector4f   dst_point[] );

// definition that is consistent with using it as a callback in qsort() or some similar function
static int  cmp_matrix_cache( const void * vlhs, const void * vrhs );


bool chr_matrix_valid( const Object * pchr )
{
    /// @author BB
    /// @details Determine whether the character has a valid matrix

    if ( nullptr == ( pchr ) ) return false;

    // both the cache and the matrix need to be valid
    return pchr->inst.matrix_cache.valid && pchr->inst.matrix_cache.matrix_valid;
}


int get_grip_verts( Uint16 grip_verts[], const CHR_REF imount, int vrt_offset )
{
    /// @author BB
    /// @details Fill the grip_verts[] array from the mount's data.
    ///     Return the number of vertices found.

    Uint32  i;
    int vrt_count, tnc;

    Object * pmount;

    if ( NULL == grip_verts ) return 0;

    // set all the vertices to a "safe" value
    for ( i = 0; i < GRIP_VERTS; i++ )
    {
        grip_verts[i] = 0xFFFF;
    }

    if ( !_currentModule->getObjectHandler().exists( imount ) ) return 0;
    pmount = _currentModule->getObjectHandler().get( imount );

    if ( 0 == pmount->inst.vrt_count ) return 0;

    //---- set the proper weapongrip vertices
    tnc = ( int )pmount->inst.vrt_count - ( int )vrt_offset;

    // if the starting vertex is less than 0, just take the first vertex
    if ( tnc < 0 )
    {
        grip_verts[0] = 0;
        return 1;
    }

    vrt_count = 0;
    for ( i = 0; i < GRIP_VERTS; i++ )
    {
        if ( tnc + i < pmount->inst.vrt_count )
        {
            grip_verts[i] = tnc + i;
            vrt_count++;
        }
        else
        {
            grip_verts[i] = 0xFFFF;
        }
    }

    return vrt_count;
}

//--------------------------------------------------------------------------------------------
bool chr_get_matrix_cache( Object * pchr, matrix_cache_t * mc_tmp )
{
    /// @author BB
    /// @details grab the matrix cache data for a given character and put it into mc_tmp.

    bool handled;
    CHR_REF itarget, ichr;

    if ( NULL == mc_tmp ) return false;
    if ( nullptr == ( pchr ) ) return false;
    ichr = GET_INDEX_PCHR( pchr );

    handled = false;
    itarget = INVALID_CHR_REF;

    // initialize xome parameters in case we fail
    mc_tmp->valid     = false;
    mc_tmp->type_bits = MAT_UNKNOWN;

    mc_tmp->self_scale[kX] = mc_tmp->self_scale[kY] = mc_tmp->self_scale[kZ] = pchr->fat;

    // handle the overlay first of all
    if ( !handled && pchr->is_overlay && ichr != pchr->ai.target && _currentModule->getObjectHandler().exists( pchr->ai.target ) )
    {
        // this will pretty much fail the cmp_matrix_cache() every time...

        Object * ptarget = _currentModule->getObjectHandler().get( pchr->ai.target );

        // make sure we have the latst info from the target
        chr_update_matrix( ptarget, true );

        // grab the matrix cache into from the character we are overlaying
        memcpy( mc_tmp, &( ptarget->inst.matrix_cache ), sizeof( matrix_cache_t ) );

        // just in case the overlay's matrix cannot be corrected
        // then treat it as if it is not an overlay
        handled = mc_tmp->valid;
    }

    // this will happen if the overlay "failed" or for any non-overlay character
    if ( !handled )
    {
        // assume that the "target" of the MAT_CHARACTER data will be the character itself
        itarget = GET_INDEX_PCHR( pchr );

        //---- update the MAT_WEAPON data
        if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
        {
            Object * pmount = _currentModule->getObjectHandler().get( pchr->attachedto );

            // make sure we have the latst info from the target
            chr_update_matrix( pmount, true );

            // just in case the mounts's matrix cannot be corrected
            // then treat it as if it is not mounted... yuck
            if ( pmount->inst.matrix_cache.matrix_valid )
            {
                mc_tmp->valid     = true;
                SET_BIT( mc_tmp->type_bits, MAT_WEAPON );        // add in the weapon data

                mc_tmp->grip_chr  = pchr->attachedto;
                mc_tmp->grip_slot = pchr->inwhich_slot;
                get_grip_verts( mc_tmp->grip_verts.data(), pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );

                itarget = pchr->attachedto;
            }
        }

        //---- update the MAT_CHARACTER data
        if ( _currentModule->getObjectHandler().exists( itarget ) )
        {
            Object * ptarget = _currentModule->getObjectHandler().get( itarget );

            mc_tmp->valid   = true;
            SET_BIT( mc_tmp->type_bits, MAT_CHARACTER );  // add in the MAT_CHARACTER-type data for the object we are "connected to"

            mc_tmp->rotate[kX] = CLIP_TO_16BITS( ptarget->ori.map_twist_facing_x - MAP_TURN_OFFSET );
            mc_tmp->rotate[kY] = CLIP_TO_16BITS( ptarget->ori.map_twist_facing_y - MAP_TURN_OFFSET );
            mc_tmp->rotate[kZ] = ptarget->ori.facing_z;

            mc_tmp->pos = ptarget->getPosition();

            mc_tmp->grip_scale[kX] = mc_tmp->grip_scale[kY] = mc_tmp->grip_scale[kZ] = ptarget->fat;
        }
    }

    return mc_tmp->valid;
}

//--------------------------------------------------------------------------------------------
int convert_grip_to_local_points( Object * pholder, Uint16 grip_verts[], Vector4f dst_point[] )
{
    /// @author ZZ
    /// @details a helper function for apply_one_weapon_matrix()

    int cnt, point_count;

    if ( NULL == grip_verts || NULL == dst_point ) return 0;

    if (!pholder || pholder->isTerminated()) return 0;

    // count the valid weapon connection dst_points
    point_count = 0;
    for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
    {
        if ( 0xFFFF != grip_verts[cnt] )
        {
            point_count++;
        }
    }

    // do the best we can
    if ( 0 == point_count )
    {
        // punt! attach to origin
        dst_point[0][kX] = pholder->getPosX();
        dst_point[0][kY] = pholder->getPosY();
        dst_point[0][kZ] = pholder->getPosZ();
        dst_point[0][kW] = 1;

        point_count = 1;
    }
    else
    {
        // update the grip vertices
        chr_instance_t::update_grip_verts(pholder->inst, grip_verts, GRIP_VERTS );

        // copy the vertices into dst_point[]
        for ( point_count = 0, cnt = 0; cnt < GRIP_VERTS; cnt++, point_count++ )
        {
            Uint16 vertex = grip_verts[cnt];

            if ( 0xFFFF == vertex ) continue;

            dst_point[point_count][kX] = pholder->inst.vrt_lst[vertex].pos[XX];
            dst_point[point_count][kY] = pholder->inst.vrt_lst[vertex].pos[YY];
            dst_point[point_count][kZ] = pholder->inst.vrt_lst[vertex].pos[ZZ];
            dst_point[point_count][kW] = 1.0f;
        }
    }

    return point_count;
}

//--------------------------------------------------------------------------------------------
int convert_grip_to_global_points( const CHR_REF iholder, Uint16 grip_verts[], Vector4f   dst_point[] )
{
    /// @author ZZ
    /// @details a helper function for apply_one_weapon_matrix()

    int       point_count;
	Vector4f  src_point[GRIP_VERTS];

    if ( !_currentModule->getObjectHandler().exists( iholder ) ) return 0;
    Object *pholder = _currentModule->getObjectHandler().get( iholder );

    // find the grip points in the character's "local" or "body-fixed" coordinates
    point_count = convert_grip_to_local_points( pholder, grip_verts, src_point );

    if ( 0 == point_count ) return 0;

    // use the math function instead of rolling out own
	Utilities::transform(pholder->inst.matrix,src_point, dst_point, point_count);

    return point_count;
}

//--------------------------------------------------------------------------------------------
bool apply_one_weapon_matrix( Object * pweap, matrix_cache_t * mc_tmp )
{
    /// @author ZZ
    /// @details Request that the data in the matrix cache be used to create a "character matrix".
    ///               i.e. a matrix that is not being held by anything.

	Vector4f  nupoint[GRIP_VERTS];
    int       iweap_points;

    matrix_cache_t * pweap_mcache;

    if ( NULL == mc_tmp || !mc_tmp->valid || 0 == ( MAT_WEAPON & mc_tmp->type_bits ) ) return false;

    if ( nullptr == ( pweap ) ) return false;
    pweap_mcache = &( pweap->inst.matrix_cache );

    if ( !_currentModule->getObjectHandler().exists( mc_tmp->grip_chr ) ) return false;

    // make sure that the matrix is invalid incase of an error
    pweap_mcache->matrix_valid = false;

    // grab the grip points in world coordinates
    iweap_points = convert_grip_to_global_points( mc_tmp->grip_chr, mc_tmp->grip_verts.data(), nupoint );

    if ( 4 == iweap_points )
    {
        // Calculate weapon's matrix based on positions of grip points
        // chrscale is recomputed at time of attachment
        mat_FourPoints( pweap->inst.matrix, Vector3f(nupoint[0][kX], nupoint[0][kY], nupoint[0][kZ]),
			                                Vector3f(nupoint[1][kX], nupoint[1][kY], nupoint[1][kZ]),
			                                Vector3f(nupoint[2][kX], nupoint[2][kY], nupoint[2][kZ]),
			                                Vector3f(nupoint[3][kX], nupoint[3][kY], nupoint[3][kZ]), mc_tmp->self_scale[kZ]);

        // update the weapon position
        pweap->setPosition(Vector3f(nupoint[3][kX],nupoint[3][kY],nupoint[3][kZ]));

        memcpy( &( pweap->inst.matrix_cache ), mc_tmp, sizeof( matrix_cache_t ) );

        pweap_mcache->matrix_valid = true;
    }
    else if ( iweap_points > 0 )
    {
        // cannot find enough vertices. punt.
        // ignore the shape of the grip and just stick the character to the single mount point

        // update the character position
        pweap->setPosition(Vector3f(nupoint[0][kX],nupoint[0][kY],nupoint[0][kZ]));

        // make sure we have the right data
        chr_get_matrix_cache( pweap, mc_tmp );

        // add in the appropriate mods
        // this is a hybrid character and weapon matrix
        SET_BIT( mc_tmp->type_bits, MAT_CHARACTER );

        // treat it like a normal character matrix
        apply_one_character_matrix( pweap, mc_tmp );
    }

    return pweap_mcache->matrix_valid;
}

//--------------------------------------------------------------------------------------------
bool apply_one_character_matrix( Object * pchr, matrix_cache_t * mc_tmp )
{
    /// @author ZZ
    /// @details Request that the matrix cache data be used to create a "weapon matrix".
    ///               i.e. a matrix that is attached to a specific grip.

    if ( NULL == mc_tmp ) return false;

    // only apply character matrices using this function
    if ( 0 == ( MAT_CHARACTER & mc_tmp->type_bits ) ) return false;

    pchr->inst.matrix_cache.matrix_valid = false;

    if ( nullptr == ( pchr ) ) return false;

    if ( pchr->getProfile()->hasStickyButt() )
    {
        mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(
            pchr->inst.matrix,
            mc_tmp->self_scale,
            TO_TURN( mc_tmp->rotate[kZ] ), TO_TURN( mc_tmp->rotate[kX] ), TO_TURN( mc_tmp->rotate[kY] ),
            mc_tmp->pos);
    }
    else
    {
        mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(
            pchr->inst.matrix,
            mc_tmp->self_scale,
            TO_TURN( mc_tmp->rotate[kZ] ), TO_TURN( mc_tmp->rotate[kX] ), TO_TURN( mc_tmp->rotate[kY] ),
            mc_tmp->pos);
    }

    memcpy( &( pchr->inst.matrix_cache ), mc_tmp, sizeof( matrix_cache_t ) );

    pchr->inst.matrix_cache.matrix_valid = true;

    return true;
}

//--------------------------------------------------------------------------------------------
bool apply_matrix_cache( Object * pchr, matrix_cache_t * mc_tmp )
{
    /// @author BB
    /// @details request that the info in the matrix cache mc_tmp, be used to
    ///               make a matrix for the character pchr.

    bool applied = false;

    if ( nullptr == ( pchr ) ) return false;
    if ( NULL == mc_tmp || !mc_tmp->valid ) return false;

    if ( 0 != ( MAT_WEAPON & mc_tmp->type_bits ) )
    {
        if ( _currentModule->getObjectHandler().exists( mc_tmp->grip_chr ) )
        {
            applied = apply_one_weapon_matrix( pchr, mc_tmp );
        }
        else
        {
            matrix_cache_t * mcache = &( pchr->inst.matrix_cache );

            // !!!the mc_tmp was mis-labeled as a MAT_WEAPON!!!
            make_one_character_matrix( GET_INDEX_PCHR( pchr ) );

            // recover the matrix_cache values from the character
            SET_BIT( mcache->type_bits, MAT_CHARACTER );
            if ( mcache->matrix_valid )
            {
                mcache->valid     = true;
                mcache->type_bits = MAT_CHARACTER;

                mcache->self_scale[kX] =
                    mcache->self_scale[kY] =
                        mcache->self_scale[kZ] = pchr->fat;

                mcache->grip_scale = mcache->self_scale;

                mcache->rotate[kX] = CLIP_TO_16BITS( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET );
                mcache->rotate[kY] = CLIP_TO_16BITS( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET );
                mcache->rotate[kZ] = pchr->ori.facing_z;

                mcache->pos =pchr->getPosition();

                applied = true;
            }
        }
    }
    else if ( 0 != ( MAT_CHARACTER & mc_tmp->type_bits ) )
    {
        applied = apply_one_character_matrix( pchr, mc_tmp );
    }

    if ( applied )
    {
        chr_instance_t::apply_reflection_matrix(pchr->inst, pchr->enviro.grid_level );
    }

    return applied;
}

//--------------------------------------------------------------------------------------------
int cmp_matrix_cache( const void * vlhs, const void * vrhs )
{
    /// @author BB
    /// @details check for differences between the data pointed to
    ///     by vlhs and vrhs, assuming that they point to matrix_cache_t data.
    ///
    ///    The function is implemented this way so that in pronciple
    ///    if could be used in a function like qsort().
    ///
    ///    We could almost certainly make something easier and quicker by
    ///    using the function memcmp()

    int   itmp, cnt;
    float ftmp;

    matrix_cache_t * plhs = ( matrix_cache_t * )vlhs;
    matrix_cache_t * prhs = ( matrix_cache_t * )vrhs;

    // handle problems with pointers
    if ( plhs == prhs )
    {
        return 0;
    }
    else if ( NULL == plhs )
    {
        return 1;
    }
    else if ( NULL == prhs )
    {
        return -1;
    }

    // handle one of both if the matrix caches being invalid
    if ( !plhs->valid && !prhs->valid )
    {
        return 0;
    }
    else if ( !plhs->valid )
    {
        return 1;
    }
    else if ( !prhs->valid )
    {
        return -1;
    }

    // handle differences in the type
    itmp = plhs->type_bits - prhs->type_bits;
    if ( 0 != itmp ) goto cmp_matrix_cache_end;

    //---- check for differences in the MAT_WEAPON data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_WEAPON ) )
    {
        itmp = ( signed )REF_TO_INT( plhs->grip_chr ) - ( signed )REF_TO_INT( prhs->grip_chr );
        if ( 0 != itmp ) goto cmp_matrix_cache_end;

        itmp = ( signed )plhs->grip_slot - ( signed )prhs->grip_slot;
        if ( 0 != itmp ) goto cmp_matrix_cache_end;

        for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
        {
            itmp = ( signed )plhs->grip_verts[cnt] - ( signed )prhs->grip_verts[cnt];
            if ( 0 != itmp ) goto cmp_matrix_cache_end;
        }

        // handle differences in the scale of our mount
        for ( cnt = 0; cnt < 3; cnt ++ )
        {
            ftmp = plhs->grip_scale[cnt] - prhs->grip_scale[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }
    }

    //---- check for differences in the MAT_CHARACTER data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_CHARACTER ) )
    {
        // handle differences in the "Euler" rotation angles in 16-bit form
        for ( cnt = 0; cnt < 3; cnt++ )
        {
            ftmp = plhs->rotate[cnt] - prhs->rotate[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }

        // handle differences in the translate vector
        for ( cnt = 0; cnt < 3; cnt++ )
        {
            ftmp = plhs->pos[cnt] - prhs->pos[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }
    }

    //---- check for differences in the shared data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_WEAPON ) || HAS_SOME_BITS( plhs->type_bits, MAT_CHARACTER ) )
    {
        // handle differences in our own scale
        for ( cnt = 0; cnt < 3; cnt ++ )
        {
            ftmp = plhs->self_scale[cnt] - prhs->self_scale[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }
    }

    // if it got here, the data is all the same
    itmp = 0;

cmp_matrix_cache_end:

    return SGN( itmp );
}

//--------------------------------------------------------------------------------------------
egolib_rv matrix_cache_needs_update( Object * pchr, matrix_cache_t * pmc )
{
    /// @author BB
    /// @details determine whether a matrix cache has become invalid and needs to be updated

    matrix_cache_t local_mc;
    bool needs_cache_update;

    if ( nullptr == ( pchr ) ) return rv_error;

    if ( NULL == pmc ) pmc = &local_mc;

    // get the matrix data that is supposed to be used to make the matrix
    chr_get_matrix_cache( pchr, pmc );

    // compare that data to the actual data used to make the matrix
    needs_cache_update = ( 0 != cmp_matrix_cache( pmc, &( pchr->inst.matrix_cache ) ) );

    return needs_cache_update ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_update_matrix( Object * pchr, bool update_size )
{
    /// @author BB
    /// @details Do everything necessary to set the current matrix for this character.
    ///     This might include recursively going down the list of this character's mounts, etc.
    ///
    ///     Return true if a new matrix is applied to the character, false otherwise.

    egolib_rv      retval;
    bool         needs_update = false;
    bool         applied      = false;
    matrix_cache_t mc_tmp;
    matrix_cache_t *pchr_mc = NULL;

    if ( nullptr == ( pchr ) ) return rv_error;
    pchr_mc = &( pchr->inst.matrix_cache );

    // recursively make sure that any mount matrices are updated
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        egolib_rv attached_update = chr_update_matrix( _currentModule->getObjectHandler().get( pchr->attachedto ), true );

        // if this fails, we should probably do something...
        if ( rv_error == attached_update )
        {
            // there is an error so this matrix is not defined and no readon to go farther
            pchr_mc->matrix_valid = false;
            return attached_update;
        }
        else if ( rv_success == attached_update )
        {
            // the holder/mount matrix has changed.
            // this matrix is no longer valid.
            pchr_mc->matrix_valid = false;
        }
    }

    // does the matrix cache need an update at all?
    retval = matrix_cache_needs_update( pchr, &mc_tmp );
    if ( rv_error == retval ) return rv_error;
    needs_update = ( rv_success == retval );

    // Update the grip vertices no matter what (if they are used)
    if ( HAS_SOME_BITS( mc_tmp.type_bits, MAT_WEAPON ) && _currentModule->getObjectHandler().exists( mc_tmp.grip_chr ) )
    {
        egolib_rv grip_retval;
        Object   * ptarget = _currentModule->getObjectHandler().get( mc_tmp.grip_chr );

        // has that character changes its animation?
        grip_retval = ( egolib_rv )chr_instance_t::update_grip_verts(ptarget->inst, mc_tmp.grip_verts.data(), GRIP_VERTS);

        if ( rv_error   == grip_retval ) return rv_error;
        if ( rv_success == grip_retval ) needs_update = true;
    }

    // if it is not the same, make a new matrix with the new data
    applied = false;
    if ( needs_update )
    {
        // we know the matrix is not valid
        pchr_mc->matrix_valid = false;

        applied = apply_matrix_cache( pchr, &mc_tmp );
    }

    if ( applied && update_size )
    {
        // call chr_update_collision_size() but pass in a false value to prevent a recursize call
        chr_update_collision_size( pchr, false );
    }

    return applied ? rv_success : rv_fail;
}


//--------------------------------------------------------------------------------------------
bool chr_getMatUp(Object *pchr, Vector3f& up)
{
	bool rv;

	if (nullptr == (pchr)) return false;

	if (!chr_matrix_valid(pchr))
	{
		chr_update_matrix(pchr, true);
	}

	rv = false;
	if (chr_matrix_valid(pchr))
	{
        rv = true;
        up = mat_getChrUp(pchr->inst.matrix);
	}

	if (!rv)
	{
		// assume default Up is +z
		up[kZ] = 1.0f;
		up[kX] = up[kY] = 0.0f;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool chr_getMatRight(Object *pchr, Vector3f& right)
{
	bool rv;

	if (nullptr == (pchr)) return false;

	if (!chr_matrix_valid(pchr))
	{
		chr_update_matrix(pchr, true);
	}

	rv = false;
	if (chr_matrix_valid(pchr))
	{
        rv = true;
        right = mat_getChrRight(pchr->inst.matrix);
	}

	if (!rv)
	{
		// assume default Right is +y
		right[kY] = 1.0f;
		right[kX] = right[kZ] = 0.0f;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool chr_getMatForward(Object *pchr, Vector3f& forward)
{
	bool rv;

	if (nullptr == (pchr)) return false;

	if (!chr_matrix_valid(pchr))
	{
		chr_update_matrix(pchr, true);
	}

	rv = false;
	if (chr_matrix_valid(pchr))
	{
        rv = true;
        forward = mat_getChrForward(pchr->inst.matrix);
	}

	if (!rv)
	{
		// assume default Forward is +x
		forward[kX] = 1.0f;
		forward[kY] = forward[kZ] = 0.0f;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool chr_getMatTranslate(Object *pchr, Vector3f& translate)
{
	bool rv;

	if (nullptr == (pchr)) return false;

	if (!chr_matrix_valid(pchr))
	{
		chr_update_matrix(pchr, true);
	}

	rv = false;
	if (chr_matrix_valid(pchr))
	{
        rv = true;
        translate = mat_getTranslate(pchr->inst.matrix);
	}

	if (!rv)
	{
		translate = pchr->getPosition();
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool chr_calc_grip_cv( Object * pmount, int grip_offset, oct_bb_t * grip_cv_ptr, const bool shift_origin )
{
    /// @author BB
    /// @details use a standard size for the grip

    // take the character size from the adventurer model
    const float default_chr_height = 88.0f;
    const float default_chr_radius = 22.0f;

    int              cnt;

    oct_bb_t         tmp_cv;

    int      grip_count;
    Uint16   grip_verts[GRIP_VERTS];
	Vector4f grip_points[GRIP_VERTS];
	Vector4f grip_nupoints[GRIP_VERTS];
    bumper_t bmp;

	if (!pmount) {
		return false;
	}

    // alias this variable for notation simplicity
	chr_instance_t& pmount_inst = pmount->inst;

    // tune the grip radius
    bmp.size     = default_chr_radius * pmount->fat * 0.75f;
    bmp.height   = default_chr_height * pmount->fat * 2.00f;
    bmp.size_big = bmp.size * Ego::Math::sqrtTwo<float>();

    tmp_cv.assign(bmp);

    // move the vertical bounding box down a little
    tmp_cv._mins[OCT_Z] -= bmp.height * 0.25f;
    tmp_cv._maxs[OCT_Z] -= bmp.height * 0.25f;

    // get appropriate vertices for this model's grip
    {
        // do the automatic vertex update
		int vert_stt = (signed)(pmount_inst.vrt_count) - (signed)grip_offset;
        if ( vert_stt < 0 ) return false;

		if (gfx_error == chr_instance_t::update_vertices(pmount_inst, vert_stt, vert_stt + grip_offset, false))
        {
            grip_count = 0;
            for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
            {
                grip_verts[cnt] = 0xFFFF;
            }
        }
        else
        {
            // calculate the grip vertices
			for (grip_count = 0, cnt = 0; cnt < GRIP_VERTS && (size_t)(vert_stt + cnt) < pmount_inst.vrt_count; grip_count++, cnt++)
            {
                grip_verts[cnt] = vert_stt + cnt;
            }
            for ( /* nothing */ ; cnt < GRIP_VERTS; cnt++ )
            {
                grip_verts[cnt] = 0xFFFF;
            }
        }

        // calculate grip_origin and grip_up
        if ( 4 == grip_count )
        {
            // Calculate grip point locations with linear interpolation and other silly things
            convert_grip_to_local_points( pmount, grip_verts, grip_points );
        }
        else if ( grip_count > 0 )
        {
            // Calculate grip point locations with linear interpolation and other silly things
            convert_grip_to_local_points( pmount, grip_verts, grip_points );

            if ( grip_count < 2 )
            {
                grip_points[2] = Vector4f::zero();
                grip_points[2][kY] = 1.0f;
            }

            if ( grip_count < 3 )
            {
                grip_points[3] = Vector4f::zero();
                grip_points[3][kZ] = 1.0f;
            }
        }
        else if ( 0 == grip_count )
        {
            // choose the location point at the model's origin and axis aligned

            for ( cnt = 0; cnt < 4; cnt++ )
            {
                grip_points[cnt] = Vector4f::zero();
            }

            grip_points[1][kX] = 1.0f;
            grip_points[2][kY] = 1.0f;
            grip_points[3][kZ] = 1.0f;
        }

        // fix the 4th component depending on the whether we shift the origin of the cv
        if ( !shift_origin )
        {
            for ( cnt = 0; cnt < grip_count; cnt++ )
            {
                grip_points[cnt][kW] = 0.0f;
            }
        }
    }

    // transform the vertices to calculate the grip_vecs[]
    // we only need one vertex
	Utilities::transform(pmount_inst.matrix,grip_points, grip_nupoints, 1);

    // add in the "origin" of the grip, if necessary
    if ( NULL != grip_cv_ptr )
    {
		oct_bb_t::translate(tmp_cv, Vector3f(grip_nupoints[0][kX], grip_nupoints[0][kY], grip_nupoints[0][kZ]), *grip_cv_ptr);
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool set_weapongrip( const CHR_REF iitem, const CHR_REF iholder, Uint16 vrt_off )
{
    int i;

    bool needs_update;
    Uint16 grip_verts[GRIP_VERTS];

    matrix_cache_t * mcache;
    Object * pitem;

    needs_update = false;

    if ( !_currentModule->getObjectHandler().exists( iitem ) ) return false;
    pitem = _currentModule->getObjectHandler().get( iitem );
    mcache = &( pitem->inst.matrix_cache );

    // is the item attached to this valid holder?
    if ( pitem->attachedto != iholder ) return false;

    needs_update  = true;

    if ( GRIP_VERTS == get_grip_verts( grip_verts, iholder, vrt_off ) )
    {
        //---- detect any changes in the matrix_cache data

        needs_update  = false;

        if ( iholder != mcache->grip_chr || pitem->attachedto != iholder )
        {
            needs_update  = true;
        }

        if ( pitem->inwhich_slot != mcache->grip_slot )
        {
            needs_update  = true;
        }

        // check to see if any of the
        for ( i = 0; i < GRIP_VERTS; i++ )
        {
            if ( grip_verts[i] != mcache->grip_verts[i] )
            {
                needs_update = true;
                break;
            }
        }
    }

    if ( needs_update )
    {
        // cannot create the matrix, therefore the current matrix must be invalid
        mcache->matrix_valid = false;

        mcache->grip_chr  = iholder;
        mcache->grip_slot = pitem->inwhich_slot;

        for ( i = 0; i < GRIP_VERTS; i++ )
        {
            mcache->grip_verts[i] = grip_verts[i];
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix( const CHR_REF ichr )
{
    /// @author ZZ
    /// @details This function sets one character's matrix

    Object * pchr;
    chr_instance_t * pinst;

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return;
    pchr = _currentModule->getObjectHandler().get( ichr );
    pinst = &( pchr->inst );

    // invalidate this matrix
    pinst->matrix_cache.matrix_valid = false;

    if ( pchr->is_overlay )
    {
        // This character is an overlay and its ai.target points to the object it is overlaying
        // Overlays are kept with their target...
        if ( _currentModule->getObjectHandler().exists( pchr->ai.target ) )
        {
            Object * ptarget = _currentModule->getObjectHandler().get( pchr->ai.target );

            pchr->setPosition(ptarget->getPosition());

            // copy the matrix
            pinst->matrix = ptarget->inst.matrix;

            // copy the matrix data
            pinst->matrix_cache = ptarget->inst.matrix_cache;
        }
    }
    else
    {
        if ( pchr->getProfile()->hasStickyButt() )
        {
            mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(
                pinst->matrix,
				Vector3f(pchr->fat, pchr->fat, pchr->fat),
                TO_TURN( pchr->ori.facing_z ),
                TO_TURN( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET ),
                TO_TURN( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET ),
                pchr->getPosition());
        }
        else
        {
            mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(
                pinst->matrix,
				Vector3f(pchr->fat, pchr->fat, pchr->fat),
                TO_TURN( pchr->ori.facing_z ),
                TO_TURN( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET ),
                TO_TURN( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET ),
                pchr->getPosition());
        }

        pinst->matrix_cache.valid        = true;
        pinst->matrix_cache.matrix_valid = true;
        pinst->matrix_cache.type_bits    = MAT_CHARACTER;

        pinst->matrix_cache.self_scale[kX] = pchr->fat;
        pinst->matrix_cache.self_scale[kY] = pchr->fat;
        pinst->matrix_cache.self_scale[kZ] = pchr->fat;

        pinst->matrix_cache.rotate[kX] = CLIP_TO_16BITS( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET );
        pinst->matrix_cache.rotate[kY] = CLIP_TO_16BITS( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET );
        pinst->matrix_cache.rotate[kZ] = pchr->ori.facing_z;

        pinst->matrix_cache.pos = pchr->getPosition();
    }
}

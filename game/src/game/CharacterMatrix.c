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

static int get_grip_verts( Uint16 grip_verts[], const ObjectRef imount, int vrt_offset );

static egolib_rv matrix_cache_needs_update( Object * pchr, matrix_cache_t& pmc );
static bool apply_matrix_cache( Object * pchr, matrix_cache_t& mc_tmp );
static bool chr_get_matrix_cache( Object * pchr, matrix_cache_t& mc_tmp );

static bool apply_one_character_matrix( Object * pchr, matrix_cache_t& mcache );
static bool apply_one_weapon_matrix( Object * pweap, matrix_cache_t& mcache );

static int convert_grip_to_local_points( Object * pholder, Uint16 grip_verts[], Vector4f   dst_point[] );
static int convert_grip_to_global_points( const ObjectRef iholder, Uint16 grip_verts[], Vector4f   dst_point[] );

 bool matrix_cache_t::operator==(const matrix_cache_t &rhs) const {

    // handle problems with pointers
    if (this == &rhs) {
        return true;
    }

    // handle one of both if the matrix caches being invalid
    if (!this->valid || !rhs.valid) {
        return false;
    }

    // handle differences in the type
    int itmp = this->type_bits - rhs.type_bits;
    if (0 != itmp) return false;

    //---- check for differences in the MAT_WEAPON data
    if (HAS_SOME_BITS(this->type_bits, MAT_WEAPON)) {
        itmp = (signed)REF_TO_INT(this->grip_chr.get()) - (signed)REF_TO_INT(rhs.grip_chr.get());
        if (0 != itmp) return false;

        itmp = (signed)this->grip_slot - (signed)rhs.grip_slot;
        if (0 != itmp) return false;

        for (int cnt = 0; cnt < GRIP_VERTS; cnt++) {
            itmp = (signed)this->grip_verts[cnt] - (signed)rhs.grip_verts[cnt];
            if (0 != itmp) return false;
        }

        // handle differences in the scale of our mount
        for (int cnt = 0; cnt < 3; cnt++) {
            float ftmp = this->grip_scale[cnt] - rhs.grip_scale[cnt];
            if (0.0f != ftmp) { itmp = sgn(ftmp); return false; }
        }
    }

    //---- check for differences in the MAT_CHARACTER data
    if (HAS_SOME_BITS(this->type_bits, MAT_CHARACTER)) {
        // handle differences in the "Euler" rotation angles in 16-bit form
        for (int cnt = 0; cnt < 3; cnt++) {
            Facing ftmp = this->rotate[cnt] - rhs.rotate[cnt];
            if (Facing(0) != ftmp) { itmp = sgn(ftmp); return false; }
        }

        // handle differences in the translate vector
        for (int cnt = 0; cnt < 3; cnt++) {
            float ftmp = this->pos[cnt] - rhs.pos[cnt];
            if (0.0f != ftmp) { itmp = sgn(ftmp); return false; }
        }
    }

    //---- check for differences in the shared data
    if (HAS_SOME_BITS(this->type_bits, MAT_WEAPON) || HAS_SOME_BITS(this->type_bits, MAT_CHARACTER)) {
        // handle differences in our own scale
        for (int cnt = 0; cnt < 3; cnt++) {
            float ftmp = this->self_scale[cnt] - rhs.self_scale[cnt];
            if (0.0f != ftmp) { itmp = sgn(ftmp); return false; }
        }
    }

    // if it got here, the data is all the same
    return true;
}


/// @brief Determine wether the object has a valid matrix cache.
/// @param pobj a pointer to the object
/// @return @a false if a @a pobj is a null pointer or if the object's matrix cache is invalid,
/// @a true otherwise
bool chr_matrix_valid(const Object *pobj) {
    /// @author BB
    /// @details Determine whether the character has a valid matrix

    if (!pobj) return false;
    return pobj->inst.matrix_cache.isValid();
}


int get_grip_verts( Uint16 grip_verts[], const ObjectRef imount, int vrt_offset )
{
    /// @author BB
    /// @details Fill the grip_verts[] array from the mount's data.
    ///     Return the number of vertices found.

    if ( NULL == grip_verts ) return 0;

    // set all the vertices to a "safe" value
    for (size_t i = 0; i < GRIP_VERTS; i++ )
    {
        grip_verts[i] = 0xFFFF;
    }

    if ( !_currentModule->getObjectHandler().exists( imount ) ) return 0;
    Object *pmount = _currentModule->getObjectHandler().get( imount );

    if ( 0 == pmount->inst.getVertexCount() ) return 0;

    //---- set the proper weapongrip vertices
    int tnc = ( int )pmount->inst.getVertexCount() - ( int )vrt_offset;

    // if the starting vertex is less than 0, just take the first vertex
    if ( tnc < 0 )
    {
        grip_verts[0] = 0;
        return 1;
    }

    int vrt_count = 0;
    for (size_t i = 0; i < GRIP_VERTS; i++ )
    {
        if ( tnc + i < pmount->inst.getVertexCount() )
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
bool chr_get_matrix_cache( Object * pchr, matrix_cache_t& mc_tmp )
{
    /// @author BB
    /// @details grab the matrix cache data for a given character and put it into mc_tmp.
    if ( nullptr == pchr ) return false;
    auto ichr = GET_INDEX_PCHR( pchr );

    bool handled = false;
    auto itarget = ObjectRef::Invalid;

    // initialize xome parameters in case we fail
    mc_tmp.valid     = false;
    mc_tmp.type_bits = MAT_UNKNOWN;

    mc_tmp.self_scale = Vector3f(pchr->fat, pchr->fat, pchr->fat);

    // handle the overlay first of all
    if ( !handled && pchr->is_overlay && ichr != pchr->ai.getTarget() && _currentModule->getObjectHandler().exists( pchr->ai.getTarget() ) )
    {
        // this will pretty much fail the cmp_matrix_cache() every time...

        Object * ptarget = _currentModule->getObjectHandler().get( pchr->ai.getTarget() );

        // make sure we have the latst info from the target
        chr_update_matrix( ptarget, true );

        // grab the matrix cache into from the character we are overlaying
        mc_tmp = ptarget->inst.matrix_cache;

        // just in case the overlay's matrix cannot be corrected
        // then treat it as if it is not an overlay
        handled = mc_tmp.valid;
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
                mc_tmp.valid     = true;
                SET_BIT( mc_tmp.type_bits, MAT_WEAPON );        // add in the weapon data

                mc_tmp.grip_chr  = pchr->attachedto;
                mc_tmp.grip_slot = pchr->inwhich_slot;
                get_grip_verts( mc_tmp.grip_verts.data(), pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );

                itarget = pchr->attachedto;
            }
        }

        //---- update the MAT_CHARACTER data
        if ( _currentModule->getObjectHandler().exists( itarget ) )
        {
            Object * ptarget = _currentModule->getObjectHandler().get( itarget );

            mc_tmp.valid   = true;
            SET_BIT( mc_tmp.type_bits, MAT_CHARACTER );  // add in the MAT_CHARACTER-type data for the object we are "connected to"

            mc_tmp.rotate[kX] = ptarget->ori.map_twist_facing_x - orientation_t::MAP_TURN_OFFSET;
            mc_tmp.rotate[kY] = ptarget->ori.map_twist_facing_y - orientation_t::MAP_TURN_OFFSET;
            mc_tmp.rotate[kZ] = ptarget->ori.facing_z;

            mc_tmp.pos = ptarget->getPosition();

            mc_tmp.grip_scale = Vector3f(ptarget->fat, ptarget->fat, ptarget->fat);
        }
    }

    return mc_tmp.valid;
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
        pholder->inst.updateGripVertices(grip_verts, GRIP_VERTS );

        // copy the vertices into dst_point[]
        for ( point_count = 0, cnt = 0; cnt < GRIP_VERTS; cnt++, point_count++ )
        {
            uint16_t vertex = grip_verts[cnt];

            if ( 0xFFFF == vertex ) continue;

            dst_point[point_count][kX] = pholder->inst.getVertex(vertex).pos[XX];
            dst_point[point_count][kY] = pholder->inst.getVertex(vertex).pos[YY];
            dst_point[point_count][kZ] = pholder->inst.getVertex(vertex).pos[ZZ];
            dst_point[point_count][kW] = 1.0f;
        }
    }

    return point_count;
}

//--------------------------------------------------------------------------------------------
int convert_grip_to_global_points( const ObjectRef iholder, Uint16 grip_verts[], Vector4f   dst_point[] )
{
    /// @author ZZ
    /// @details a helper function for apply_one_weapon_matrix()

	Vector4f  src_point[GRIP_VERTS];

    if ( !_currentModule->getObjectHandler().exists( iholder ) ) return 0;
    Object *pholder = _currentModule->getObjectHandler().get( iholder );

    // find the grip points in the character's "local" or "body-fixed" coordinates
    int point_count = convert_grip_to_local_points( pholder, grip_verts, src_point );

    if ( 0 == point_count ) return 0;

    // use the math function instead of rolling out own
	Utilities::transform(pholder->inst.getMatrix(), src_point, dst_point, point_count);

    return point_count;
}

//--------------------------------------------------------------------------------------------
bool apply_one_weapon_matrix( Object * pweap, matrix_cache_t& mc_tmp )
{
    /// @author ZZ
    /// @details Request that the data in the matrix cache be used to create a "character matrix".
    ///               i.e. a matrix that is not being held by anything.

	Vector4f  nupoint[GRIP_VERTS];
    int       iweap_points;

    if ( !mc_tmp.valid || 0 == ( MAT_WEAPON & mc_tmp.type_bits ) ) return false;

    if ( nullptr == pweap ) return false;
    matrix_cache_t& pweap_mcache = pweap->inst.matrix_cache;

    if ( !_currentModule->getObjectHandler().exists( mc_tmp.grip_chr ) ) return false;

    // make sure that the matrix is invalid incase of an error
    pweap_mcache.matrix_valid = false;

    // grab the grip points in world coordinates
    iweap_points = convert_grip_to_global_points( mc_tmp.grip_chr, mc_tmp.grip_verts.data(), nupoint );

    if ( 4 == iweap_points )
    {
        // Calculate weapon's matrix based on positions of grip points
        // chrscale is recomputed at time of attachment
        pweap->inst.setMatrix(Utilities::fromFourPoints(Vector3f(nupoint[0][kX], nupoint[0][kY], nupoint[0][kZ]),
                                                        Vector3f(nupoint[1][kX], nupoint[1][kY], nupoint[1][kZ]),
                                                        Vector3f(nupoint[2][kX], nupoint[2][kY], nupoint[2][kZ]),
                                                        Vector3f(nupoint[3][kX], nupoint[3][kY], nupoint[3][kZ]), 
                                                        mc_tmp.self_scale[kZ]));

        // update the weapon position
        pweap->setPosition(Vector3f(nupoint[3][kX],nupoint[3][kY],nupoint[3][kZ]));
        pweap->inst.matrix_cache = mc_tmp;

        pweap_mcache.matrix_valid = true;
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
        SET_BIT( mc_tmp.type_bits, MAT_CHARACTER );

        // treat it like a normal character matrix
        apply_one_character_matrix( pweap, mc_tmp );
    }

    return pweap_mcache.matrix_valid;
}

//--------------------------------------------------------------------------------------------
bool apply_one_character_matrix( Object * pchr, matrix_cache_t& mc_tmp )
{
    /// @author ZZ
    /// @details Request that the matrix cache data be used to create a "weapon matrix".
    ///               i.e. a matrix that is attached to a specific grip.

    // only apply character matrices using this function
    if ( 0 == ( MAT_CHARACTER & mc_tmp.type_bits ) ) return false;

    if ( nullptr == pchr ) return false;

    pchr->inst.matrix_cache.matrix_valid = false;

    if ( pchr->getProfile()->hasStickyButt() )
    {
        pchr->inst.setMatrix(
        mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(
            mc_tmp.self_scale,
            mc_tmp.rotate[kZ],
            mc_tmp.rotate[kX],
            mc_tmp.rotate[kY],
            mc_tmp.pos));
    }
    else
    {
        pchr->inst.setMatrix(
        mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(
            mc_tmp.self_scale,
            mc_tmp.rotate[kZ],
            mc_tmp.rotate[kX],
            mc_tmp.rotate[kY],
            mc_tmp.pos));
    }

    pchr->inst.matrix_cache = mc_tmp;
    pchr->inst.matrix_cache.matrix_valid = true;

    return true;
}

//--------------------------------------------------------------------------------------------
bool apply_matrix_cache( Object * pchr, matrix_cache_t& mc_tmp )
{
    /// @author BB
    /// @details request that the info in the matrix cache mc_tmp, be used to
    ///               make a matrix for the character pchr.

    bool applied = false;

    if ( nullptr == pchr ) return false;
    if ( !mc_tmp.valid ) return false;

    if ( 0 != ( MAT_WEAPON & mc_tmp.type_bits ) )
    {
        if ( _currentModule->getObjectHandler().exists( mc_tmp.grip_chr ) )
        {
            applied = apply_one_weapon_matrix( pchr, mc_tmp );
        }
        else
        {
            matrix_cache_t& mcache = pchr->inst.matrix_cache;

            // !!!the mc_tmp was mis-labeled as a MAT_WEAPON!!!
            make_one_character_matrix(pchr->getObjRef());

            // recover the matrix_cache values from the character
            SET_BIT( mcache.type_bits, MAT_CHARACTER );
            if ( mcache.matrix_valid )
            {
                mcache.valid     = true;
                mcache.type_bits = MAT_CHARACTER;

                mcache.self_scale = Vector3f(pchr->fat, pchr->fat, pchr->fat);

                mcache.grip_scale = mcache.self_scale;

                mcache.rotate[kX] = pchr->ori.map_twist_facing_x - orientation_t::MAP_TURN_OFFSET;
                mcache.rotate[kY] = pchr->ori.map_twist_facing_y - orientation_t::MAP_TURN_OFFSET;
                mcache.rotate[kZ] = pchr->ori.facing_z;

                mcache.pos = pchr->getPosition();

                applied = true;
            }
        }
    }
    else if ( 0 != ( MAT_CHARACTER & mc_tmp.type_bits ) )
    {
        applied = apply_one_character_matrix( pchr, mc_tmp );
    }

    return applied;
}

//--------------------------------------------------------------------------------------------
egolib_rv matrix_cache_needs_update( Object * pchr, matrix_cache_t& pmc )
{
    /// @author BB
    /// @details determine whether a matrix cache has become invalid and needs to be updated

    if ( nullptr == pchr ) return rv_error;

    // get the matrix data that is supposed to be used to make the matrix
    chr_get_matrix_cache( pchr, pmc );

    // compare that data to the actual data used to make the matrix
    return !(pmc == pchr->inst.matrix_cache) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_update_matrix( Object * pchr, bool update_size )
{
    /// @author BB
    /// @details Do everything necessary to set the current matrix for this character.
    ///     This might include recursively going down the list of this character's mounts, etc.
    ///
    ///     Return true if a new matrix is applied to the character, false otherwise.

    bool         needs_update = false;

    // recursively make sure that any mount matrices are updated
    const std::shared_ptr<Object> &holder = pchr->getHolder();

    if (holder)
    {
        egolib_rv attached_update = chr_update_matrix(holder.get(), true);

        // if this fails, we should probably do something...
        if ( rv_error == attached_update )
        {
            // there is an error so this matrix is not defined and no readon to go farther
            pchr->inst.matrix_cache.matrix_valid = false;
            return attached_update;
        }
        else if ( rv_success == attached_update )
        {
            // the holder/mount matrix has changed.
            // this matrix is no longer valid.
            pchr->inst.matrix_cache.matrix_valid = false;
        }
    }

    // does the matrix cache need an update at all?
    matrix_cache_t mc_tmp;
    egolib_rv retval = matrix_cache_needs_update( pchr, mc_tmp );
    if ( rv_error == retval ) return rv_error;
    needs_update = ( rv_success == retval );

    // Update the grip vertices no matter what (if they are used)
    const std::shared_ptr<Object> &heldItem = _currentModule->getObjectHandler()[mc_tmp.grip_chr];
    if ( HAS_SOME_BITS(mc_tmp.type_bits, MAT_WEAPON) && heldItem)
    {
        // has that character changes its animation?
        if(heldItem->inst.updateGripVertices(mc_tmp.grip_verts.data(), GRIP_VERTS)) {
            needs_update = true;
        }
    }

    // if it is not the same, make a new matrix with the new data
    if (needs_update)
    {
        // we know the matrix is not valid
        pchr->inst.matrix_cache.matrix_valid = false;

        if(apply_matrix_cache(pchr, mc_tmp)) {
            if(update_size) {
                // call chr_update_collision_size() but pass in a false value to prevent a recursize call
                pchr->getObjectPhysics().updateCollisionSize(false);
            }
            return rv_success;
        }

    }

    return rv_fail;
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
        up = mat_getChrUp(pchr->inst.getMatrix());
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
        right = mat_getChrRight(pchr->inst.getMatrix());
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
        forward = mat_getChrForward(pchr->inst.getMatrix());
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
        translate = mat_getTranslate(pchr->inst.getMatrix());
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
		int vert_stt = static_cast<int>(pmount->inst.getVertexCount()) - static_cast<int>(grip_offset);
        if ( vert_stt < 0 ) return false;

		if (gfx_error == pmount->inst.updateVertices(vert_stt, vert_stt + grip_offset, false))
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
			for (grip_count = 0, cnt = 0; cnt < GRIP_VERTS && (size_t)(vert_stt + cnt) < pmount->inst.getVertexCount(); grip_count++, cnt++)
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
	Utilities::transform(pmount->inst.getMatrix(),grip_points, grip_nupoints, 1);

    // add in the "origin" of the grip, if necessary
    if ( NULL != grip_cv_ptr )
    {
        *grip_cv_ptr = oct_bb_t::translate(tmp_cv, Vector3f(grip_nupoints[0][kX], grip_nupoints[0][kY], grip_nupoints[0][kZ]));
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool set_weapongrip( const ObjectRef iitem, const ObjectRef iholder, Uint16 vrt_off )
{
    uint16_t grip_verts[GRIP_VERTS];

    if ( !_currentModule->getObjectHandler().exists( iitem ) ) return false;
	Object *pitem = _currentModule->getObjectHandler().get( iitem );
	matrix_cache_t& mcache = pitem->inst.matrix_cache;

    // is the item attached to this valid holder?
    if ( pitem->attachedto != iholder ) return false;

    bool needs_update  = true;

    if ( GRIP_VERTS == get_grip_verts( grip_verts, iholder, vrt_off ) )
    {
        //---- detect any changes in the matrix_cache data

        needs_update  = false;

        if ( iholder != mcache.grip_chr || pitem->attachedto != iholder )
        {
            needs_update  = true;
        }

        if ( pitem->inwhich_slot != mcache.grip_slot )
        {
            needs_update  = true;
        }

        // check to see if any of the
        for (size_t i = 0; i < GRIP_VERTS; i++ )
        {
            if ( grip_verts[i] != mcache.grip_verts[i] )
            {
                needs_update = true;
                break;
            }
        }
    }

    if ( needs_update )
    {
        // cannot create the matrix, therefore the current matrix must be invalid
        mcache.matrix_valid = false;

        mcache.grip_chr  = iholder;
        mcache.grip_slot = pitem->inwhich_slot;

        for (size_t i = 0; i < GRIP_VERTS; i++ )
        {
            mcache.grip_verts[i] = grip_verts[i];
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix( const ObjectRef ichr )
{
    /// @author ZZ
    /// @details This function sets one character's matrix

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return;
    Object * pchr = _currentModule->getObjectHandler().get( ichr );

    // invalidate this matrix
    pchr->inst.matrix_cache.matrix_valid = false;

    if ( pchr->is_overlay )
    {
        // This character is an overlay and its ai.target points to the object it is overlaying
        // Overlays are kept with their target...
        if ( _currentModule->getObjectHandler().exists( pchr->ai.getTarget() ) )
        {
            Object * ptarget = _currentModule->getObjectHandler().get( pchr->ai.getTarget() );

            pchr->setPosition(ptarget->getPosition());

            // copy the matrix
            pchr->inst.setMatrix(ptarget->inst.getMatrix());

            // copy the matrix data
            pchr->inst.matrix_cache = ptarget->inst.matrix_cache;
        }
    }
    else
    {
        if ( pchr->getProfile()->hasStickyButt() )
        {
            pchr->inst.setMatrix(
                mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(
    				Vector3f(pchr->fat, pchr->fat, pchr->fat),
                    pchr->ori.facing_z,
                    pchr->ori.map_twist_facing_x - orientation_t::MAP_TURN_OFFSET,
                    pchr->ori.map_twist_facing_y - orientation_t::MAP_TURN_OFFSET,
                    pchr->getPosition())
            );
        }
        else
        {
            pchr->inst.setMatrix(
                mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(
    				Vector3f(pchr->fat, pchr->fat, pchr->fat),
                    pchr->ori.facing_z,
                    pchr->ori.map_twist_facing_x - orientation_t::MAP_TURN_OFFSET,
                    pchr->ori.map_twist_facing_y - orientation_t::MAP_TURN_OFFSET,
                    pchr->getPosition())
            );
        }

        pchr->inst.matrix_cache.valid        = true;
        pchr->inst.matrix_cache.matrix_valid = true;
        pchr->inst.matrix_cache.type_bits    = MAT_CHARACTER;

        pchr->inst.matrix_cache.self_scale = Vector3f(pchr->fat, pchr->fat, pchr->fat);

        pchr->inst.matrix_cache.rotate[kX] = pchr->ori.map_twist_facing_x - orientation_t::MAP_TURN_OFFSET;
        pchr->inst.matrix_cache.rotate[kY] = pchr->ori.map_twist_facing_y - orientation_t::MAP_TURN_OFFSET;
        pchr->inst.matrix_cache.rotate[kZ] = pchr->ori.facing_z;

        pchr->inst.matrix_cache.pos = pchr->getPosition();
    }
}

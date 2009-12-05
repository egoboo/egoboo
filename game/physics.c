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

/// @file physics.c

#include "physics.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float hillslide       =  1.00f;
float slippyfriction  =  1.00f; 
float airfriction     =  0.91f;    
float waterfriction   =  0.80f;
float noslipfriction  =  0.91f;
float gravity         = -1.00f;        

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int oct_bb_to_points( oct_bb_t * pbmp, fvec4_t   pos[], size_t pos_count )
{
    /// @details BB@> convert the corners of the level 1 bounding box to a point cloud
    ///      set pos[].w to zero for now, that the transform does not
    ///      shift the points while transforming them
    ///
    /// @note Make sure to set pos[].w to zero so that the bounding box will not be translated
    ///      then the transformation matrix is applied.
    ///
    /// @note The math for finding the corners of this bumper is not hard, but it is easy to make a mistake.
    ///      be careful if you modify anything.

    float ftmp;
    float val_x, val_y;

    int vcount = 0;

    if ( NULL == pbmp || NULL == pos || 0 == pos_count ) return 0;

    //---- the points along the y_max edge
    ftmp = 0.5f * ( pbmp->maxs[OCT_XY] + pbmp->maxs[OCT_YX] );  // the top point of the diamond
    if ( ftmp <= pbmp->maxs[OCT_Y] )
    {
        val_x = 0.5f * ( pbmp->maxs[OCT_XY] - pbmp->maxs[OCT_YX] );
        val_y = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_y = pbmp->maxs[OCT_Y];

        val_x = pbmp->maxs[OCT_Y] - pbmp->maxs[OCT_YX];
        if ( val_x < pbmp->mins[OCT_X] )
        {
            val_x = pbmp->mins[OCT_X];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_x = pbmp->maxs[OCT_XY] - pbmp->maxs[OCT_Y];
        if ( val_x > pbmp->maxs[OCT_X] )
        {
            val_x = pbmp->maxs[OCT_X];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    //---- the points along the y_min edge
    ftmp = 0.5f * ( pbmp->mins[OCT_XY] + pbmp->mins[OCT_YX] );  // the top point of the diamond
    if ( ftmp >= pbmp->mins[OCT_Y] )
    {
        val_x = 0.5f * ( pbmp->mins[OCT_XY] - pbmp->mins[OCT_YX] );
        val_y = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_y = pbmp->mins[OCT_Y];

        val_x = pbmp->mins[OCT_XY] - pbmp->mins[OCT_Y];
        if ( val_x < pbmp->mins[OCT_X] )
        {
            val_x = pbmp->mins[OCT_X];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_x = pbmp->mins[OCT_Y] - pbmp->mins[OCT_YX];
        if ( val_x > pbmp->maxs[OCT_X] )
        {
            val_x = pbmp->maxs[OCT_X];
        }
        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    //---- the points along the x_max edge
    ftmp = 0.5f * ( pbmp->maxs[OCT_XY] - pbmp->mins[OCT_YX] );  // the top point of the diamond
    if ( ftmp <= pbmp->maxs[OCT_X] )
    {
        val_y = 0.5f * ( pbmp->maxs[OCT_XY] + pbmp->mins[OCT_YX] );
        val_x = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_x = pbmp->maxs[OCT_X];

        val_y = pbmp->maxs[OCT_X] + pbmp->mins[OCT_YX];
        if ( val_y < pbmp->mins[OCT_Y] )
        {
            val_y = pbmp->mins[OCT_Y];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_y = pbmp->maxs[OCT_XY] - pbmp->maxs[OCT_X];
        if ( val_y > pbmp->maxs[OCT_Y] )
        {
            val_y = pbmp->maxs[OCT_Y];
        }
        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    //---- the points along the x_min edge
    ftmp = 0.5f * ( pbmp->mins[OCT_XY] - pbmp->maxs[OCT_YX] );  // the left point of the diamond
    if ( ftmp >= pbmp->mins[OCT_X] )
    {
        val_y = 0.5f * ( pbmp->mins[OCT_XY] + pbmp->maxs[OCT_YX] );
        val_x = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_x = pbmp->mins[OCT_X];

        val_y = pbmp->mins[OCT_XY] - pbmp->mins[OCT_X];
        if ( val_y < pbmp->mins[OCT_Y] )
        {
            val_y = pbmp->mins[OCT_Y];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_y = pbmp->maxs[OCT_YX] + pbmp->mins[OCT_X];
        if ( val_y > pbmp->maxs[OCT_Y] )
        {
            val_y = pbmp->maxs[OCT_Y];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    return vcount;
}

//--------------------------------------------------------------------------------------------
void points_to_oct_bb( oct_bb_t * pbmp, fvec4_t   pos[], size_t pos_count )
{
    /// @details BB@> convert the new point cloud into a level 1 bounding box using a fvec4_t
    ///     array as the source

    Uint32 cnt;

    if ( NULL == pbmp || NULL == pos || 0 == pos_count ) return;

    // determine a bounding box for the point cloud
    pbmp->mins[OCT_X]  = pbmp->maxs[OCT_X]  = pos[0].x;
    pbmp->mins[OCT_Y]  = pbmp->maxs[OCT_Y]  = pos[0].y;
    pbmp->mins[OCT_Z]  = pbmp->maxs[OCT_Z]  = pos[0].z;
    pbmp->mins[OCT_XY] = pbmp->maxs[OCT_XY] = pbmp->mins[OCT_X] + pbmp->mins[OCT_Y];
    pbmp->mins[OCT_YX] = pbmp->maxs[OCT_YX] = -pbmp->mins[OCT_X] + pbmp->mins[OCT_Y];

    for ( cnt = 1; cnt < pos_count; cnt++ )
    {
        float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;

        tmp_x = pos[cnt].x;
        pbmp->mins[OCT_X]  = MIN( pbmp->mins[OCT_X], tmp_x );
        pbmp->maxs[OCT_X]  = MAX( pbmp->maxs[OCT_X], tmp_x );

        tmp_y = pos[cnt].y;
        pbmp->mins[OCT_Y]  = MIN( pbmp->mins[OCT_Y], tmp_y );
        pbmp->maxs[OCT_Y]  = MAX( pbmp->maxs[OCT_Y], tmp_y );

        tmp_z = pos[cnt].z;
        pbmp->mins[OCT_Z]  = MIN( pbmp->mins[OCT_Z], tmp_z );
        pbmp->maxs[OCT_Z]  = MAX( pbmp->maxs[OCT_Z], tmp_z );

        tmp_xy = tmp_x + tmp_y;
        pbmp->mins[OCT_XY] = MIN( pbmp->mins[OCT_XY], tmp_xy );
        pbmp->maxs[OCT_XY] = MAX( pbmp->maxs[OCT_XY], tmp_xy );

        tmp_yx = -tmp_x + tmp_y;
        pbmp->mins[OCT_YX] = MIN( pbmp->mins[OCT_YX], tmp_yx );
        pbmp->maxs[OCT_YX] = MAX( pbmp->maxs[OCT_YX], tmp_yx );
    }
}

//--------------------------------------------------------------------------------------------
bool_t vec_to_oct_vec( fvec3_t pos, oct_vec_t ovec )
{
    if( NULL == ovec ) return bfalse;

    ovec[OCT_X ] =  pos.x;
    ovec[OCT_Y ] =  pos.y;
    ovec[OCT_Z ] =  pos.z;
    ovec[OCT_XY] =  pos.x + pos.y;
    ovec[OCT_YX] = -pos.x + pos.y;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t bumper_to_oct_bb( bumper_t src, oct_bb_t * pdst )
{
    if( NULL == pdst ) return bfalse;

    pdst->mins[OCT_X] = -src.size;
    pdst->maxs[OCT_X] =  src.size;

    pdst->mins[OCT_Y] = -src.size;
    pdst->maxs[OCT_Y] =  src.size;

    pdst->mins[OCT_XY] = -src.sizebig;
    pdst->maxs[OCT_XY] =  src.sizebig;

    pdst->mins[OCT_YX] = -src.sizebig;
    pdst->maxs[OCT_YX] =  src.sizebig;

    pdst->mins[OCT_Z] = -src.height;
    pdst->maxs[OCT_Z] =  src.height;

    return btrue;
};

//--------------------------------------------------------------------------------------------
void oct_bb_downgrade( oct_bb_t * psrc_bb, bumper_t bump_base, bumper_t * pdst_bump, oct_bb_t * pdst_bb )
{
    /// @details BB@> convert a level 1 bumper to an "equivalent" level 0 bumper

    float val1, val2, val3, val4;

    // return if there is no source
    if ( NULL == psrc_bb ) return;

    //---- handle all of the pdst_bump data first
    if ( NULL != pdst_bump )
    {
        if ( 0 == bump_base.height )
        {
            pdst_bump->height = 0.0f;
        }
        else
        {
            // have to use MAX here because the height can be distorted due
            // to make object-particle interactions easier (i.e. it allows you to
            // hit a grub bug with your hands)

            pdst_bump->height = MAX( bump_base.height, psrc_bb->maxs[OCT_Z] );
        }

        if ( 0 == bump_base.size )
        {
            pdst_bump->size = 0.0f;
        }
        else
        {
            val1 = ABS( psrc_bb->mins[OCT_X] );
            val2 = ABS( psrc_bb->maxs[OCT_Y] );
            val3 = ABS( psrc_bb->mins[OCT_Y] );
            val4 = ABS( psrc_bb->maxs[OCT_Y] );
            pdst_bump->size = MAX( MAX( val1, val2 ), MAX( val3, val4 ) );
        }

        if ( 0 == bump_base.sizebig )
        {
            pdst_bump->sizebig = 0;
        }
        else
        {
            val1 =  psrc_bb->maxs[OCT_YX];
            val2 = -psrc_bb->mins[OCT_YX];
            val3 =  psrc_bb->maxs[OCT_XY];
            val4 = -psrc_bb->mins[OCT_XY];
            pdst_bump->sizebig = MAX( MAX( val1, val2 ), MAX( val3, val4 ) );
        }
    }

    //---- handle all of the pdst_bb data second
    if ( NULL != pdst_bb )
    {
        // memcpy() can fail horribly if the domains overlap
        // I don't think this should ever happen, though
        if ( pdst_bb != psrc_bb )
        {
            memcpy( pdst_bb, psrc_bb, sizeof(*pdst_bb) );
        }

        if ( 0 == bump_base.height )
        {
            pdst_bb->mins[OCT_Z] = pdst_bb->maxs[OCT_Z] = 0.0f;
        }
        else
        {
            // handle the vertical distortion the same as above
            pdst_bb->maxs[OCT_Z] = MAX( bump_base.height, psrc_bb->maxs[OCT_Z] );
        }

        // 0 == bump_base.size is supposed to be shorthand for "this object doesn't interact
        // with anything", so we have to set all of the horizontal pdst_bb data to zero to
        // make
        if ( 0 == bump_base.size )
        {
            pdst_bb->mins[OCT_X ] = pdst_bb->maxs[OCT_X ] = 0.0f;
            pdst_bb->mins[OCT_Y ] = pdst_bb->maxs[OCT_Y ] = 0.0f;
            pdst_bb->mins[OCT_XY] = pdst_bb->maxs[OCT_XY] = 0.0f;
            pdst_bb->mins[OCT_YX] = pdst_bb->maxs[OCT_YX] = 0.0f;
        }
    }
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t get_depth_close_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb(bump_a, &cv_a);
    bumper_to_oct_bb(bump_b, &cv_b);

    return get_depth_close_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool_t get_depth_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb(bump_a, &cv_a);
    bumper_to_oct_bb(bump_b, &cv_b);

    // convert the bumper to the correct format
    bumper_to_oct_bb(bump_b, &cv_b);

    return get_depth_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool_t get_depth_close_1( oct_bb_t cv_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb(bump_b, &cv_b);

    return get_depth_close_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool_t get_depth_1( oct_bb_t cv_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb(bump_b, &cv_b);

    return get_depth_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool_t get_depth_close_2( oct_bb_t cv_a, fvec3_t pos_a, oct_bb_t cv_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    oct_vec_t oa, ob;
    bool_t valid;

    if( NULL == depth ) return bfalse;

    // translate the positions to oct_vecs
    vec_to_oct_vec( pos_a, oa );
    vec_to_oct_vec( pos_b, ob );

    // calculate the depth
    valid = btrue;
    for( cnt=0; cnt<OCT_Z; cnt++ )
    {
        float ftmp1 = MIN(( ob[cnt] + cv_b.maxs[cnt] ) - oa[cnt], oa[cnt] - ( ob[cnt] + cv_b.mins[cnt] ) );
        float ftmp2 = MIN(( oa[cnt] + cv_a.maxs[cnt] ) - ob[cnt], ob[cnt] - ( oa[cnt] + cv_a.mins[cnt] ) );
        depth[cnt] = MAX( ftmp1, ftmp2 );

        if( depth[cnt] <= 0.0f )
        {
            valid = bfalse;
            if( break_out ) return bfalse;
        }
    }

    // treat the z coordinate the same as always
    depth[OCT_Z]  = MIN( cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z] ) -
                    MAX( cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z] );

    if( depth[OCT_Z] <= 0.0f )
    {
        valid = bfalse;
        if( break_out ) return bfalse;
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= INV_SQRT_TWO;
    depth[OCT_YX] *= INV_SQRT_TWO;

    return valid;
}

//--------------------------------------------------------------------------------------------
bool_t get_depth_2( oct_bb_t cv_a, fvec3_t pos_a, oct_bb_t cv_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    oct_vec_t oa, ob;
    bool_t valid;

    if( NULL == depth ) return bfalse;

    // translate the positions to oct_vecs
    vec_to_oct_vec( pos_a, oa );
    vec_to_oct_vec( pos_b, ob );

    // calculate the depth
    valid = btrue;
    for( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        depth[cnt]  = MIN( cv_b.maxs[cnt] + ob[cnt], cv_a.maxs[cnt] + oa[cnt] ) -
                      MAX( cv_b.mins[cnt] + ob[cnt], cv_a.mins[cnt] + oa[cnt] );

        if( depth[cnt] <= 0.0f )
        {
            valid = bfalse;
            if( break_out ) return bfalse;
        }
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= INV_SQRT_TWO;
    depth[OCT_YX] *= INV_SQRT_TWO;

    return valid;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t test_interaction_close_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb(bump_a, &cv_a);
    bumper_to_oct_bb(bump_b, &cv_b);

    return test_interaction_close_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool_t test_interaction_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb(bump_a, &cv_a);
    bumper_to_oct_bb(bump_b, &cv_b);

    return test_interaction_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool_t test_interaction_close_1( oct_bb_t cv_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb(bump_b, &cv_b);

    return test_interaction_close_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool_t test_interaction_1( oct_bb_t cv_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb(bump_b, &cv_b);

    return test_interaction_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool_t test_interaction_close_2( oct_bb_t cv_a, fvec3_t pos_a, oct_bb_t cv_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    float depth;
    oct_vec_t oa, ob;

    // translate the positions to oct_vecs
    vec_to_oct_vec( pos_a, oa );
    vec_to_oct_vec( pos_b, ob );

    // calculate the depth
    for( cnt=0; cnt<OCT_Z; cnt++ )
    {
        float ftmp1 = MIN(( ob[cnt] + cv_b.maxs[cnt] ) - oa[cnt], oa[cnt] - ( ob[cnt] + cv_b.mins[cnt] ) );
        float ftmp2 = MIN(( oa[cnt] + cv_a.maxs[cnt] ) - ob[cnt], ob[cnt] - ( oa[cnt] + cv_a.mins[cnt] ) );
        depth = MAX( ftmp1, ftmp2 );
        if( depth <= 0.0f ) return bfalse;
    }

    // treat the z coordinate the same as always
    depth = MIN( cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z] ) -
            MAX( cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z] );
    
    return test_platform ? (depth > -PLATTOLERANCE) : (depth > 0.0f);
}

//--------------------------------------------------------------------------------------------
bool_t test_interaction_2( oct_bb_t cv_a, fvec3_t pos_a, oct_bb_t cv_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    oct_vec_t oa, ob;
    float depth;

    // translate the positions to oct_vecs
    vec_to_oct_vec( pos_a, oa );
    vec_to_oct_vec( pos_b, ob );

    // calculate the depth
    for( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        depth  = MIN( cv_b.maxs[cnt] + ob[cnt], cv_a.maxs[cnt] + oa[cnt] ) -
                      MAX( cv_b.mins[cnt] + ob[cnt], cv_a.mins[cnt] + oa[cnt] );

        if( depth <= 0.0f ) return bfalse;
    }

    // treat the z coordinate the same as always
    depth = MIN( cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z] ) -
            MAX( cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z] );

    return test_platform ? (depth > -PLATTOLERANCE) : (depth > 0.0f);
}

//--------------------------------------------------------------------------------------------
bool_t phys_estimate_chr_chr_normal(oct_vec_t opos_a, oct_vec_t opos_b, oct_vec_t odepth, float exponent, fvec3_base_t nrm )
{
    bool_t retval;

    // is everything valid?
    if( NULL == opos_a || NULL == opos_b || NULL == odepth || NULL == nrm ) return bfalse;

    // initialize the vector
    nrm[kX] = nrm[kY] = nrm[kZ] = 0.0f;

    if ( odepth[OCT_X] <= 0.0f )
    {
        odepth[OCT_X] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_X] - opos_a[OCT_X];
        sgn = sgn > 0 ? -1 : 1;

        nrm[kX] += sgn / POW( odepth[OCT_X] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_Y] <= 0.0f )
    {
        odepth[OCT_Y] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_Y] - opos_a[OCT_Y];
        sgn = sgn > 0 ? -1 : 1;

        nrm[kY] += sgn / POW( odepth[OCT_Y] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_XY] <= 0.0f )
    {
        odepth[OCT_XY] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_XY] - opos_a[OCT_XY];
        sgn = sgn > 0 ? -1 : 1;

        nrm[kX] += sgn / POW( odepth[OCT_XY] / PLATTOLERANCE, exponent );
        nrm[kY] += sgn / POW( odepth[OCT_XY] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_YX] <= 0.0f )
    {
        odepth[OCT_YX] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_YX] - opos_a[OCT_YX];
        sgn = sgn > 0 ? -1 : 1;
        nrm[kX] -= sgn / POW( odepth[OCT_YX] / PLATTOLERANCE, exponent );
        nrm[kY] += sgn / POW( odepth[OCT_YX] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_Z] <= 0.0f )
    {
        odepth[OCT_Z] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_Z] - opos_a[OCT_Z];

        sgn = sgn > 0 ? -1 : 1;

        nrm[kZ] += sgn / POW( exponent * odepth[OCT_Z] / PLATTOLERANCE, exponent );
    }

    retval = bfalse;
    if ( ABS( nrm[kX] ) + ABS( nrm[kY] ) + ABS( nrm[kZ] ) > 0.0f )
    {
        fvec3_t vtmp = fvec3_normalize( nrm );
        memcpy( nrm, vtmp.v, sizeof(fvec3_base_t) );
        retval = btrue;
    }

    return retval;
}


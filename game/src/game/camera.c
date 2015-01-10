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

/// @file game/camera.c
/// @brief Various functions related to how the game camera works.
/// @details

#include "game/camera.h"

#include "game/input.h"
#include "game/graphic.h"
#include "game/network.h"
#include "game/player.h"
#include "game/egoboo.h"

#include "game/char.inl"
#include "game/mesh.inl"

//--------------------------------------------------------------------------------------------

const float camera_t::DEFAULT_FOV = 60.0f;

const float camera_t::DEFAULT_TURN_JOY = 64;

const float camera_t::DEFAULT_TURN_KEY = camera_t::DEFAULT_TURN_JOY;

const Uint8 camera_t::DEFAULT_TURN_TIME = 16;

//--------------------------------------------------------------------------------------------

static void camera_update_position( camera_t * pcam );
static void camera_update_center( camera_t * pcam );
static void camera_update_track( camera_t * pcam, const ego_mesh_t * pmesh, CHR_REF track_list[], const size_t track_list_size );
//static void camera_update_zoom( camera_t * pcam, float height );

static size_t camera_create_track_list( CHR_REF track_list[], const size_t track_list_max_size );

#if 0
static void dump_matrix( const fmat_4x4_base_t a );
#endif
static void camera_update_effects( camera_t * pcam );
static void camera_update_zoom( camera_t * pcam );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

camera_options_t cam_options;

//--------------------------------------------------------------------------------------------
// Camera control stuff
//--------------------------------------------------------------------------------------------

bool camera_reset_view( camera_t * pcam )
{
    if ( NULL == pcam ) return false;

    camera_gluLookAt( pcam, RAD_TO_DEG(pcam->roll) );

    return true;
}

//--------------------------------------------------------------------------------------------
bool camera_reset_projection( camera_t * pcam, float fov_deg, float ar )
{
    if ( NULL == pcam ) return false;

    camera_gluPerspective( pcam, fov_deg, ar, 1, 20 );

    return true;
}

//--------------------------------------------------------------------------------------------
camera_t *camera_ctor( camera_t * pcam )
{
    /// @author BB
    /// @details initialize the camera structure

    //BLANK_STRUCT_PTR( pcam )

    // global options
    pcam->turn_mode  = cam_options.turn_mode;
    pcam->swing      = cam_options.swing;
    pcam->swing_amp  = cam_options.swing_amp;
    pcam->swing_rate = cam_options.swing_rate;

    // constant values
	pcam->track_level     =  0;
	pcam->turn_time       =  camera_t::DEFAULT_TURN_TIME;
	pcam->move_mode_old   =  CAM_PLAYER;
    pcam->move_mode       =  CAM_PLAYER;
    pcam->zoom            =  CAM_ZOOM_AVG;
    pcam->zadd            =  CAM_ZADD_AVG;
    pcam->zadd_goto       =  CAM_ZADD_AVG;
    pcam->zgoto           =  CAM_ZADD_AVG;
    pcam->turn_z_rad      = -PI_OVER_FOUR;
    pcam->turn_z_add      =  0;
    pcam->turn_z_sustain  =  0.60f;
    pcam->roll            =  0.0f;
    pcam->motion_blur     =  0.0f;
    pcam->motion_blur_old =  0.0f;
    fvec3_self_clear( pcam->center.v );

    // derived values
    fvec3_base_copy( pcam->track_pos.v, pcam->center.v );
    fvec3_base_copy( pcam->pos.v,       pcam->center.v );

    pcam->pos.x += pcam->zoom * SIN( pcam->turn_z_rad );
    pcam->pos.y += pcam->zoom * COS( pcam->turn_z_rad );
    pcam->pos.z += CAM_ZADD_MAX;

    pcam->turn_z_one   = RAD_TO_ONE( pcam->turn_z_rad );
    pcam->ori.facing_z = ONE_TO_TURN( pcam->turn_z_one ) ;

    camera_reset_view( pcam );

    camera_reset_projection( pcam, camera_t::DEFAULT_FOV, ( float )sdl_scr.x / ( float )sdl_scr.y );

    return pcam;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if 0
void dump_matrix( const fmat_4x4_base_t a )
{
    /// @author ZZ
    /// @details dump a text representation of a 4x4 matrix to stdout

    int i, j;

    if ( NULL == a ) return;

    for ( j = 0; j < 4; j++ )
    {
        printf( "  " );

        for ( i = 0; i < 4; i++ )
        {
            printf( "%f ", a[MAT_IDX( i, j )] );
        }
        printf( "\n" );
    }
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void camera_update_position( camera_t * pcam )
{
    /// @author ZZ
    /// @details This function makes the camera turn to face the character

    TURN_T turnsin;
    fvec3_t pos_new;

    // update the height
    pcam->zgoto = pcam->zadd;

    // update the turn
    //if ( 0 != pcam->turn_time )
    //{
    //    pcam->turn_z_rad   = ATAN2( pcam->center.y - pcam->pos.y, pcam->center.x - pcam->pos.x );  // xgg
    //    pcam->turn_z_one   = RAD_TO_ONE( pcam->turn_z_rad );
    //    pcam->ori.facing_z = ONE_TO_TURN( pcam->turn_z_one );
    //}

    // update the camera position
    turnsin = TO_TURN( pcam->ori.facing_z );
    pos_new.x = pcam->center.x + pcam->zoom * turntosin[turnsin];
    pos_new.y = pcam->center.y + pcam->zoom * turntocos[turnsin];
    pos_new.z = pcam->center.z + pcam->zgoto;

    // make the camera motion smooth
    pcam->pos.x = 0.9f * pcam->pos.x + 0.1f * pos_new.x;
    pcam->pos.y = 0.9f * pcam->pos.y + 0.1f * pos_new.y;
    pcam->pos.z = 0.9f * pcam->pos.z + 0.1f * pos_new.z;
}

//--------------------------------------------------------------------------------------------
static INLINE float camera_multiply_fov( const float old_fov_deg, const float factor )
{
    float old_fov_rad;
    float new_fov_rad, new_fov_deg;

    old_fov_rad = DEG_TO_RAD( old_fov_deg );

    new_fov_rad = 2.0f * ATAN( factor * TAN( old_fov_rad * 0.5f ) );
    new_fov_deg = RAD_TO_DEG( new_fov_rad );

    return new_fov_deg;
}

//--------------------------------------------------------------------------------------------
void camera_gluPerspective( camera_t * pcam, float fovy_deg, float aspect_ratio, float frustum_near, float frustum_far )
{
    const float fov_mag = SQRT_TWO;

    float fov_big   = fovy_deg;
    float fov_small = fovy_deg;
    GLint matrix_mode[1];

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // switch to the projection mode
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();

    // do the actual glu call
    GL_DEBUG( glLoadIdentity )();
    gluPerspective( fovy_deg, aspect_ratio, frustum_near, frustum_far );

    // grab the matrix
    GL_DEBUG( glGetFloatv )( GL_PROJECTION_MATRIX, pcam->mProjection.v );

    // do another glu call
    fov_big = camera_multiply_fov( camera_t::DEFAULT_FOV, fov_mag );
    GL_DEBUG( glLoadIdentity )();
    gluPerspective( fov_big, aspect_ratio, frustum_near, frustum_far );

    // grab the big matrix
    GL_DEBUG( glGetFloatv )( GL_PROJECTION_MATRIX, pcam->mProjection_big.v );

    // do another glu call
    fov_small = camera_multiply_fov( camera_t::DEFAULT_FOV, 1.0f / fov_mag );
    GL_DEBUG( glLoadIdentity )();
    gluPerspective( fov_small, aspect_ratio, frustum_near, frustum_far );

    // grab the small matrix
    GL_DEBUG( glGetFloatv )( GL_PROJECTION_MATRIX, pcam->mProjection_small.v );

    // restore the matrix mode(s)
    GL_DEBUG( glPopMatrix )();
    GL_DEBUG( glMatrixMode )( matrix_mode[0] );

    // recalculate the frustum, too
    egolib_frustum_calculate( &( pcam->frustum ), pcam->mProjection.v, pcam->mView.v );
    egolib_frustum_calculate( &( pcam->frustum_big ), pcam->mProjection_big.v, pcam->mView.v );
    egolib_frustum_calculate( &( pcam->frustum_small ), pcam->mProjection_small.v, pcam->mView.v );
}

//--------------------------------------------------------------------------------------------
void camera_gluLookAt( camera_t * pcam, float roll_deg )
{
    GLint matrix_mode[1];

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // switch to the projection mode
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glLoadIdentity )();

    // get the correct coordinate system
    GL_DEBUG( glScalef )( -1, 1, 1 );

    // check for stupidity
    if (( pcam->pos.x != pcam->center.x ) || ( pcam->pos.y != pcam->center.y ) || ( pcam->pos.z != pcam->center.z ) )
    {
        // do the camera roll
        glRotatef( roll_deg, 0, 0, 1 );

        // do the actual glu call
        gluLookAt(
            pcam->pos.x, pcam->pos.y, pcam->pos.z,
            pcam->center.x, pcam->center.y, pcam->center.z,
            0.0f, 0.0f, 1.0f );
    }

    // grab the matrix
    GL_DEBUG( glGetFloatv )( GL_MODELVIEW_MATRIX, pcam->mView.v );

    // restore the matrix mode(s)
    GL_DEBUG( glPopMatrix )();
    GL_DEBUG( glMatrixMode )( matrix_mode[0] );

    // the view matrix was updated, so update the frustum
    egolib_frustum_calculate( &( pcam->frustum ), pcam->mProjection.v, pcam->mView.v );
    egolib_frustum_calculate( &( pcam->frustum_big ), pcam->mProjection_big.v, pcam->mView.v );
    egolib_frustum_calculate( &( pcam->frustum_small ), pcam->mProjection_small.v, pcam->mView.v );
}

//--------------------------------------------------------------------------------------------
void camera_update_effects( camera_t * pcam )
{
    float local_swingamp = pcam->swing_amp;
    
    pcam->motion_blur_old = pcam->motion_blur;

    // Fade out the motion blur
    if ( pcam->motion_blur > 0 )
    {
        pcam->motion_blur *= 0.99f; //Decay factor
        if ( pcam->motion_blur < 0.001f ) pcam->motion_blur = 0;
    }

    // Swing the camera if players are groggy
    if ( local_stats.grog_level > 0 )
    {
        float zoom_add;
        pcam->swing = ( pcam->swing + 120 ) & 0x3FFF;
        local_swingamp = std::max( local_swingamp, 0.175f );

        zoom_add = ( 0 == ((( int )local_stats.grog_level ) % 2 ) ? 1 : - 1 ) * camera_t::DEFAULT_TURN_KEY * local_stats.grog_level * 0.35f;

        pcam->zadd_goto   = pcam->zadd_goto + zoom_add;

        pcam->zadd_goto = CLIP( pcam->zadd_goto, CAM_ZADD_MIN, CAM_ZADD_MAX );
    }

    //Rotate camera if they are dazed
    if ( local_stats.daze_level > 0 )
    {
        pcam->turn_z_add = local_stats.daze_level * camera_t::DEFAULT_TURN_KEY * 0.5f;
    }
    
    // Apply motion blur
    if ( local_stats.daze_level > 0 || local_stats.grog_level > 0 )
        pcam->motion_blur = std::min( 0.95f, 0.5f + 0.03f * std::max( local_stats.daze_level, local_stats.grog_level ));

    //Apply camera swinging
    //mat_Multiply( pcam->mView.v, mat_Translate( tmp1.v, pcam->pos.x, -pcam->pos.y, pcam->pos.z ), pcam->mViewSave.v );  // xgg
    if ( local_swingamp > 0.001f )
    {
        pcam->roll = turntosin[pcam->swing] * local_swingamp;
        //mat_Multiply( pcam->mView.v, mat_RotateY( tmp1.v, pcam->roll ), mat_Copy( tmp2.v, pcam->mView.v ) );
    }
    // If the camera stops swinging for some reason, slowly return to original position
    else if ( 0 != pcam->roll )
    {
        pcam->roll *= 0.9875f;            //Decay factor
        //mat_Multiply( pcam->mView.v, mat_RotateY( tmp1.v, pcam->roll ), mat_Copy( tmp2.v, pcam->mView.v ) );

        // Come to a standstill at some point
        if ( ABS( pcam->roll ) < 0.001f )
        {
            pcam->roll = 0;
            pcam->swing = 0;
        }
    }

    pcam->swing = ( pcam->swing + pcam->swing_rate ) & 0x3FFF;
}

//--------------------------------------------------------------------------------------------
void camera_make_matrix( camera_t * pcam )
{
    /// @author ZZ
    /// @details This function sets pcam->mView to the camera's location and rotation

    camera_gluLookAt( pcam, RAD_TO_DEG(pcam->roll) );

    //--- pre-compute some camera vectors
    mat_getCamForward( pcam->mView.v, pcam->vfw.v );
    fvec3_self_normalize( pcam->vfw.v );

    mat_getCamUp( pcam->mView.v, pcam->vup.v );
    fvec3_self_normalize( pcam->vup.v );

    mat_getCamRight( pcam->mView.v, pcam->vrt.v );
    fvec3_self_normalize( pcam->vrt.v );
}

//--------------------------------------------------------------------------------------------
void camera_update_zoom( camera_t * pcam )
{
    /// @author ZZ
    /// @details This function makes the camera look downwards as it is raised up

    float percentmin, percentmax;

    if ( NULL == pcam ) return;

    // update zadd
    pcam->zadd_goto = CLIP( pcam->zadd_goto, CAM_ZADD_MIN, CAM_ZADD_MAX );
    pcam->zadd      = 0.9f * pcam->zadd  + 0.1f * pcam->zadd_goto;

    // update zoom
    percentmax = ( pcam->zadd_goto - CAM_ZADD_MIN ) / ( float )( CAM_ZADD_MAX - CAM_ZADD_MIN );
    percentmin = 1.0f - percentmax;
    pcam->zoom = ( CAM_ZOOM_MIN * percentmin ) + ( CAM_ZOOM_MAX * percentmax );

    // update turn_z
    if ( ABS( pcam->turn_z_add ) < 0.5f )
    {
        pcam->turn_z_add = 0.0f;
    }
    else
    {
        pcam->ori.facing_z += pcam->turn_z_add;
        pcam->turn_z_one    = TURN_TO_ONE( pcam->ori.facing_z );
        pcam->turn_z_rad    = ONE_TO_RAD( pcam->turn_z_one );
    }
    pcam->turn_z_add *= pcam->turn_z_sustain;

}

//--------------------------------------------------------------------------------------------
void camera_update_center( camera_t * pcam )
{
    // Center on target for doing rotation...
    if ( 0 != pcam->turn_time )
    {
        pcam->center.x = pcam->center.x * 0.9f + pcam->track_pos.x * 0.1f;
        pcam->center.y = pcam->center.y * 0.9f + pcam->track_pos.y * 0.1f;
    }
    else
    {
        fvec2_t scroll;
        fvec2_t vup, vrt, diff;
        float diff_up, diff_rt;

        float track_fov, track_dist, track_size, track_size_x, track_size_y;
        fvec3_t track_vec;

        // what is the distance to the track position?
        fvec3_sub( track_vec.v, pcam->track_pos.v, pcam->pos.v );

        // determine the size of the dead zone
        track_fov = camera_t::DEFAULT_FOV * 0.25f;
        track_dist = fvec3_length( track_vec.v );
        track_size = track_dist * TAN( track_fov );
        track_size_x = track_size;
        track_size_y = track_size;  /// @todo adjust this based on the camera viewing angle

        // calculate the difference between the center of the tracked characters
        // and the center of the camera look_at
        fvec2_sub( diff.v, pcam->track_pos.v, pcam->center.v );

        // get 2d versions of the camera's right and up vectors
        fvec2_base_copy( vrt.v, pcam->vrt.v );
        fvec2_self_normalize( vrt.v );

        fvec2_base_copy( vup.v, pcam->vup.v );
        fvec2_self_normalize( vup.v );

        // project the diff vector into this space
        diff_rt = fvec2_dot_product( vrt.v, diff.v );
        diff_up = fvec2_dot_product( vup.v, diff.v );

        // Get ready to scroll...
        scroll.x = 0;
        scroll.y = 0;
        if ( diff_rt < -track_size_x )
        {
            // Scroll left
            scroll.x += vrt.x * ( diff_rt + track_size_x );
            scroll.y += vrt.y * ( diff_rt + track_size_x );
        }
        if ( diff_rt > track_size_x )
        {
            // Scroll right
            scroll.x += vrt.x * ( diff_rt - track_size_x );
            scroll.y += vrt.y * ( diff_rt - track_size_x );
        }

        if ( diff_up > track_size_y )
        {
            // Scroll down
            scroll.x += vup.x * ( diff_up - track_size_y );
            scroll.y += vup.y * ( diff_up - track_size_y );
        }

        if ( diff_up < -track_size_y )
        {
            // Scroll up
            scroll.x += vup.x * ( diff_up + track_size_y );
            scroll.y += vup.y * ( diff_up + track_size_y );
        }

        // scroll
        pcam->center.x += scroll.x;
        pcam->center.y += scroll.y;
    }

    // center.z always approaches track_pos.z
    pcam->center.z = pcam->center.z * 0.9f + pcam->track_pos.z * 0.1f;
}

//--------------------------------------------------------------------------------------------
void camera_update_track( camera_t * pcam, const ego_mesh_t * pmesh, CHR_REF track_list[], const size_t track_list_size )
{
    fvec3_t new_track;
    float new_track_level;

    // the default camera motion is to do nothing
    new_track.x     = pcam->track_pos.x;
    new_track.y     = pcam->track_pos.y;
    new_track.z     = pcam->track_pos.z;
    new_track_level = pcam->track_level;

    if ( CAM_FREE == pcam->move_mode )
    {
        // the keypad control the camera
        if ( SDL_KEYDOWN( keyb, SDLK_KP8 ) )
        {
            pcam->track_pos.x -= pcam->mView.CNV( 0, 1 ) * 50;
            pcam->track_pos.y -= pcam->mView.CNV( 1, 1 ) * 50;
        }

        if ( SDL_KEYDOWN( keyb, SDLK_KP2 ) )
        {
            pcam->track_pos.x += pcam->mView.CNV( 0, 1 ) * 50;
            pcam->track_pos.y += pcam->mView.CNV( 1, 1 ) * 50;
        }

        if ( SDL_KEYDOWN( keyb, SDLK_KP4 ) )
        {
            pcam->track_pos.x += pcam->mView.CNV( 0, 0 ) * 50;
            pcam->track_pos.y += pcam->mView.CNV( 1, 0 ) * 50;
        }

        if ( SDL_KEYDOWN( keyb, SDLK_KP6 ) )
        {
            pcam->track_pos.x -= pcam->mView.CNV( 0, 0 ) * 10;
            pcam->track_pos.y -= pcam->mView.CNV( 1, 0 ) * 10;
        }

        if ( SDL_KEYDOWN( keyb, SDLK_KP7 ) )
        {
            pcam->turn_z_add += camera_t::DEFAULT_TURN_KEY;
        }

        if ( SDL_KEYDOWN( keyb, SDLK_KP9 ) )
        {
            pcam->turn_z_add -= camera_t::DEFAULT_TURN_KEY;
        }

        pcam->track_pos.z = 128 + ego_mesh_get_level( pmesh, pcam->track_pos.x, pcam->track_pos.y );
    }
    else if ( CAM_RESET == pcam->move_mode )
    {
        // a camera movement mode for re-focusing in on a bunch of players

        int cnt;
        fvec3_t sum_pos;
        float   sum_wt, sum_level;

        sum_wt    = 0.0f;
        sum_level = 0.0f;
        fvec3_self_clear( sum_pos.v );

        for ( cnt = 0; cnt < track_list_size; cnt++ )
        {
            chr_t * pchr = NULL;
            CHR_REF ichr = track_list[cnt];

            if ( !ACTIVE_CHR( ichr ) ) continue;
            pchr = ChrList_get_ptr( ichr );

            if ( !pchr->alive ) continue;

            sum_pos.x += pchr->pos.x;
            sum_pos.y += pchr->pos.y;
            sum_pos.z += pchr->pos.z + pchr->chr_min_cv.maxs[OCT_Z] * 0.9f;
            sum_level += pchr->enviro.level;
            sum_wt    += 1.0f;
        }

        // if any of the characters is doing anything
        if ( sum_wt > 0.0f )
        {
            new_track.x     = sum_pos.x / sum_wt;
            new_track.y     = sum_pos.y / sum_wt;
            new_track.z     = sum_pos.z / sum_wt;
            new_track_level = sum_level / sum_wt;
        }
    }
    else if ( CAM_PLAYER == pcam->move_mode )
    {
        // a camera mode for focusing in on the players that are actually doing something.
        // "Show me the drama!"
        chr_t * local_chr_ptrs[MAX_PLAYER];
        int local_chr_count = 0;

        // count the number of local players, first
        local_chr_count = 0;
        for ( size_t cnt = 0; cnt < track_list_size; cnt++ )
        {
            chr_t * pchr = NULL;
            CHR_REF ichr = track_list[cnt];

            if ( !ACTIVE_CHR( ichr ) ) continue;
            pchr = ChrList_get_ptr( ichr );

            if ( !pchr->alive ) continue;

            local_chr_ptrs[local_chr_count] = pchr;
            local_chr_count++;
        }

        if ( 0 == local_chr_count )
        {
            // do nothing
        }
        else if ( 1 == local_chr_count )
        {
            // copy from the one character

            new_track.x = local_chr_ptrs[0]->pos.x;
            new_track.y = local_chr_ptrs[0]->pos.y;
            new_track.z = local_chr_ptrs[0]->pos.z;
            new_track_level = local_chr_ptrs[0]->enviro.level + 128;
        }
        else
        {
            // use the characer's "activity" to average the position the camera is viewing

            fvec3_t sum_pos;
            float   sum_wt, sum_level;

            sum_wt    = 0.0f;
            sum_level = 0.0f;
            fvec3_self_clear( sum_pos.v );

            for ( int cnt = 0; cnt < local_chr_count; cnt++ )
            {
                chr_t * pchr;
                float weight1, weight2, weight;

                // we JUST checked the validity of these characters. No need to do it again?
                pchr = local_chr_ptrs[ cnt ];

                // weight it by the character's velocity^2, so that
                // inactive characters don't control the camera
                weight1 = fvec3_dot_product( pchr->vel.v, pchr->vel.v );

                // make another weight based on button-pushing
                weight2 = ( 0 == pchr->latch.b ) ? 0 : 127;

                // I would weight this by the amount of damage that the character just sustained,
                // but there is no real way to do this?

                // get the maximum effect
                weight =  std::max( weight1, weight2 );

                // The character is on foot
                sum_pos.x += pchr->pos.x * weight;
                sum_pos.y += pchr->pos.y * weight;
                sum_pos.z += pchr->pos.z * weight;
                sum_level += ( pchr->enviro.level + 128 ) * weight;
                sum_wt    += weight;
            }

            // if any of the characters is doing anything
            if ( sum_wt > 0.0f )
            {
                new_track.x = sum_pos.x / sum_wt;
                new_track.y = sum_pos.y / sum_wt;
                new_track.z = sum_pos.z / sum_wt;
                new_track_level = sum_level / sum_wt;
            }
        }
    }

    if ( CAM_RESET == pcam->move_mode )
    {
        // just set the position
        pcam->track_pos.x = new_track.x;
        pcam->track_pos.y = new_track.y;
        pcam->track_pos.z = new_track.z;
        pcam->track_level = new_track_level;

        // reset the camera mode
        pcam->move_mode = pcam->move_mode_old;
    }
    else
    {
        // smoothly interpolate the camera tracking position
        pcam->track_pos.x = 0.9f * pcam->track_pos.x + 0.1f * new_track.x;
        pcam->track_pos.y = 0.9f * pcam->track_pos.y + 0.1f * new_track.y;
        pcam->track_pos.z = 0.9f * pcam->track_pos.z + 0.1f * new_track.z;
        pcam->track_level = 0.9f * pcam->track_level + 0.1f * new_track_level;
    }

}

//--------------------------------------------------------------------------------------------
size_t camera_create_track_list( CHR_REF track_list[], const size_t track_list_max_size )
{
    /// @author ZZ
    /// @details Create a default list of obhects that are tracked

    PLA_REF ipla;

    size_t track_count = 0;

    if ( NULL == track_list || 0 == track_list_max_size ) return 0;

    // blank out the list
    track_list[0] = INVALID_CHR_REF;
    track_count = 0;

    // scan the PlaStack for objects the camera can track
    for ( ipla = 0; ipla < MAX_PLAYER && track_count < track_list_max_size; ipla++ )
    {
        player_t * ppla = PlaStack_get_ptr( ipla );

        if ( !ppla->valid || !ACTIVE_CHR( ppla->index ) ) continue;

        // add in a valid character
        track_list[track_count] = ppla->index;
        track_count++;
    }

    return track_count;
}

//--------------------------------------------------------------------------------------------
void camera_move( camera_t * pcam, const ego_mesh_t * pmesh, const CHR_REF track_list[], const size_t track_list_size )
{
    /// @author ZZ
    /// @details This function moves the camera

    PLA_REF ipla;

    CHR_REF _track_list[ MAX_PLAYER ];

    CHR_REF * loc_track_list  = ( CHR_REF * )track_list;
    size_t    loc_target_count = track_list_size;

    // deal with optional parameters
    if ( NULL == track_list )
    {
        loc_track_list  = ( CHR_REF * )_track_list;
        loc_target_count = camera_create_track_list( _track_list, SDL_arraysize( _track_list ) );
    }

    // update the turn_time counter
    if ( CAM_TURN_NONE != pcam->turn_mode )
    {
        pcam->turn_time = 255;
    }
    else if ( pcam->turn_time > 0 )
    {
        pcam->turn_time--;
    }

    // Camera controls
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        player_t *ppla;

        //Don't do invalid players
        if ( INVALID_PLA( ipla ) ) continue;
        ppla = PlaStack_get_ptr( ipla );

        //Handle camera control from this player
        camera_read_input( pcam, ppla->pdevice );
    }

    // update the special camera effects like grog
    camera_update_effects( pcam );

    // update the average position of the tracked characters
    camera_update_track( pcam, pmesh, loc_track_list, loc_target_count );

    // move the camera center, if need be
    camera_update_center( pcam );

    // make the zadd and zoom work together
    camera_update_zoom( pcam );

    // update the position of the camera
    camera_update_position( pcam );

    // set the view matrix
    camera_make_matrix( pcam );
}

//--------------------------------------------------------------------------------------------
void camera_read_input( camera_t *pcam, input_device_t *pdevice )
{
    /// @author ZF
    /// @details Read camera control input for one specific player controller

    int type;
    bool autoturn_camera;

    //Don't do network players
    if ( NULL == pdevice ) return;
    type = pdevice->device_type;

    //If the device isn't enabled there is no point in continuing
    if ( !input_device_is_enabled( pdevice ) ) return;

    //Autoturn camera only works in single player and when it is enabled
    autoturn_camera = ( CAM_TURN_GOOD == pcam->turn_mode ) && ( 1 == local_stats.player_count );

    //No auto camera
    if ( INPUT_DEVICE_MOUSE == type )
    {
        // Mouse control

        //Auto camera
        if ( autoturn_camera )
        {
            if ( !input_device_control_active( pdevice,  CONTROL_CAMERA ) )
            {
                pcam->turn_z_add -= ( mous.x * 0.5f );
            }
        }

        //Normal camera
        else if ( input_device_control_active( pdevice,  CONTROL_CAMERA ) )
        {
            pcam->turn_z_add += ( mous.x / 3.0f );
            pcam->zadd_goto  += ( float ) mous.y / 3.0f;

            pcam->turn_time = camera_t::DEFAULT_TURN_TIME;  // Sticky turn...
        }
    }
    else if ( IS_VALID_JOYSTICK( type ) )
    {
        // Joystick camera controls

        int ijoy = type - INPUT_DEVICE_JOY;

        // figure out which joystick this is
        joystick_data_t *pjoy = joy_lst + ijoy;

        // Autocamera
        if ( autoturn_camera )
        {
            if ( !input_device_control_active( pdevice, CONTROL_CAMERA ) )
            {
                pcam->turn_z_add -= pjoy->x * camera_t::DEFAULT_TURN_JOY;
            }
        }

        //Normal camera
        else if ( input_device_control_active( pdevice, CONTROL_CAMERA ) )
        {
            pcam->turn_z_add += pjoy->x * camera_t::DEFAULT_TURN_JOY;
            pcam->zadd_goto  += pjoy->y * camera_t::DEFAULT_TURN_JOY;

            pcam->turn_time = camera_t::DEFAULT_TURN_TIME;  // Sticky turn...
        }
    }
    else
    {
        // INPUT_DEVICE_KEYBOARD and any unknown device end up here

        if ( autoturn_camera )
        {
            // Auto camera

            if ( input_device_control_active( pdevice,  CONTROL_LEFT ) )
            {
                pcam->turn_z_add += camera_t::DEFAULT_TURN_KEY;
            }
            if ( input_device_control_active( pdevice,  CONTROL_RIGHT ) )
            {
                pcam->turn_z_add -= camera_t::DEFAULT_TURN_KEY;
            }
        }
        else
        {
            // Normal camera

            int turn_z_diff = 0;

            // rotation
            if ( input_device_control_active( pdevice,  CONTROL_CAMERA_LEFT ) )
            {
                turn_z_diff += camera_t::DEFAULT_TURN_KEY;
            }
            if ( input_device_control_active( pdevice,  CONTROL_CAMERA_RIGHT ) )
            {
                turn_z_diff -= camera_t::DEFAULT_TURN_KEY;
            }

            // Sticky turn?
            if ( 0 != turn_z_diff )
            {
                pcam->turn_z_add += turn_z_diff;
                pcam->turn_time   = camera_t::DEFAULT_TURN_TIME;
            }

            //zoom
            if ( input_device_control_active( pdevice,  CONTROL_CAMERA_OUT ) )
            {
                pcam->zadd_goto += camera_t::DEFAULT_TURN_KEY;
            }
            if ( input_device_control_active( pdevice,  CONTROL_CAMERA_IN ) )
            {
                pcam->zadd_goto -= camera_t::DEFAULT_TURN_KEY;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void camera_reset( camera_t * pcam, const ego_mesh_t * pmesh, const CHR_REF track_list[], const size_t track_list_size )
{
    /// @author ZZ
    /// @details This function makes sure the camera starts in a suitable position

    // constant values
    pcam->swing        = 0;
    pcam->track_level  = 0;
    pcam->zoom         = CAM_ZOOM_AVG;
    pcam->zadd         = CAM_ZADD_AVG;
    pcam->zadd_goto     = CAM_ZADD_AVG;
    pcam->zgoto        = CAM_ZADD_AVG;
    pcam->turn_z_rad   = -PI_OVER_FOUR;
    pcam->turn_z_add   = 0;
    pcam->roll         = 0;

    // derived values
    pcam->center.x     = pmesh->gmem.edge_x * 0.5f;
    pcam->center.y     = pmesh->gmem.edge_y * 0.5f;
    pcam->center.z     = 0.0f;

    fvec3_base_copy( pcam->track_pos.v, pcam->center.v );
    fvec3_base_copy( pcam->pos.v,       pcam->center.v );

    pcam->pos.x += pcam->zoom * SIN( pcam->turn_z_rad );
    pcam->pos.y += pcam->zoom * COS( pcam->turn_z_rad );
    pcam->pos.z += CAM_ZADD_MAX;

    pcam->turn_z_one   = RAD_TO_ONE( pcam->turn_z_rad );
    pcam->ori.facing_z = ONE_TO_TURN( pcam->turn_z_one ) ;

    // get optional parameters
    pcam->swing      = cam_options.swing;
    pcam->swing_rate = cam_options.swing_rate;
    pcam->swing_amp  = cam_options.swing_amp;
    pcam->turn_mode  = cam_options.turn_mode;

    // make sure you are looking at the players
    camera_reset_target( pcam, pmesh, track_list, track_list_size );
}

//--------------------------------------------------------------------------------------------
bool camera_reset_target( camera_t * pcam, const ego_mesh_t * pmesh, const CHR_REF track_list[], const size_t track_list_size )
{
    /// @author BB
    /// @details Force the camera to focus in on the players. Should be called any time there is
    ///          a "change of scene". With the new velocity-tracking of the camera, this would include
    ///          things like character respawns, adding new players, etc.

	e_camera_turn_mode turn_mode_save;
	Uint8 move_mode_save;

    if ( NULL == pcam ) return false;

    // save some values
    turn_mode_save = pcam->turn_mode;
    move_mode_save = pcam->move_mode;

    // get back to the default view matrix
    camera_reset_view( pcam );

    // specify the modes that will make the camera point at the players
    pcam->turn_mode = CAM_TURN_AUTO;
    pcam->move_mode = CAM_RESET;

    // If you use CAM_RESET, camera_move() automatically restores pcam->move_mode
    // to its default setting
    camera_move( pcam, pmesh, track_list, track_list_size );

    // fix the center position
    pcam->center.x = pcam->track_pos.x;
    pcam->center.y = pcam->track_pos.y;

    // restore the modes
    pcam->turn_mode = turn_mode_save;
    pcam->move_mode = move_mode_save;

    // reset the turn time
    pcam->turn_time = 0;

    return true;
}

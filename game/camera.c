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

/// @file camera.c
/// @brief Various functions related to how the game camera works.
/// @details

#include "camera.h"

#include "char.inl"
#include "mesh.inl"

#include "input.h"
#include "graphic.h"
#include "network.h"
#include "controls_file.h"

#include "egoboo_setup.h"
#include "egoboo.h"

#include "SDL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

camera_t gCamera;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Camera control stuff

//--------------------------------------------------------------------------------------------
void camera_rotmesh__init()
{
    // Matrix init stuff (from remove.c)
    rotmesh_topside    = (( float )sdl_scr.x / sdl_scr.y ) * CAM_ROTMESH_TOPSIDE / ( 1.33333f );
    rotmesh_bottomside = (( float )sdl_scr.x / sdl_scr.y ) * CAM_ROTMESH_BOTTOMSIDE / ( 1.33333f );
    rotmesh_up         = (( float )sdl_scr.x / sdl_scr.y ) * CAM_ROTMESH_UP / ( 1.33333f );
    rotmesh_down       = (( float )sdl_scr.x / sdl_scr.y ) * CAM_ROTMESH_DOWN / ( 1.33333f );
}

//--------------------------------------------------------------------------------------------
camera_t * camera_ctor( camera_t * pcam )
{
    /// @detalis BB@> initialize the camera structure

    fvec3_t   t1 = {{0, 0, 0}};
    fvec3_t   t2 = {{0, 0, -1}};
    fvec3_t   t3 = {{0, 1, 0}};

    memset( pcam, 0, sizeof( *pcam ) );

    pcam->move_mode = pcam->move_mode_old = CAM_PLAYER;
    pcam->turn_mode = cfg.autoturncamera;

    pcam->swing        =  0;
    pcam->swingrate    =  0;
    pcam->swingamp     =  0;
    pcam->pos.x        =  0;
    pcam->pos.y        =  1500;
    pcam->pos.z        =  1500;
    pcam->zoom         =  1000;
    pcam->zadd         =  800;
    pcam->zaddgoto     =  800;
    pcam->zgoto        =  800;
    pcam->turn_z_rad   = -PI / 4.0f;
    pcam->turn_z_one   = pcam->turn_z_rad / TWO_PI;
    pcam->ori.facing_z = CLIP_TO_16BITS(( int )( pcam->turn_z_one * ( float )0x00010000 ) ) ;
    pcam->turnadd      =  0;
    pcam->sustain      =  0.60f;
    pcam->turnupdown   = ( float )( PI / 4 );
    pcam->roll         =  0;
    pcam->motion_blur  =  0;

    pcam->mView       = pcam->mViewSave = ViewMatrix( t1.v, t2.v, t3.v, 0 );
    pcam->mProjection = ProjectionMatrix( .001f, 2000.0f, ( float )( CAM_FOV * PI / 180 ) ); // 60 degree CAM_FOV
    pcam->mProjection = MatrixMult( Translate( 0, 0, -0.999996f ), pcam->mProjection ); // Fix Z value...
    pcam->mProjection = MatrixMult( ScaleXYZ( -1, -1, 100000 ), pcam->mProjection );  // HUK // ...'cause it needs it

    // [claforte] Fudge the values.
    pcam->mProjection.v[10] /= 2.0f;
    pcam->mProjection.v[11] /= 2.0f;

    camera_rotmesh__init();

    return pcam;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void dump_matrix( fmat_4x4_t a )
{
    /// @detalis ZZ@> dump a text representation of a 4x4 matrix to stdout

    int i; int j;

    for ( j = 0; j < 4; j++ )
    {
        printf( "  " );

        for ( i = 0; i < 4; i++ )
        {
            printf( "%f ", a.CNV( i, j ) );
        }
        printf( "\n" );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void camera_look_at( camera_t * pcam, float x, float y )
{
    /// @details ZZ@> This function makes the camera turn to face the character

    pcam->zgoto = pcam->zadd;
    if ( 0 != pcam->turn_time )
    {
        pcam->turn_z_rad = ( 1.5f * PI ) - ATAN2( y - pcam->pos.y, x - pcam->pos.x );  // xgg
    }
}

//--------------------------------------------------------------------------------------------
void camera_make_matrix( camera_t * pcam )
{
    /// @details ZZ@> This function sets pcam->mView to the camera's location and rotation

    float local_swingamp = pcam->swingamp;

    //Fade out the motion blur
    if ( pcam->motion_blur > 0 )
    {
        pcam->motion_blur *= 0.99f; //Decay factor
        if ( pcam->motion_blur < 0.001f ) pcam->motion_blur = 0;
    }

    //Swing the camera if players are groggy and apply motion blur
    if ( local_stats.grog_level > 0 )
    {
        float zoom_add;
        pcam->swing = ( pcam->swing + 120 ) & 0x3FFF;
        local_swingamp = MAX( local_swingamp, 0.175f );

        zoom_add = ( 0 == ((( int )local_stats.grog_level ) % 2 ) ? 1 : - 1 ) * CAM_TURN_KEY * local_stats.grog_level * 0.35f;
        pcam->zaddgoto = CLIP( pcam->zaddgoto + zoom_add, CAM_ZADD_MIN, CAM_ZADD_MAX );
        pcam->motion_blur = MIN( 1.00f, 0.5f + 0.03f * local_stats.grog_level );
    }

    //Rotate camera if they are dazed and apply motion blur
    if ( local_stats.daze_level > 0 )
    {
        pcam->turnadd = local_stats.daze_level * CAM_TURN_KEY * 0.5f;
        pcam->motion_blur = MIN( 1.00f, 0.5f + 0.03f * local_stats.daze_level );
    }

    //Apply camera swinging
    pcam->mView = MatrixMult( Translate( pcam->pos.x, -pcam->pos.y, pcam->pos.z ), pcam->mViewSave );  // xgg
    if ( local_swingamp > 0.001f )
    {
        pcam->roll = turntosin[pcam->swing] * local_swingamp;
        pcam->mView = MatrixMult( RotateY( pcam->roll ), pcam->mView );
    }

    // If the camera stops swinging for some reason, slowly return to original position
    else if ( 0 != pcam->roll )
    {
        pcam->roll *= 0.9875f;            //Decay factor
        pcam->mView = MatrixMult( RotateY( pcam->roll ), pcam->mView );

        // Come to a standstill at some point
        if ( ABS( pcam->roll ) < 0.001f )
        {
            pcam->roll = 0;
            pcam->swing = 0;
        }
    }

    pcam->mView = MatrixMult( RotateZ( pcam->turn_z_rad ), pcam->mView );
    pcam->mView = MatrixMult( RotateX( pcam->turnupdown ), pcam->mView );

    //--- pre-compute some camera vectors
    mat_getCamForward( pcam->mView.v, pcam->vfw.v );
    fvec3_self_normalize( pcam->vfw.v );

    mat_getCamUp( pcam->mView.v, pcam->vup.v );
    fvec3_self_normalize( pcam->vup.v );

    mat_getCamRight( pcam->mView.v, pcam->vrt.v );
    fvec3_self_normalize( pcam->vrt.v );
}

//--------------------------------------------------------------------------------------------
void camera_adjust_angle( camera_t * pcam, float height )
{
    /// @details ZZ@> This function makes the camera look downwards as it is raised up

    float percentmin, percentmax;
    if ( height < CAM_ZADD_MIN )  height = CAM_ZADD_MIN;

    percentmax = ( height - CAM_ZADD_MIN ) / ( float )( CAM_ZADD_MAX - CAM_ZADD_MIN );
    percentmin = 1.0f - percentmax;

    pcam->turnupdown = (( CAM_UPDOWN_MIN * percentmin ) + ( CAM_UPDOWN_MAX * percentmax ) );
    pcam->zoom = ( CAM_ZOOM_MIN * percentmin ) + ( CAM_ZOOM_MAX * percentmax );
}

//--------------------------------------------------------------------------------------------
void camera_move( camera_t * pcam, ego_mpd_t * pmesh )
{
    /// @details ZZ@> This function moves the camera

    Uint16 cnt;
    float x, y, z, level, newx, newy, movex, movey;
    Uint16 turnsin;

    if ( CAM_TURN_NONE != pcam->turn_mode )
        pcam->turn_time = 255;
    else if ( 0 != pcam->turn_time )
        pcam->turn_time--;

    // the default camera motion is to do nothing
    x     = pcam->track_pos.x;
    y     = pcam->track_pos.y;
    z     = pcam->track_pos.z;
    level = 128 + mesh_get_level( pmesh, x, y );

    if ( CAM_FREE == pcam->move_mode )
    {
        // the keypad controls the camera
        if ( SDLKEYDOWN( SDLK_KP8 ) )
        {
            pcam->track_pos.x -= pcam->mView.CNV( 0, 1 ) * 50;
            pcam->track_pos.y -= pcam->mView.CNV( 1, 1 ) * 50;
        }

        if ( SDLKEYDOWN( SDLK_KP2 ) )
        {
            pcam->track_pos.x += pcam->mView.CNV( 0, 1 ) * 50;
            pcam->track_pos.y += pcam->mView.CNV( 1, 1 ) * 50;
        }

        if ( SDLKEYDOWN( SDLK_KP4 ) )
        {
            pcam->track_pos.x += pcam->mView.CNV( 0, 0 ) * 50;
            pcam->track_pos.y += pcam->mView.CNV( 1, 0 ) * 50;
        }

        if ( SDLKEYDOWN( SDLK_KP6 ) )
        {
            pcam->track_pos.x -= pcam->mView.CNV( 0, 0 ) * 10;
            pcam->track_pos.y -= pcam->mView.CNV( 1, 0 ) * 10;
        }

        if ( SDLKEYDOWN( SDLK_KP7 ) )
        {
            pcam->turnadd += CAM_TURN_KEY;
        }

        if ( SDLKEYDOWN( SDLK_KP9 ) )
        {
            pcam->turnadd -= CAM_TURN_KEY;
        }

        pcam->track_pos.z = 128 + mesh_get_level( pmesh, pcam->track_pos.x, pcam->track_pos.y );
    }
    else if ( CAM_RESET == pcam->move_mode )
    {
        // a camera movement mode for re-focusing in on a bunch of players

        PLA_REF ipla;
        fvec3_t sum_pos;
        float   sum_wt, sum_level;

        sum_wt    = 0.0f;
        sum_level = 0.0f;
        fvec3_self_clear( sum_pos.v );

        for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
        {
            chr_t * pchr;

            pchr = pla_get_pchr( ipla );
            if ( NULL == pchr || !pchr->alive ) continue;

            sum_pos.x += pchr->pos.x;
            sum_pos.y += pchr->pos.y;
            sum_pos.z += pchr->pos.z + pchr->chr_min_cv.maxs[OCT_Z] * 0.9f;
            sum_level += pchr->enviro.level;
            sum_wt    += 1.0f;
        }

        // if any of the characters is doing anything
        if ( sum_wt > 0.0f )
        {
            x     = sum_pos.x / sum_wt;
            y     = sum_pos.y / sum_wt;
            z     = sum_pos.z / sum_wt;
            level = sum_level / sum_wt;
        }
    }
    else if ( CAM_PLAYER == pcam->move_mode )
    {
        // a camera mode for focusing in on the players that are actually doing something.
        // "Show me the drama!"

        PLA_REF ipla;
        chr_t * local_chr_ptrs[MAX_PLAYER];
        int local_chr_count = 0;

        // count the number of local players, first
        local_chr_count = 0;
        for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
        {
            chr_t * pchr;

            pchr = pla_get_pchr( ipla );
            if ( NULL == pchr || !pchr->alive ) continue;

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

            x = local_chr_ptrs[0]->pos.x;
            y = local_chr_ptrs[0]->pos.y;
            z = local_chr_ptrs[0]->pos.z;
            level = local_chr_ptrs[0]->enviro.level;
        }
        else
        {
            // use the characer's "activity" to average the position the camera is viewing

            fvec3_t sum_pos;
            float   sum_wt, sum_level;

            sum_wt    = 0.0f;
            sum_level = 0.0f;
            fvec3_self_clear( sum_pos.v );

            for ( cnt = 0; cnt < local_chr_count; cnt++ )
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
                weight =  MAX( weight1, weight2 );

                // The character is on foot
                sum_pos.x += pchr->pos.x * weight;
                sum_pos.y += pchr->pos.y * weight;
                sum_pos.z += pchr->pos.z * weight;
                sum_level += pchr->enviro.level * weight;
                sum_wt    += weight;
            }

            // if any of the characters is doing anything
            if ( sum_wt > 0 )
            {
                x = sum_pos.x / sum_wt;
                y = sum_pos.y / sum_wt;
                z = sum_pos.z / sum_wt;
                level = sum_level / sum_wt;
            }
        }
    }

    if ( CAM_RESET == pcam->move_mode )
    {
        // just set the position
        pcam->track_pos.x = x;
        pcam->track_pos.y = y;
        pcam->track_pos.z = z;
        pcam->track_level = level;

        // reset the camera mode
        pcam->move_mode = pcam->move_mode_old;
    }
    else
    {
        // smoothly interpolate the camera tracking position
        pcam->track_pos.x = 0.9f * pcam->track_pos.x + 0.1f * x;
        pcam->track_pos.y = 0.9f * pcam->track_pos.y + 0.1f * y;
        pcam->track_pos.z = 0.9f * pcam->track_pos.z + 0.1f * z;
        pcam->track_level = 0.9f * pcam->track_level + 0.1f * level;
    }

    pcam->turnadd = pcam->turnadd * pcam->sustain;
    pcam->zadd    = 0.9f * pcam->zadd  + 0.1f * pcam->zaddgoto;
    pcam->pos.z   = 0.9f * pcam->pos.z + 0.1f * pcam->zgoto;

    // Camera controls
    if ( CAM_TURN_GOOD == pcam->turn_mode && 1 == local_numlpla )
    {
        if ( mous.on )
        {
            if ( !control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) )
            {
                pcam->turnadd -= ( mous.x * 0.5f );
            }
        }

        if ( keyb.on )
        {
            pcam->turnadd += ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) ) * CAM_TURN_KEY;
        }

        if ( joy[0].on )
        {
            if ( !control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_CAMERA ) )
            {
                pcam->turnadd -= joy[0].x * CAM_TURN_JOY;
            }
        }

        if ( joy[1].on )
        {
            if ( !control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_CAMERA ) )
            {
                pcam->turnadd -= joy[1].x * CAM_TURN_JOY;
            }
        }
    }
    else
    {
        if ( mous.on )
        {
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) )
            {
                pcam->turnadd += ( mous.x / 3.0f );
                pcam->zaddgoto += ( float ) mous.y / 3.0f;
                if ( pcam->zaddgoto < CAM_ZADD_MIN )  pcam->zaddgoto = CAM_ZADD_MIN;
                if ( pcam->zaddgoto > CAM_ZADD_MAX )  pcam->zaddgoto = CAM_ZADD_MAX;

                pcam->turn_time = CAM_TURN_TIME;  // Sticky turn...
            }
        }

        // JoyA camera controls
        if ( joy[0].on )
        {
            if ( control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_CAMERA ) )
            {
                pcam->turnadd += joy[0].x * CAM_TURN_JOY;
                pcam->zaddgoto += joy[0].y * CAM_TURN_JOY;
                if ( pcam->zaddgoto < CAM_ZADD_MIN )  pcam->zaddgoto = CAM_ZADD_MIN;
                if ( pcam->zaddgoto > CAM_ZADD_MAX )  pcam->zaddgoto = CAM_ZADD_MAX;

                pcam->turn_time = CAM_TURN_TIME;  // Sticky turn...
            }
        }

        // JoyB camera controls
        if ( joy[1].on )
        {
            if ( control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_CAMERA ) )
            {
                pcam->turnadd += joy[1].x * CAM_TURN_JOY;
                pcam->zaddgoto += joy[1].y * CAM_TURN_JOY;
                if ( pcam->zaddgoto < CAM_ZADD_MIN )  pcam->zaddgoto = CAM_ZADD_MIN;
                if ( pcam->zaddgoto > CAM_ZADD_MAX )  pcam->zaddgoto = CAM_ZADD_MAX;

                pcam->turn_time = CAM_TURN_TIME;  // Sticky turn...
            }
        }
    }

    // Keyboard camera controls
    if ( keyb.on )
    {
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_LEFT ) || control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_RIGHT ) )
        {
            pcam->turnadd += ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_LEFT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_RIGHT ) ) * CAM_TURN_KEY;
            pcam->turn_time = CAM_TURN_TIME;  // Sticky turn...
        }

        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_IN ) || control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_OUT ) )
        {
            pcam->zaddgoto += ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_OUT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_IN ) ) * CAM_TURN_KEY;
            if ( pcam->zaddgoto < CAM_ZADD_MIN )  pcam->zaddgoto = CAM_ZADD_MIN;
            if ( pcam->zaddgoto > CAM_ZADD_MAX )  pcam->zaddgoto = CAM_ZADD_MAX;
        }
    }

    pcam->pos.x -= ( float )( pcam->mView.CNV( 0, 0 ) ) * pcam->turnadd; // xgg
    pcam->pos.y += ( float )( pcam->mView.CNV( 1, 0 ) ) * -pcam->turnadd;

    // Center on target for doing rotation...
    if ( 0 != pcam->turn_time )
    {
        pcam->center.x = pcam->center.x * 0.9f + pcam->track_pos.x * 0.1f;
        pcam->center.y = pcam->center.y * 0.9f + pcam->track_pos.y * 0.1f;
    }

    // Create a tolerance area for walking without camera movement
    x = pcam->track_pos.x - pcam->pos.x;
    y = pcam->track_pos.y - pcam->pos.y;
    newx = -( pcam->mView.CNV( 0, 0 ) * x + pcam->mView.CNV( 1, 0 ) * y ); // newx = -(pcam->mView(0,0) * x + pcam->mView(1,0) * y);
    newy = -( pcam->mView.CNV( 0, 1 ) * x + pcam->mView.CNV( 1, 1 ) * y ); // newy = -(pcam->mView(0,1) * x + pcam->mView(1,1) * y);

    // Get ready to scroll...
    movex = 0;
    movey = 0;

    // Adjust for camera height...
    z = ( CAM_TRACK_X_AREA_LOW  * ( CAM_ZADD_MAX - pcam->zadd ) ) +
        ( CAM_TRACK_X_AREA_HIGH * ( pcam->zadd - CAM_ZADD_MIN ) );
    z = z / ( CAM_ZADD_MAX - CAM_ZADD_MIN );
    if ( newx < -z )
    {
        // Scroll left
        movex += ( newx + z );
    }
    if ( newx > z )
    {
        // Scroll right
        movex += ( newx - z );
    }

    // Adjust for camera height...
    z = ( CAM_TRACK_Y_AREA_MINLOW  * ( CAM_ZADD_MAX - pcam->zadd ) ) +
        ( CAM_TRACK_Y_AREA_MINHIGH * ( pcam->zadd - CAM_ZADD_MIN ) );
    z = z / ( CAM_ZADD_MAX - CAM_ZADD_MIN );
    if ( newy < z )
    {
        // Scroll down
        movey -= ( newy - z );
    }
    else
    {
        // Adjust for camera height...
        z = ( CAM_TRACK_Y_AREA_MAXLOW  * ( CAM_ZADD_MAX - pcam->zadd ) ) +
            ( CAM_TRACK_Y_AREA_MAXHIGH * ( pcam->zadd - CAM_ZADD_MIN ) );
        z = z / ( CAM_ZADD_MAX - CAM_ZADD_MIN );
        if ( newy > z )
        {
            // Scroll up
            movey -= ( newy - z );
        }
    }

    turnsin = TO_TURN( pcam->ori.facing_z );
    pcam->center.x += movex * turntocos[ turnsin & TRIG_TABLE_MASK ] + movey * turntosin[ turnsin & TRIG_TABLE_MASK ];
    pcam->center.y += -movex * turntosin[ turnsin & TRIG_TABLE_MASK ] + movey * turntocos[ turnsin & TRIG_TABLE_MASK ];

    // Finish up the camera
    camera_look_at( pcam, pcam->center.x, pcam->center.y );
    pcam->pos.x = ( float ) pcam->center.x + ( pcam->zoom * SIN( pcam->turn_z_rad ) );
    pcam->pos.y = ( float ) pcam->center.y + ( pcam->zoom * COS( pcam->turn_z_rad ) );

    camera_adjust_angle( pcam, pcam->pos.z );

    camera_make_matrix( pcam );

    pcam->turn_z_one = ( pcam->turn_z_rad ) / ( TWO_PI );
    pcam->ori.facing_z     = CLIP_TO_16BITS( FLOAT_TO_FP16( pcam->turn_z_one ) );
}

//--------------------------------------------------------------------------------------------
void camera_reset( camera_t * pcam, ego_mpd_t * pmesh )
{
    /// @details ZZ@> This function makes sure the camera starts in a suitable position

    pcam->swing        = 0;
    pcam->pos.x        = pmesh->gmem.edge_x / 2;
    pcam->pos.y        = pmesh->gmem.edge_y / 2;
    pcam->pos.z        = 1500;
    pcam->zoom         = 1000;
    pcam->center.x     = pcam->pos.x;
    pcam->center.y     = pcam->pos.y;
    pcam->track_pos.x  = pcam->pos.x;
    pcam->track_pos.y  = pcam->pos.y;
    pcam->track_pos.z  = 1500;
    pcam->turnadd      = 0;
    pcam->track_level  = 0;
    pcam->zadd         = 1500;
    pcam->zaddgoto     = CAM_ZADD_MAX;
    pcam->zgoto        = 1500;
    pcam->turn_z_rad   = -PI / 4.0f;
    pcam->turn_z_one   = pcam->turn_z_rad / TWO_PI;
    pcam->ori.facing_z = CLIP_TO_16BITS(( int )( pcam->turn_z_one * ( float )0x00010000 ) ) ;
    pcam->turnupdown   = PI / 4.0f;
    pcam->roll         = 0;

    // make sure you are looking at the players
    camera_reset_target( pcam, pmesh );
}

//--------------------------------------------------------------------------------------------
bool_t camera_reset_target( camera_t * pcam, ego_mpd_t * pmesh )
{
    // @details BB@> Force the camera to focus in on the players. Should be called any time there is
    //               a "change of scene". With the new velocity-tracking of the camera, this would include
    //               things like character respawns, adding new players, etc.

    int turn_mode_save;

    if ( NULL == pcam ) return bfalse;

    turn_mode_save = pcam->turn_mode;

    // set an identity matrix.
    pcam->mView = IdentityMatrix();

    // specify the modes that will make the camera point at the players
    pcam->turn_mode = CAM_TURN_AUTO;
    pcam->move_mode = CAM_RESET;

    // If you use CAM_RESET, camera_move() automatically restores pcam->move_mode
    // to its default setting
    camera_move( pcam, pmesh );

    // fix the center position
    pcam->center.x = pcam->track_pos.x;
    pcam->center.y = pcam->track_pos.y;

    // restore the turn mode
    pcam->turn_mode = turn_mode_save;
    pcam->turn_time = 0;

    return btrue;
}

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
void camera_rotmesh_init()
{
    // Matrix init stuff (from remove.c)
    rotmeshtopside    = (( float )sdl_scr.x / sdl_scr.y ) * ROTMESHTOPSIDE / ( 1.33333f );
    rotmeshbottomside = (( float )sdl_scr.x / sdl_scr.y ) * ROTMESHBOTTOMSIDE / ( 1.33333f );
    rotmeshup         = (( float )sdl_scr.x / sdl_scr.y ) * ROTMESHUP / ( 1.33333f );
    rotmeshdown       = (( float )sdl_scr.x / sdl_scr.y ) * ROTMESHDOWN / ( 1.33333f );
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

    pcam->swing     =  0;
    pcam->swingrate =  0;
    pcam->swingamp  =  0;
    pcam->pos.x     =  0;
    pcam->pos.y     =  1500;
    pcam->pos.z     =  1500;
    pcam->zoom      =  1000;
    pcam->zadd      =  800;
    pcam->zaddgoto  =  800;
    pcam->zgoto     =  800;
    pcam->turn_z_rad = -PI / 4.0f;
    pcam->turn_z_one = pcam->turn_z_rad / TWO_PI;
    pcam->facing_z     = (( signed )( pcam->turn_z_one * 0x00010000L ) ) & 0xFFFF;
    pcam->turnadd    =  0;
    pcam->sustain    =  0.60f;
    pcam->turnupdown = ( float )( PI / 4 );
    pcam->roll       =  0;

    pcam->mView       = pcam->mViewSave = ViewMatrix( t1.v, t2.v, t3.v, 0 );
    pcam->mProjection = ProjectionMatrix( .001f, 2000.0f, ( float )( FOV * PI / 180 ) ); // 60 degree FOV
    pcam->mProjection = MatrixMult( Translate( 0, 0, -0.999996f ), pcam->mProjection ); // Fix Z value...
    pcam->mProjection = MatrixMult( ScaleXYZ( -1, -1, 100000 ), pcam->mProjection );  // HUK // ...'cause it needs it

    // [claforte] Fudge the values.
    pcam->mProjection.v[10] /= 2.0f;
    pcam->mProjection.v[11] /= 2.0f;

    camera_rotmesh_init();

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
    if ( pcam->turn_time != 0 )
    {
        pcam->turn_z_rad = ( 1.5f * PI ) - ATAN2( y - pcam->pos.y, x - pcam->pos.x );  // xgg
    }
}

//--------------------------------------------------------------------------------------------
void camera_make_matrix( camera_t * pcam )
{
    /// @details ZZ@> This function sets pcam->mView to the camera's location and rotation

    pcam->mView = MatrixMult( Translate( pcam->pos.x, -pcam->pos.y, pcam->pos.z ), pcam->mViewSave );  // xgg
    if ( pcam->swingamp > 0.001f )
    {
        pcam->roll = turntosin[pcam->swing] * pcam->swingamp;
        pcam->mView = MatrixMult( RotateY( pcam->roll ), pcam->mView );
    }

    pcam->mView = MatrixMult( RotateZ( pcam->turn_z_rad ), pcam->mView );
    pcam->mView = MatrixMult( RotateX( pcam->turnupdown ), pcam->mView );

    //--- pre-compute some camera vectors
    pcam->vfw = mat_getCamForward( pcam->mView );
    pcam->vfw = fvec3_normalize( pcam->vfw.v );

    pcam->vup = mat_getCamUp( pcam->mView );
    pcam->vup = fvec3_normalize( pcam->vup.v );

    pcam->vrt = mat_getCamRight( pcam->mView );
    pcam->vrt = fvec3_normalize( pcam->vrt.v );

}

//--------------------------------------------------------------------------------------------
void camera_adjust_angle( camera_t * pcam, float height )
{
    /// @details ZZ@> This function makes the camera look downwards as it is raised up

    float percentmin, percentmax;
    if ( height < MINZADD )  height = MINZADD;

    percentmax = ( height - MINZADD ) / ( float )( MAXZADD - MINZADD );
    percentmin = 1.0f - percentmax;

    pcam->turnupdown = (( MINUPDOWN * percentmin ) + ( MAXUPDOWN * percentmax ) );
    pcam->zoom = ( MINZOOM * percentmin ) + ( MAXZOOM * percentmax );
}

//--------------------------------------------------------------------------------------------
void camera_move( camera_t * pcam, ego_mpd_t * pmesh )
{
    /// @details ZZ@> This function moves the camera

    Uint16 cnt;
    float x, y, z, level, newx, newy, movex, movey;
    Uint16 turnsin;

    if ( CAMTURN_NONE != pcam->turn_mode )
        pcam->turn_time = 255;
    else if ( pcam->turn_time != 0 )
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
            pcam->turnadd += CAMKEYTURN;
        }

        if ( SDLKEYDOWN( SDLK_KP9 ) )
        {
            pcam->turnadd -= CAMKEYTURN;
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
        fvec3_clear( &sum_pos );

        for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
        {
            chr_t * pchr;

            pchr = pla_get_pchr( ipla );
            if ( NULL == pchr || !pchr->alive ) continue;

            sum_pos.x += pchr->pos.x;
            sum_pos.y += pchr->pos.y;
            sum_pos.z += pchr->pos.z + pchr->chr_chr_cv.maxs[OCT_Z] * 0.9f;
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
            fvec3_clear( &sum_pos );

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
    if ( pcam->turn_mode == CAMTURN_GOOD && local_numlpla == 1 )
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
            pcam->turnadd += ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) ) * ( CAMKEYTURN );
        }

        if ( joy[0].on )
        {
            if ( !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )
            {
                pcam->turnadd -= joy[0].x * CAMJOYTURN;
            }
        }

        if ( joy[1].on )
        {
            if ( !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )
            {
                pcam->turnadd -= joy[1].x * CAMJOYTURN;
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
                if ( pcam->zaddgoto < MINZADD )  pcam->zaddgoto = MINZADD;
                if ( pcam->zaddgoto > MAXZADD )  pcam->zaddgoto = MAXZADD;

                pcam->turn_time = TURNTIME;  // Sticky turn...
            }
        }

        // JoyA camera controls
        if ( joy[0].on )
        {
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )
            {
                pcam->turnadd += joy[0].x * CAMJOYTURN;
                pcam->zaddgoto += joy[0].y * CAMJOYTURN;
                if ( pcam->zaddgoto < MINZADD )  pcam->zaddgoto = MINZADD;
                if ( pcam->zaddgoto > MAXZADD )  pcam->zaddgoto = MAXZADD;

                pcam->turn_time = TURNTIME;  // Sticky turn...
            }
        }

        // JoyB camera controls
        if ( joy[1].on )
        {
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )
            {
                pcam->turnadd += joy[1].x * CAMJOYTURN;
                pcam->zaddgoto += joy[1].y * CAMJOYTURN;
                if ( pcam->zaddgoto < MINZADD )  pcam->zaddgoto = MINZADD;
                if ( pcam->zaddgoto > MAXZADD )  pcam->zaddgoto = MAXZADD;

                pcam->turn_time = TURNTIME;  // Sticky turn...
            }
        }
    }

    // Keyboard camera controls
    if ( keyb.on )
    {
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_LEFT ) || control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_RIGHT ) )
        {
            pcam->turnadd += ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_LEFT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_RIGHT ) ) * CAMKEYTURN;
            pcam->turn_time = TURNTIME;  // Sticky turn...
        }

        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_IN ) || control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_OUT ) )
        {
            pcam->zaddgoto += ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_OUT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_CAMERA_IN ) ) * CAMKEYTURN;
            if ( pcam->zaddgoto < MINZADD )  pcam->zaddgoto = MINZADD;
            if ( pcam->zaddgoto > MAXZADD )  pcam->zaddgoto = MAXZADD;
        }
    }

    pcam->pos.x -= ( float )( pcam->mView.CNV( 0, 0 ) ) * pcam->turnadd; // xgg
    pcam->pos.y += ( float )( pcam->mView.CNV( 1, 0 ) ) * -pcam->turnadd;

    // Center on target for doing rotation...
    if ( pcam->turn_time != 0 )
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
    z = ( TRACKXAREALOW  * ( MAXZADD - pcam->zadd ) ) +
        ( TRACKXAREAHIGH * ( pcam->zadd - MINZADD ) );
    z = z / ( MAXZADD - MINZADD );
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
    z = ( TRACKYAREAMINLOW  * ( MAXZADD - pcam->zadd ) ) +
        ( TRACKYAREAMINHIGH * ( pcam->zadd - MINZADD ) );
    z = z / ( MAXZADD - MINZADD );
    if ( newy < z )
    {
        // Scroll down
        movey -= ( newy - z );
    }
    else
    {
        // Adjust for camera height...
        z = ( TRACKYAREAMAXLOW  * ( MAXZADD - pcam->zadd ) ) +
            ( TRACKYAREAMAXHIGH * ( pcam->zadd - MINZADD ) );
        z = z / ( MAXZADD - MINZADD );
        if ( newy > z )
        {
            // Scroll up
            movey -= ( newy - z );
        }
    }

    turnsin = TO_TURN(pcam->facing_z);
    pcam->center.x += movex * turntocos[ turnsin & TRIG_TABLE_MASK ] + movey * turntosin[ turnsin & TRIG_TABLE_MASK ];
    pcam->center.y += -movex * turntosin[ turnsin & TRIG_TABLE_MASK ] + movey * turntocos[ turnsin & TRIG_TABLE_MASK ];

    // Finish up the camera
    camera_look_at( pcam, pcam->center.x, pcam->center.y );
    pcam->pos.x = ( float ) pcam->center.x + ( pcam->zoom * SIN( pcam->turn_z_rad ) );
    pcam->pos.y = ( float ) pcam->center.y + ( pcam->zoom * COS( pcam->turn_z_rad ) );

    camera_adjust_angle( pcam, pcam->pos.z );

    camera_make_matrix( pcam );

    pcam->turn_z_one = ( pcam->turn_z_rad ) / ( TWO_PI );
    pcam->facing_z     = CLIP_TO_16BITS( FLOAT_TO_FP16( pcam->turn_z_one ) );
}

//--------------------------------------------------------------------------------------------
void camera_reset( camera_t * pcam, ego_mpd_t * pmesh )
{
    /// @details ZZ@> This function makes sure the camera starts in a suitable position

    pcam->swing = 0;
    pcam->pos.x = pmesh->gmem.edge_x / 2;
    pcam->pos.y = pmesh->gmem.edge_y / 2;
    pcam->pos.z = 1500;
    pcam->zoom = 1000;
    pcam->center.x = pcam->pos.x;
    pcam->center.y = pcam->pos.y;
    pcam->track_pos.x = pcam->pos.x;
    pcam->track_pos.y = pcam->pos.y;
    pcam->track_pos.z = 1500;
    pcam->turnadd = 0;
    pcam->track_level = 0;
    pcam->zadd = 1500;
    pcam->zaddgoto = MAXZADD;
    pcam->zgoto = 1500;
    pcam->turn_z_rad = -PI / 4.0f;
    pcam->turn_z_one = pcam->turn_z_rad / TWO_PI;
    pcam->facing_z     = (( signed )( pcam->turn_z_one * 0x00010000L ) ) & 0xFFFF;
    pcam->turnupdown = PI / 4.0f;
    pcam->roll = 0;

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
    pcam->turn_mode = CAMTURN_AUTO;
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

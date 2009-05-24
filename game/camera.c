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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - camera.c
 * Various functions related to how the game camera works.
 */

#include "camera.h"

#include "input.h"
#include "char.h"
#include "graphic.h"
#include "network.h"

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

camera_t gCamera;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Camera control stuff

camera_t * camera_new( camera_t * pcam )
{
    GLvector3 t1 = {0, 0, 0};
    GLvector3 t2 = {0, 0, -1};
    GLvector3 t3 = {0, 1, 0};

    memset( pcam, 0, sizeof(camera_t) );

    pcam->move_mode = CAM_PLAYER;
    pcam->turn_mode = autoturncamera;

    pcam->swing     =  0;
    pcam->swingrate =  0;
    pcam->swingamp  =  0;
    pcam->x         =  0;
    pcam->y         =  1500;
    pcam->z         =  1500;
    pcam->zoom      =  1000;
    pcam->zadd     =  800;
    pcam->zaddgoto =  800;
    pcam->zgoto    =  800;
    pcam->turnleftright      =  ( float )( -PI / 4 );
    pcam->turnleftrightone   =  ( float )( -PI / 4 ) / ( TWO_PI );
    pcam->turnleftrightshort =  0;
    pcam->turnadd            =  0;
    pcam->sustain            =  0.60f;
    pcam->turnupdown         =  ( float )( PI / 4 );
    pcam->roll               =  0;

    pcam->mView       = pcam->mViewSave = ViewMatrix( t1, t2, t3, 0 );
    pcam->mProjection = ProjectionMatrix( .001f, 2000.0f, ( float )( FOV * PI / 180 ) ); // 60 degree FOV
    pcam->mProjection = MatrixMult( Translate( 0, 0, -0.999996f ), pcam->mProjection ); // Fix Z value...
    pcam->mProjection = MatrixMult( ScaleXYZ( -1, -1, 100000 ), pcam->mProjection );  // HUK // ...'cause it needs it

    //[claforte] Fudge the values.
    pcam->mProjection.v[10] /= 2.0f;
    pcam->mProjection.v[11] /= 2.0f;

    return pcam;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void dump_matrix( GLmatrix a )
{
    int i; int j;

    for ( j = 0; j < 4; j++ )
    {
        printf( "  " );

        for ( i = 0; i < 4; i++ )
            printf( "%f ", a.CNV( i, j ) );

        printf( "\n" );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void camera_look_at( camera_t * pcam, float x, float y )
{
    // ZZ> This function makes the camera turn to face the character
    pcam->zgoto = pcam->zadd;
    if ( pcam->turn_time != 0 )
    {
        pcam->turnleftright = ( 1.5f * PI ) - ATAN2( y - pcam->y, x - pcam->x );  // xgg
    }
}

//--------------------------------------------------------------------------------------------
void camera_make_matrix( camera_t * pcam )
{
    // ZZ> This function sets pcam->mView to the camera's location and rotation

    pcam->mView = MatrixMult( Translate( pcam->x, -pcam->y, pcam->z ), pcam->mViewSave );  // xgg
    if ( pcam->swingamp > 0.001f )
    {
        pcam->roll = turntosin[pcam->swing] * pcam->swingamp;
        pcam->mView = MatrixMult( RotateY( pcam->roll ), pcam->mView );
    }

    pcam->mView = MatrixMult( RotateZ( pcam->turnleftright ), pcam->mView );
    pcam->mView = MatrixMult( RotateX( pcam->turnupdown ), pcam->mView );

}

//--------------------------------------------------------------------------------------------
void camera_adjust_angle( camera_t * pcam, float height )
{
    // ZZ> This function makes the camera look downwards as it is raised up
    float percentmin, percentmax;
    if ( height < MINZADD )  height = MINZADD;

    percentmax = ( height - MINZADD ) / ( float )( MAXZADD - MINZADD );
    percentmin = 1.0f - percentmax;

    pcam->turnupdown = ( ( MINUPDOWN * percentmin ) + ( MAXUPDOWN * percentmax ) );
    pcam->zoom = ( MINZOOM * percentmin ) + ( MAXZOOM * percentmax );
}

//--------------------------------------------------------------------------------------------
void camera_move( camera_t * pcam )
{
    // ZZ> This function moves the camera
    Uint16 cnt;
    Sint16 locoalive;
    float x, y, z, level, newx, newy, movex, movey;
    Uint16 character, turnsin, turncos;

    if ( pcam->turn_mode )
        pcam->turn_time = 255;
    else if ( pcam->turn_time != 0 )
        pcam->turn_time--;

    if ( CAM_FREE == pcam->move_mode )
    {
        // the keypad controls the camera
        if ( SDLKEYDOWN( SDLK_KP8 ) )
        {
            pcam->trackx -= pcam->mView.CNV( 0, 1 ) * 50;
            pcam->tracky -= pcam->mView.CNV( 1, 1 ) * 50;
        }

        if ( SDLKEYDOWN( SDLK_KP2 ) )
        {
            pcam->trackx += pcam->mView.CNV( 0, 1 ) * 50;
            pcam->tracky += pcam->mView.CNV( 1, 1 ) * 50;
        }

        if ( SDLKEYDOWN( SDLK_KP4 ) )
        {
            pcam->trackx += pcam->mView.CNV( 0, 0 ) * 50;
            pcam->tracky += pcam->mView.CNV( 1, 0 ) * 50;
        }

        if ( SDLKEYDOWN( SDLK_KP6 ) )
        {
            pcam->trackx -= pcam->mView.CNV( 0, 0 ) * 10;
            pcam->tracky -= pcam->mView.CNV( 1, 0 ) * 10;
        }

        if ( SDLKEYDOWN( SDLK_KP7 ) )
        {
            pcam->turnadd += CAMKEYTURN;
        }

        if ( SDLKEYDOWN( SDLK_KP9 ) )
        {
            pcam->turnadd -= CAMKEYTURN;
        }

        pcam->trackz = 128 + get_level( PMesh, pcam->trackx, pcam->tracky, bfalse );
    }

    if ( CAM_PLAYER == pcam->move_mode )
    {
        x = 0;
        y = 0;
        z = 0;
        level = 0;
        locoalive = 0;
        for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
        {
            if ( PlaList[cnt].valid && PlaList[cnt].device != INPUT_BITS_NONE )
            {
                character = PlaList[cnt].index;
                if ( ChrList[character].alive )
                {
                    if ( ChrList[character].attachedto == MAX_CHR )
                    {
                        // The character is on foot
                        x += ChrList[character].pos.x;
                        y += ChrList[character].pos.y;
                        z += ChrList[character].pos.z;
                        level += ChrList[character].phys_level;
                    }
                    else
                    {
                        // The character is mounted
                        x += ChrList[ChrList[character].attachedto].pos.x;
                        y += ChrList[ChrList[character].attachedto].pos.y;
                        z += ChrList[ChrList[character].attachedto].pos.z;
                        level += ChrList[ChrList[character].attachedto].phys_level;
                    }

                    locoalive++;
                }
            }
        }
        if ( locoalive > 0 )
        {
            x = x / locoalive;
            y = y / locoalive;
            z = z / locoalive;
            level = level / locoalive;
        }
        else
        {
            x = pcam->trackx;
            y = pcam->tracky;
            z = pcam->trackz;
        }
    }
    else
    {
        x = pcam->trackx;
        y = pcam->tracky;
        z = pcam->trackz;

        level = 128 + get_level(PMesh, x, y, bfalse);
    }

    pcam->trackxvel = -pcam->trackx;
    pcam->trackyvel = -pcam->tracky;
    pcam->trackzvel = -pcam->trackz;
    pcam->trackx = ( pcam->trackx + x ) / 2.0f;
    pcam->tracky = ( pcam->tracky + y ) / 2.0f;
    pcam->trackz = ( pcam->trackz + z ) / 2.0f;
    pcam->tracklevel = ( pcam->tracklevel + level ) / 2.0f;

    pcam->turnadd = pcam->turnadd * pcam->sustain;
    pcam->zadd = ( pcam->zadd * 3.0f + pcam->zaddgoto ) / 4.0f;
    pcam->z = ( pcam->z * 3.0f + pcam->zgoto ) / 4.0f;

    // Camera controls
    if ( pcam->turn_mode == 255 && local_numlpla == 1 )
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

    pcam->x -= ( float ) ( pcam->mView.CNV( 0, 0 ) ) * pcam->turnadd;  // xgg
    pcam->y += ( float ) ( pcam->mView.CNV( 1, 0 ) ) * -pcam->turnadd;

    // Do distance effects for overlay and background
    pcam->trackxvel += pcam->trackx;
    pcam->trackyvel += pcam->tracky;
    pcam->trackzvel += pcam->trackz;
    //if ( draw_water_0 )
    //{
    //    // Do fg distance effect
    //    waterlayeru[0] += pcam->trackxvel * waterlayerdistx[0];
    //    waterlayerv[0] += pcam->trackyvel * waterlayerdisty[0];
    //}
    //if ( draw_water_1 )
    //{
    //    // Do bg distance effect
    //    waterlayeru[1] += pcam->trackxvel * waterlayerdistx[1];
    //    waterlayerv[1] += pcam->trackyvel * waterlayerdisty[1];
    //}

    // Center on target for doing rotation...
    if ( pcam->turn_time != 0 )
    {
        pcam->centerx = pcam->centerx * 0.9f + pcam->trackx * 0.1f;
        pcam->centery = pcam->centery * 0.9f + pcam->tracky * 0.1f;
    }

    // Create a tolerance area for walking without camera movement
    x = pcam->trackx - pcam->x;
    y = pcam->tracky - pcam->y;
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

    turnsin = (Uint16)( pcam->turnleftrightone * TRIG_TABLE_SIZE ) & TRIG_TABLE_MASK;
    turncos = ( turnsin + TRIG_TABLE_OFFSET ) & TRIG_TABLE_MASK;
    pcam->centerx += ( movex * turntocos[turnsin] + movey * turntosin[turnsin] );
    pcam->centery += ( -movex * turntosin[turnsin] + movey * turntocos[turnsin] );

    // Finish up the camera
    camera_look_at( pcam, pcam->centerx, pcam->centery );
    pcam->x = ( float ) pcam->centerx + ( pcam->zoom * SIN( pcam->turnleftright ) );
    pcam->y = ( float ) pcam->centery + ( pcam->zoom * COS( pcam->turnleftright ) );

    camera_adjust_angle( pcam, pcam->z );

    camera_make_matrix( pcam );
}

//--------------------------------------------------------------------------------------------
void camera_reset( camera_t * pcam )
{
    // ZZ> This function makes sure the camera starts in a suitable position
    int cnt;

    pcam->swing = 0;
    pcam->x = PMesh->info.edge_x / 2;
    pcam->y = PMesh->info.edge_y / 2;
    pcam->z = 1500;
    pcam->zoom = 1000;
    pcam->trackxvel = 0;
    pcam->trackyvel = 0;
    pcam->trackzvel = 1500;
    pcam->centerx = pcam->x;
    pcam->centery = pcam->y;
    pcam->trackx = pcam->x;
    pcam->tracky = pcam->y;
    pcam->trackz = 1500;
    pcam->turnadd = 0;
    pcam->tracklevel = 0;
    pcam->zadd = 1500;
    pcam->zaddgoto = MAXZADD;
    pcam->zgoto = 1500;
    pcam->turnleftright = ( float ) ( -PI / 4 );
    pcam->turnleftrightone = ( float ) ( -PI / 4 ) / ( TWO_PI );
    pcam->turnleftrightshort = 0;
    pcam->turnupdown = ( float ) ( PI / 4 );
    pcam->roll = 0;

    // Now move the camera towards the players
    {
        Uint8 move_mode_save = pcam->move_mode;
        Uint8 turn_mode_save = pcam->turn_mode;

        pcam->mView = IdentityMatrix();

        pcam->turn_mode = 1;
        pcam->move_mode = CAM_PLAYER;

        for ( cnt = 0; cnt < 32; cnt++ )
        {
            camera_move( pcam );
            pcam->centerx = pcam->trackx;
            pcam->centery = pcam->tracky;
        }

        pcam->move_mode = move_mode_save;
        pcam->turn_mode = turn_mode_save;
        pcam->turn_time = 0;
    }

}


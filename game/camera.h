#pragma once

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

/// @file camera.h

#include "egoboo_typedef.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_ego_mpd;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// The camera mode
enum e_camera_mode
{
    CAM_PLAYER = 0,
    CAM_FREE,
    CAM_RESET
};

/// The mode that the camera uses to determine where is is looking
enum e_camera_turn_mode
{
    CAMTURN_NONE = ( 1 == 0 ),  ///< false
    CAMTURN_AUTO = ( 1 == 1 ),  ///< true
    CAMTURN_GOOD = 255
};

#define TRACKXAREALOW     100
#define TRACKXAREAHIGH    180
#define TRACKYAREAMINLOW  320
#define TRACKYAREAMAXLOW  460
#define TRACKYAREAMINHIGH 460
#define TRACKYAREAMAXHIGH 600

#define FOV                             60           ///< Field of view
#define CAMJOYTURN                      (0.01f*3)    ///< Joystick camera rotation
#define CAMKEYTURN                      (10*3)       ///< Keyboard camera rotation
#define FARTRACK                        1200         ///< For outside modules...
#define EDGETRACK                       800          ///< Camtrack bounds
#define TURNTIME                        16           ///< Smooth turn

/// Multi cam (uses macro to switch between old and new camera
#ifndef OLD_CAMERA_MODE
#    define MINZOOM                         800         ///< Camera distance
#    define MAXZOOM                         700
#    define MINZADD                         800         ///< Camera height
#    define MAXZADD                         2500
#    define MINUPDOWN                       (0.24f*PI)    ///< Camera updown angle
#    define MAXUPDOWN                       (0.10f*PI)
#else
#    define MINZOOM                         500         ///< Camera distance
#    define MAXZOOM                         600
#    define MINZADD                         800         ///< Camera height
#    define MAXZADD                         1500  ///< 1000
#    define MINUPDOWN                       (0.24f*PI)    ///< Camera updown angle
#    define MAXUPDOWN                       (0.18f*PI)// (0.15f*PI) ///< (0.18f*PI)
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// definition of the egoboo camera object
struct s_camera
{
    fmat_4x4_t mView, mViewSave;      ///< View Matrix
    fmat_4x4_t mProjection;           ///< Projection Matrix

    Uint8  move_mode;               ///< what is the camera mode
    Uint8  move_mode_old;           ///< the default movement mode
    Uint8  turn_mode;               ///< what is the camera mode
    Uint8  turn_time;               ///< time for the smooth turn

    int       swing;                   ///< Camera swingin'
    int       swingrate;
    float     swingamp;
    fvec3_t   pos;                       ///< Camera position (z = 500-1000)
    float     zoom;                    ///< Distance from the trackee
    fvec3_t   track_pos;                  ///< Trackee position
    float     track_level;
    fvec3_t   center;                 ///< Move character to side before tracking
    float     zadd;                    ///< Camera height above terrain
    float     zaddgoto;                ///< Desired z position
    float     zgoto;
    float     turn_z_rad;           ///< Camera rotations
    float     turn_z_one;
    Uint16    turn_z;
    float     turnadd;                 ///< Turning rate
    float     sustain;                 ///< Turning rate falloff
    float     turnupdown;
    float     roll;

    fvec3_t   vfw;                 ///< the camera forward vector
    fvec3_t   vup;                 ///< the camera up vector
    fvec3_t   vrt;                 ///< the camera right vector
};

typedef struct s_camera camera_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern camera_t gCamera;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Function prototypes
camera_t * camera_new( camera_t * pcam );

void camera_reset( camera_t * pcam, struct s_ego_mpd * pmesh );
void camera_adjust_angle( camera_t * pcam, float height );
void camera_move( camera_t * pcam, struct s_ego_mpd * pmesh );
void camera_make_matrix( camera_t * pcam );
void camera_look_at( camera_t * pcam, float x, float y );

bool_t camera_reset_target( camera_t * pcam, struct s_ego_mpd * pmesh );
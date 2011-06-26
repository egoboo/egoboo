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

#include "physics.h"

#include "egoboo_math.h"
#include "egoboo_frustum.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_input_device;
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
    CAM_TURN_NONE = 0,
    CAM_TURN_AUTO = 1,
    CAM_TURN_GOOD = 255
};

#define CAM_TRACK_X_AREA_LOW     100
#define CAM_TRACK_X_AREA_HIGH    180
#define CAM_TRACK_Y_AREA_MINLOW  320
#define CAM_TRACK_Y_AREA_MAXLOW  460
#define CAM_TRACK_Y_AREA_MINHIGH 460
#define CAM_TRACK_Y_AREA_MAXHIGH 600

#define CAM_FOV                             60          ///< Field of view
#define CAM_TURN_JOY              (3.0f * 5.0f)         ///< Joystick camera rotation
#define CAM_TURN_KEY               CAM_TURN_JOY         ///< Keyboard camera rotation
#define CAM_TURN_TIME                       16          ///< Smooth turn
#define CAM_TRACK_FAR                     1200          ///< For outside modules...
#define CAM_TRACK_EDGE                     800          ///< Camtrack bounds

// Multi cam (uses macro to switch between old and new camera
#define ZOOM_FACTOR                              0.5f

#if !defined(OLD_CAMERA_MODE)
#    define CAM_ZOOM_MIN                         (800 * ZOOM_FACTOR)         ///< Camera distance
#    define CAM_ZOOM_MAX                         (700 * ZOOM_FACTOR)
#    define CAM_ZADD_MIN                         (800 * ZOOM_FACTOR)         ///< Camera height
#    define CAM_ZADD_MAX                         (2750 * ZOOM_FACTOR)
#    define CAM_UPDOWN_MIN                       (0.24f*PI)    ///< Camera updown angle
#    define CAM_UPDOWN_MAX                       (0.10f*PI)
#    define CAM_ZADD_AVG                         (0.5f * (CAM_ZADD_MIN + CAM_ZADD_MAX))
#    define CAM_ZOOM_AVG                         (0.5f * (CAM_ZOOM_MIN + CAM_ZOOM_MAX))
#else
#    define CAM_ZOOM_MIN                         (500 * ZOOM_FACTOR)         ///< Camera distance
#    define CAM_ZOOM_MAX                         (600 * ZOOM_FACTOR)
#    define CAM_ZADD_MIN                         (800 * ZOOM_FACTOR)         ///< Camera height
#    define CAM_ZADD_MAX                         (1500 * ZOOM_FACTOR)  ///< 1000
#    define CAM_UPDOWN_MIN                       (0.24f*PI)    ///< Camera updown angle
#    define CAM_UPDOWN_MAX                       (0.18f*PI)// (0.15f*PI) ///< (0.18f*PI)
#endif

#define CAM_ZADD_AVG                         (0.5f * (CAM_ZADD_MIN + CAM_ZADD_MAX))
#define CAM_ZOOM_AVG                         (0.5f * (CAM_ZOOM_MIN + CAM_ZOOM_MAX))

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_camera_options
{
    int           swing;                   ///< Camera swing angle
    int           swing_rate;              ///< Camera swing rate
    float         swing_amp;               ///< Camera swing amplitude

    Uint8         turn_mode;               ///< what is the camera mode
};

typedef struct s_camera_options camera_options_t;

extern camera_options_t cam_options;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// definition of the Egoboo camera object
struct s_camera
{
    // the projection matrices
    fmat_4x4_t mView;                        ///< View Matrix

    fmat_4x4_t mProjection;                  ///< normal Projection Matrix
    fmat_4x4_t mProjection_big;
    fmat_4x4_t mProjection_small;

    // the view frustum
    ego_frustum_t frustum;
    ego_frustum_t frustum_big;
    ego_frustum_t frustum_small;

    // how do we calculate the motion
    Uint8  move_mode;               ///< what is the camera mode
    Uint8  move_mode_old;           ///< the default movement mode

    // how do we calculate the turning?
    Uint8  turn_mode;               ///< what is the camera mode
    Uint8  turn_time;               ///< time for the smooth turn

    // the actual camera position
    fvec3_t       pos;                       ///< Camera position (z = 500-1000)
    orientation_t ori;

    // the middle of the objects that are being tracked
    fvec3_t       track_pos;               ///< Trackee position
    float         track_level;

    // the position that the camera is focused on
    float         zoom;                    ///< Distance from the center
    fvec3_t       center;                 ///< Move character to side before tracking

    // camera z motion
    float         zadd;                    ///< Camera height above terrain
    float         zadd_goto;                ///< Desired z position
    float         zgoto;

    // turning
    float         turn_z_rad;           ///< Camera z rotation (radians)
    float         turn_z_one;           ///< Camera z rotation (from 0.0f to 1.0f)
    float         turn_z_add;           ///< Turning rate
    float         turn_z_sustain;       ///< Turning rate falloff

    // billboard info
    fvec3_t       vfw;                 ///< the camera forward vector
    fvec3_t       vup;                 ///< the camera up vector
    fvec3_t       vrt;                 ///< the camera right vector

    // effects
    float         motion_blur;      ///< Blurry effect
    int           swing;                   ///< Camera swingin'
    int           swing_rate;
    float         swing_amp;
    float         roll;
};

typedef struct s_camera camera_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes

camera_t * camera_ctor( camera_t * pcam );

void   camera_read_input( camera_t *pcam, struct s_input_device *pdevice );
void   camera_make_matrix( camera_t * pcam );

void   camera_move( camera_t * pcam, const struct s_ego_mpd * pmesh, const CHR_REF track_list[], const size_t track_list_size );
void   camera_reset( camera_t * pcam, const struct s_ego_mpd * pmesh, const CHR_REF track_list[], const size_t track_list_size );
bool_t camera_reset_target( camera_t * pcam, const struct s_ego_mpd * pmesh, const CHR_REF track_list[], const size_t track_list_size );

bool_t camera_reset_view( camera_t * pcam );
bool_t camera_reset_projection( camera_t * pcam , float fov_deg, float ar );

void camera_gluPerspective( camera_t * pcam, float fovy_deg, float aspect_ratio, float frustum_near, float frustum_far );
void camera_gluLookAt( camera_t * pcam, float roll_deg );
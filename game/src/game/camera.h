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

/// @file game/camera.h

#pragma once

#include "game/egoboo_typedef.h"
#include "game/physics.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_input_device;
struct ego_mesh_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct camera_options_t;
struct camera_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The camera mode.
enum e_camera_mode
{
    CAM_PLAYER = 0,
    CAM_FREE,
    CAM_RESET
};

/// Multi cam uses macro to switch between old and new camera
#define CAM_ZOOM_FACTOR 0.5f

#if !defined(OLD_CAMERA_MODE)
	#define CAM_ZOOM_MIN                         (800 * CAM_ZOOM_FACTOR)       ///< Camera distance
	#define CAM_ZOOM_MAX                         (700 * CAM_ZOOM_FACTOR)
	#define CAM_ZADD_MIN                         (800 * CAM_ZOOM_FACTOR)       ///< Camera height
	#define CAM_ZADD_MAX                         (2750 * CAM_ZOOM_FACTOR)
#else
	#define CAM_ZOOM_MIN                         (500 * CAM_ZOOM_FACTOR)       ///< Camera distance
	#define CAM_ZOOM_MAX                         (600 * CAM_ZOOM_FACTOR)
	#define CAM_ZADD_MIN                         (800 * CAM_ZOOM_FACTOR)       ///< Camera height
	#define CAM_ZADD_MAX                         (1500 * CAM_ZOOM_FACTOR)      ///< 1000
#endif

#define CAM_ZADD_AVG                         (0.5f * (CAM_ZADD_MIN + CAM_ZADD_MAX))
#define CAM_ZOOM_AVG                         (0.5f * (CAM_ZOOM_MIN + CAM_ZOOM_MAX))

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct camera_options_t
{
    int           swing;                   ///< Camera swing angle
    int           swing_rate;              ///< Camera swing rate
    float         swing_amp;               ///< Camera swing amplitude

	e_camera_turn_mode turn_mode;               ///< what is the camera turn mode
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// definition of the Egoboo camera object
struct camera_t
{

	/// The default field of view angle (in degrees).
	static const float DEFAULT_FOV;

	/// The default joystick turn rotation.
	/// @todo What unit is that?
	static const float DEFAULT_TURN_JOY;

	/// The default keyboard turn rotation.
	/// @todo What unit is that?
	static const float DEFAULT_TURN_KEY;

	/// The default smooth turn rotation.
	/// @todo What unit is that?
	static const Uint8 DEFAULT_TURN_TIME;

    // the projection matrices
    fmat_4x4_t mView;                        ///< view matrix (derived/cached from other attributes)

    fmat_4x4_t mProjection;                  ///< normal projection matrix (derived/cached from other attributes)
    fmat_4x4_t mProjection_big;              ///< big    projection matrix (derived/cached from other attributes)
    fmat_4x4_t mProjection_small;            ///< small  projection matrix (derived/cached from other attributes)

    // the view frustum
    egolib_frustum_t frustum;
    egolib_frustum_t frustum_big;
    egolib_frustum_t frustum_small;

    // how do we calculate the motion
    Uint8  move_mode;               ///< what is the camera mode
    Uint8  move_mode_old;           ///< the default movement mode

    // how do we calculate the turning?
	e_camera_turn_mode turn_mode;   ///< what is the camera turn mode
    Uint8  turn_time;               ///< time for the smooth turn

    // the actual camera position
    fvec3_t       pos; ///< @brief The camera position.
	                   ///< @inv @a z must be within the interval <tt>[500,1000]</tt>.
    orientation_t ori; ///< @brief The camera orientation.

    // the middle of the objects that are being tracked
    fvec3_t       track_pos;               ///< Trackee position
    float         track_level;

    // the position that the camera is focused on
    float         zoom;                    ///< Distance from the center
    fvec3_t       center;                  ///< Move character to side before tracking

    // camera z motion
    float         zadd;                    ///< Camera height above terrain
    float         zadd_goto;               ///< Desired z position
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
    float         motion_blur;         ///< Blurry effect
    float         motion_blur_old;     ///< Blurry effect one frame ago (used to init the accum buffer)
    int           swing;               ///< Camera swingin'
    int           swing_rate;
    float         swing_amp;
    float         roll;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern camera_options_t cam_options;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes

camera_t *camera_ctor(camera_t *cam);
// @todo Add camera_dtor.

void camera_read_input( camera_t *pcam, struct s_input_device *pdevice );
void camera_make_matrix( camera_t * pcam );

void camera_move( camera_t * pcam, const ego_mesh_t * pmesh, const CHR_REF track_list[], const size_t track_list_size );
void camera_reset( camera_t * pcam, const ego_mesh_t * pmesh, const CHR_REF track_list[], const size_t track_list_size );
bool camera_reset_target( camera_t * pcam, const ego_mesh_t * pmesh, const CHR_REF track_list[], const size_t track_list_size );

bool camera_reset_view( camera_t * pcam );
bool camera_reset_projection( camera_t * pcam , float fov_deg, float ar );

void camera_gluPerspective( camera_t * pcam, float fovy_deg, float aspect_ratio, float frustum_near, float frustum_far );
void camera_gluLookAt( camera_t * pcam, float roll_deg );

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

#pragma once

#include <cstdint>
#include <forward_list>
#include "game/egoboo_typedef.h"

/// The camera mode.
enum CameraMode : uint8_t
{
    CAM_PLAYER,
    CAM_FREE,
    CAM_RESET
};

/// The mode that the camera uses to determine where is is looking.
enum CameraTurnMode : uint8_t
{
	CAM_TURN_NONE = 0,
	CAM_TURN_AUTO = 1,
	CAM_TURN_GOOD = 255
};

class Camera
{
public:
	Camera();

	void resetView();

	void resetProjection();

    /// @details This function moves the camera
	void update(const ego_mesh_t * pmesh, std::forward_list<CHR_REF> &trackList);

    /// @details This function makes sure the camera starts in a suitable position
	void reset(const ego_mesh_t * pmesh, const std::forward_list<CHR_REF> &trackList);

protected:

	/// The default field of view angle (in degrees).
	static constexpr float DEFAULT_FOV = 60.0f;

	/// The default joystick turn rotation.
	/// @todo What unit is that?
	static constexpr float DEFAULT_TURN_JOY= 64;

	/// The default keyboard turn rotation.
	/// @todo What unit is that?
	static constexpr float DEFAULT_TURN_KEY = DEFAULT_TURN_JOY;

	/// The default smooth turn rotation.
	/// @todo What unit is that?
	static constexpr uint8_t DEFAULT_TURN_TIME = 16;

	/// Multi cam uses macro to switch between old and new camera
	static constexpr float CAM_ZOOM_FACTOR = 0.5f
	#ifdef OLD_CAMERA_MODE
		static constexpr float CAM_ZOOM_MIN = (800 * CAM_ZOOM_FACTOR);       ///< Camera distance
		static constexpr float CAM_ZOOM_MAX = (700 * CAM_ZOOM_FACTOR);
		static constexpr float CAM_ZADD_MIN = (800 * CAM_ZOOM_FACTOR);       ///< Camera height
		static constexpr float CAM_ZADD_MAX = (2750 * CAM_ZOOM_FACTOR);
	#else
		static constexpr float CAM_ZOOM_MIN = (500 * CAM_ZOOM_FACTOR);       ///< Camera distance
		static constexpr float CAM_ZOOM_MAX = (600 * CAM_ZOOM_FACTOR);
		static constexpr float CAM_ZADD_MIN = (800 * CAM_ZOOM_FACTOR);       ///< Camera height
		static constexpr float CAM_ZADD_MAX = (1500 * CAM_ZOOM_FACTOR);
	#endif

	static constexpr float CAM_ZADD_AVG = (0.5f * (CAM_ZADD_MIN + CAM_ZADD_MAX));
	static constexpr float CAM_ZOOM_AVG = (0.5f * (CAM_ZOOM_MIN + CAM_ZOOM_MAX));

private:
    /// @details This function makes the camera turn to face the character
	void updatePosition();

	void updateCenter();

	void updateTrack(const ego_mesh_t * pmesh, const std::forward_list<CHR_REF> &trackList);

    /// @details Create a default list of objects that are tracked
	std::forward_list<CHR_REF> createTrackList();

	void updateEffects();

    /// @details This function makes the camera look downwards as it is raised up
	void updateZoom();

    /// @details This function sets pcam->mView to the camera's location and rotation
	void makeMatrix();

    /// @details Read camera control input for one specific player controller
	void readInput(input_device_t *pdevice);

    /// @details Force the camera to focus in on the players. Should be called any time there is
    ///          a "change of scene". With the new velocity-tracking of the camera, this would include
    ///          things like character respawns, adding new players, etc.
	bool resetTarget( const ego_mesh_t * pmesh, const std::forward_list<CHR_REF> &trackList );

private:
   // the projection matrices
    fmat_4x4_t _mView;                        ///< view matrix (derived/cached from other attributes)

    fmat_4x4_t _mProjection;                  ///< normal projection matrix (derived/cached from other attributes)
    fmat_4x4_t _mProjection_big;              ///< big    projection matrix (derived/cached from other attributes)
    fmat_4x4_t _mProjectionSmall;             ///< small  projection matrix (derived/cached from other attributes)

    // the view frustum
    egolib_frustum_t _frustum;
    egolib_frustum_t _frustumBig;
    egolib_frustum_t _frustumSmall;

    // how do we calculate the motion
    CameraMode  _moveMode;               ///< what is the camera mode

    // how do we calculate the turning?
	CameraTurnMode _turnMode;         ///< what is the camera turn mode
    uint8_t  	   _turnTime;         ///< time for the smooth turn

    // the actual camera position
    fvec3_t       _pos; ///< @brief The camera position.
	                   ///< @inv @a z must be within the interval <tt>[500,1000]</tt>.
    orientation_t _ori; ///< @brief The camera orientation.

    // the middle of the objects that are being tracked
    fvec3_t       _trackPos;               ///< Trackee position
    float         _trackLevel;

    // the position that the camera is focused on
    float         _zoom;                   ///< Distance from the center
    fvec3_t       _center;                 ///< Move character to side before tracking

    // camera z motion
    float         _zadd;                    ///< Camera height above terrain
    float         _zadd_goto;               ///< Desired z position
    float         _zgoto;

    // turning
    float         _turnZRad;           ///< Camera z rotation (radians)
    float         _turnZOne;           ///< Camera z rotation (from 0.0f to 1.0f)
    float         _turnZAdd;           ///< Turning rate
    float         _turnZSustain;       ///< Turning rate falloff

    // billboard info
    fvec3_t       _vfw;                 ///< the camera forward vector
    fvec3_t       _vup;                 ///< the camera up vector
    fvec3_t       _vrt;                 ///< the camera right vector

    // effects
    float         _motionBlur;         ///< Blurry effect
    float         _motionBlurOld;      ///< Blurry effect one frame ago (used to init the accum buffer)
    int           _swing;               ///< Camera swingin'
    int           _swingRate;
    float         _swingAmp;
    float         _roll;
};

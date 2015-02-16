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
/// @file game/Camera.hpp
/// @author Johan Jansen

#pragma once

#include "game/egoboo_typedef.h"
#include "game/physics.h"			//for orientation_t

//Forward declarations
struct ego_mesh_t;

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

struct CameraOptions
{
    int            swing;                   ///< Camera swing angle
    int            swingRate;               ///< Camera swing rate
    float          swingAmp;                ///< Camera swing amplitude
	CameraTurnMode turnMode;                ///< what is the camera turn mode
};

/// Multi cam uses macro to switch between old and new camera
constexpr float CAM_ZOOM_FACTOR = 0.5f;
#ifdef OLD_CAMERA_MODE
    constexpr float CAM_ZOOM_MIN = (800 * CAM_ZOOM_FACTOR);       ///< Camera distance
    constexpr float CAM_ZOOM_MAX = (700 * CAM_ZOOM_FACTOR);
    constexpr float CAM_ZADD_MIN = (800 * CAM_ZOOM_FACTOR);       ///< Camera height
    constexpr float CAM_ZADD_MAX = (2750 * CAM_ZOOM_FACTOR);
#else
    constexpr float CAM_ZOOM_MIN = (500 * CAM_ZOOM_FACTOR);       ///< Camera distance
    constexpr float CAM_ZOOM_MAX = (800 * CAM_ZOOM_FACTOR);
    constexpr float CAM_ZADD_MIN = (800 * CAM_ZOOM_FACTOR);       ///< Camera height
    constexpr float CAM_ZADD_MAX = (1900 * CAM_ZOOM_FACTOR);
#endif

/**
* @brief class for handling camera
**/
class Camera
{
public:
	Camera(const CameraOptions &options);
    ~Camera();
    
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
    static const uint8_t DEFAULT_TURN_TIME;

    static const float CAM_ZADD_AVG;
    static const float CAM_ZOOM_AVG;

    /**
    * @brief Initialization that has to be after object construction
    **/
    void initialize(int renderList, int doList);

    //various getters
	inline const fmat_4x4_t& getProjection() const {return _mProjection;}
    inline const egolib_frustum_t& getFrustum() const {return _frustum;}
	inline const fmat_4x4_t& getView() const {return _mView;}
    inline const orientation_t& getOrientation() const {return _ori;}
    inline CameraTurnMode getTurnMode() const { return _turnMode; }
    inline const fvec3_t& getTrackPosition() const { return _trackPos; }
    inline const fvec3_t& getCenter() const { return _center; }
    inline const fvec3_t& getPosition() const { return _pos; }
    inline uint8_t getTurnTime() const { return _turnTime; }
    inline float getTurnZOne() const { return _turnZOne; }
    inline float getTurnZRad() const { return _turnZRad; }

    inline const fvec3_t& getVUP() const { return _vup; }
    inline const fvec3_t& getVRT() const { return _vrt; }
    inline const fvec3_t& getVFW() const { return _vfw; }

    inline float getMotionBlur() const { return _motionBlur; }
    inline float getMotionBlurOld() const { return _motionBlurOld; }
    inline int getSwing() const { return _swing; }

    inline int getLastFrame() const {return _lastFrame;}
    inline int getRenderList() const {return _renderList;}
    inline int getDoList() const {return _doList;}

    inline const std::forward_list<CHR_REF>& getTrackList() const {return _trackList;}

    inline const ego_frect_t& getScreen() const { return _screen; }
    
    /**
    * @brief Sets which areas of the video screen this camera should draw on
    **/
    void setScreen(float xmin, float ymin, float xmax, float ymax);
    
    /**
     * @brief Makes this camera track the specified target
     */
    void addTrackTarget(const CHR_REF target);

    /// @details This function moves the camera
    void update(const ego_mesh_t * pmesh);

    /**
     * @brief set which frame this camera was last updated
     */
    void setLastFrame(int frame) {_lastFrame = frame;}

    /**
     *  @details This function makes sure the camera starts in a suitable position
     */
    void reset(const ego_mesh_t * pmesh);

    /**
     * @brief Force the camera to focus in on the players. Should be called any time there is
     *        a "change of scene". With the new velocity-tracking of the camera, this would include
     *        things like character respawns, adding new players, etc.
     */
    void resetTarget( const ego_mesh_t * pmesh);

    /**
    * @brief Changes the camera mode
    **/
    void setCameraMode(CameraMode mode) {_moveMode = mode;}

protected:
    /**
    * @brief This function makes the camera turn to face the character
	**/
	void updatePosition();

	void updateCenter();

	void updateTrack(const ego_mesh_t * pmesh);

	/**
	* @brief updates special effects like grog, blur, shaking, etc.
	**/
	void updateEffects();

    /**
    * @brief This function makes the camera look downwards as it is raised up
	**/
	void updateZoom();

    /**
    * @brief This function sets pcam->mView to the camera's location and rotation
	**/
	void makeMatrix();

    /**
    * @brief Read camera control input for one specific player controller
	**/
	void readInput(input_device_t *pdevice);

	/**
	* @brief Helper function to calculate FOV
	**/
	static inline float multiplyFOV(const float old_fov_deg, const float factor);

	void resetView();

	void updateProjection(const float fov_deg, const float aspect_ratio, const float frustum_near = 1.0f, const float frustum_far = 20.0f);

private:
	const CameraOptions _options;

   // the projection matrices
    fmat_4x4_t _mView;                        ///< view matrix (derived/cached from other attributes)

    fmat_4x4_t _mProjection;                  ///< normal projection matrix (derived/cached from other attributes)
    fmat_4x4_t _mProjectionBig;               ///< big    projection matrix (derived/cached from other attributes)
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
    float         _zaddGoto;                ///< Desired z position
    float         _zGoto;

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

    //Extended camera data
    std::forward_list<CHR_REF> _trackList;  ///< List of characters this camera is tracking
    ego_frect_t         _screen;

    int                 _lastFrame;         ///< number of last update frame
    int                 _renderList;        ///< renderlist refid (-1 for none)
    int                 _doList;            ///< dolist refid (-1 for none)
};

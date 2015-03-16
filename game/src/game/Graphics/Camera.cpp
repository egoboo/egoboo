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

#include "game/Graphics/Camera.hpp"
#include "game/graphic.h"
#include "game/player.h"

#include "game/game.h" //TODO: remove only needed for PMesh

#include "game/char.h"
#include "game/mesh.h"
#include "game/Entities/_Include.hpp"

const float Camera::DEFAULT_FOV = 60.0f;

const float Camera::DEFAULT_TURN_JOY = 64;

const float Camera::DEFAULT_TURN_KEY = DEFAULT_TURN_JOY;

const uint8_t Camera::DEFAULT_TURN_TIME = 16;

const float Camera::CAM_ZADD_AVG = (0.5f * (CAM_ZADD_MIN + CAM_ZADD_MAX));
const float Camera::CAM_ZOOM_AVG = (0.5f * (CAM_ZOOM_MIN + CAM_ZOOM_MAX));

Camera::Camera(const CameraOptions &options) :
	_options(options),
	_mView(),
	_mProjection(),
	_mProjectionBig(),
	_mProjectionSmall(),

	_frustum(),
	_frustumBig(),
	_frustumSmall(),

	_moveMode(CAM_PLAYER),

	_turnMode(_options.turnMode),
	_turnTime(DEFAULT_TURN_TIME),

	_pos(),
	_ori(),

	_trackPos(),
	_trackLevel(0.0f),

	_zoom(CAM_ZOOM_AVG),
	_center{0, 0, 0},
	_zadd(CAM_ZADD_AVG),
	_zaddGoto(CAM_ZADD_AVG),
	_zGoto(CAM_ZADD_AVG),

	_turnZRad(-Ego::Math::piOverFour<float>()),
	_turnZOne(0.0f),
	_turnZAdd(0.0f),
	_turnZSustain(0.60f),	

	_vfw{0, 0, 0},
	_vup{0, 0, 0},
	_vrt{0, 0, 0},

	//Special effects
	_motionBlur(0.0f),
	_motionBlurOld(0.0f),
	_swing(_options.swing),
	_swingRate(_options.swingRate),
	_swingAmp(_options.swingAmp),
	_roll(0.0f),

    //Extended camera data
    _trackList(),
    _screen(),
    _lastFrame(-1),
    _renderList(-1),
    _doList(-1)
{
    // derived values
	_trackPos = _center;
	_pos = _center;
    _pos.x += _zoom * SIN( _turnZRad );
    _pos.y += _zoom * COS( _turnZRad );
    _pos.z += CAM_ZADD_MAX;

    _turnZOne   = RAD_TO_ONE( _turnZRad );
    _ori.facing_z = ONE_TO_TURN( _turnZOne ) ;

    resetView();

    //Get renderlist manager pointer
    renderlist_mgr_t *rmgr_ptr = gfx_system_get_renderlist_mgr();
    if ( nullptr == rmgr_ptr ) {
        throw "Failed to get renderlist manager";
    }

    //Get dolist manager pointer
    dolist_mgr_t     *dmgr_ptr = gfx_system_get_dolist_mgr();
    if ( nullptr == dmgr_ptr ) {
        throw "Failed toget dolist manager";
    }

    // lock a renderlist for this camera
    _renderList = renderlist_mgr_t::get_free_idx( rmgr_ptr );

    // connect the renderlist to a mesh
    renderlist_t *rlst_ptr = renderlist_mgr_t::get_ptr( rmgr_ptr, _renderList );
	renderlist_t::setMesh(rlst_ptr, PMesh);

    // lock a dolist for this camera
    _doList = dolist_mgr_t::get_free_idx( dmgr_ptr );    

    // assume that the camera is fullscreen
    setScreen(0, 0, sdl_scr.x, sdl_scr.y);
}

Camera::~Camera()
{
    // free any locked renderlist
    renderlist_mgr_t *rmgr_ptr = gfx_system_get_renderlist_mgr();
    if ( -1 != _renderList )
    {
        renderlist_mgr_t::free_one( rmgr_ptr, _renderList );
        _renderList = -1;
    }

    // free any locked dolist
    dolist_mgr_t *dmgr_ptr = gfx_system_get_dolist_mgr();
    if ( -1 != _doList )
    {
        dolist_mgr_t::free_one( dmgr_ptr, _doList );
        _doList = -1;
    }
}

float Camera::multiplyFOV( const float old_fov_deg, const float factor )
{
    float old_fov_rad;
    float new_fov_rad, new_fov_deg;

    old_fov_rad = Ego::Math::degToRad(old_fov_deg);

    new_fov_rad = 2.0f * ATAN( factor * TAN( old_fov_rad * 0.5f ) );
    new_fov_deg = Ego::Math::radToDeg( new_fov_rad );

    return new_fov_deg;
}

void Camera::updateProjection(const float fov_deg, const float aspect_ratio, const float frustum_near, const float frustum_far)
{
    const float fov_mag = Ego::Math::sqrtTwo<float>();

    float fov_deg_big   = multiplyFOV( DEFAULT_FOV, fov_mag );
    float fov_deg_small = multiplyFOV( DEFAULT_FOV, 1.0f / fov_mag );
    
	fmat_4x4_t identity = fmat_4x4_t::identity();
    
	_mProjection.setPerspective(fov_deg, aspect_ratio, frustum_near, frustum_far);
	_mProjectionBig.setPerspective(fov_deg_big, aspect_ratio, frustum_near, frustum_far);
	_mProjectionSmall.setPerspective(fov_deg_small, aspect_ratio, frustum_near, frustum_far);
    
    // recalculate the frustum, too
    _frustum.calculate(_mProjection, _mView);
    _frustumBig.calculate(_mProjectionBig, _mView);
    _frustumSmall.calculate(_mProjectionSmall, _mView);
}

void Camera::resetView()
{
	float roll_deg = Ego::Math::radToDeg(_roll);

    // check for stupidity
    if (_pos != _center)
    {
        fmat_4x4_t tmp1, tmp2;
        
        tmp1.setScaling(fvec3_t(-1, 1, 1));
        mat_glRotate(tmp2.v, tmp1.v, roll_deg, 0, 0, 1);
        mat_gluLookAt(_mView.v, tmp2.v, _pos.x, _pos.y, _pos.z,
                      _center.x, _center.y, _center.z, 0.0f, 0.0f, 1.0f);
    }

    // the view matrix was updated, so update the frustum
    _frustum.calculate(_mProjection, _mView);
    _frustumBig.calculate(_mProjectionBig, _mView);
    _frustumSmall.calculate(_mProjectionSmall, _mView);	
}

void Camera::updatePosition()
{
    fvec3_t pos_new;

    // update the height
    _zGoto = _zadd;

    // update the turn
    //if ( 0 != _turnTime )
    //{
    //    _turnZRad   = ATAN2( _center.y - pos.y, _center.x - pos.x );  // xgg
    //    _turnZOne   = RAD_TO_ONE( _turnZRad );
    //    _ori.facing_z = ONE_TO_TURN( _turnZOne );
    //}

    // update the camera position
    TURN_T turnsin = TO_TURN( _ori.facing_z );
    pos_new.x = _center.x + _zoom * turntosin[turnsin];
    pos_new.y = _center.y + _zoom * turntocos[turnsin];
    pos_new.z = _center.z + _zGoto;

    // make the camera motion smooth
    _pos.x = 0.9f * _pos.x + 0.1f * pos_new.x;
    _pos.y = 0.9f * _pos.y + 0.1f * pos_new.y;
    _pos.z = 0.9f * _pos.z + 0.1f * pos_new.z;
}

void Camera::makeMatrix()
{
    resetView();

    //--- pre-compute some camera vectors
    mat_getCamForward(_mView, _vfw);
	_vfw.normalize();

    mat_getCamUp(_mView, _vup);
	_vup.normalize();

    mat_getCamRight(_mView, _vrt);
	_vrt.normalize();
}

void Camera::updateZoom()
{
    float percentmin, percentmax;

    // update zadd
    _zaddGoto = CLIP( _zaddGoto, CAM_ZADD_MIN, CAM_ZADD_MAX );
    _zadd      = 0.9f * _zadd  + 0.1f * _zaddGoto;

    // update zoom
    percentmax = ( _zaddGoto - CAM_ZADD_MIN ) / ( float )( CAM_ZADD_MAX - CAM_ZADD_MIN );
    percentmin = 1.0f - percentmax;
    _zoom = ( CAM_ZOOM_MIN * percentmin ) + ( CAM_ZOOM_MAX * percentmax );

    // update _turn_z
    if (std::abs( _turnZAdd ) < 0.5f)
    {
        _turnZAdd = 0.0f;
    }
    else
    {
        _ori.facing_z += _turnZAdd;
        _turnZOne    = TURN_TO_ONE( _ori.facing_z );
        _turnZRad    = ONE_TO_RAD( _turnZOne );
    }
    _turnZAdd *= _turnZSustain;

}

void Camera::updateCenter()
{
    // Center on target for doing rotation...
    if ( 0 != _turnTime )
    {
        _center.x = _center.x * 0.9f + _trackPos.x * 0.1f;
        _center.y = _center.y * 0.9f + _trackPos.y * 0.1f;
    }
    else
    {
        // what is the distance to the track position?
        fvec3_t track_vec = _trackPos - _pos;

        // determine the size of the dead zone
        float track_fov = DEFAULT_FOV * 0.25f;
        float track_dist = track_vec.length();
        float track_size = track_dist * TAN( track_fov );
        float track_size_x = track_size;
        float track_size_y = track_size;  /// @todo adjust this based on the camera viewing angle

        // calculate the difference between the _center of the tracked characters
        // and the _center of the camera look_at
		fvec2_t diff = fvec2_t(_trackPos[kX],_trackPos[kY]) - fvec2_t(_center[kX],_center[kY]);

        // get 2d versions of the camera's right and up vectors
		fvec2_t vrt(_vrt[kX],_vrt[kY]);
		vrt.normalize();

		fvec2_t vup(_vup[kX], _vup[kY]);
		vup.normalize();

        // project the diff vector into this space
        float diff_rt = vrt.dot(diff);
        float diff_up = vup.dot(diff);

        // Get ready to scroll...
		fvec2_t scroll;
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
        _center.x += scroll.x;
        _center.y += scroll.y;
    }

    // _center.z always approaches _trackPos.z
    _center.z = _center.z * 0.9f + _trackPos.z * 0.1f;
}

void Camera::updateTrack(const ego_mesh_t * pmesh)
{
    fvec3_t new_track;
    float new_track_level;

    // the default camera motion is to do nothing
    new_track.x     = _trackPos.x;
    new_track.y     = _trackPos.y;
    new_track.z     = _trackPos.z;
    new_track_level = _trackLevel;

    switch(_moveMode)
    {
    	case CAM_FREE:

	        // the keypad control the camera
	        if ( SDL_KEYDOWN( keyb, SDLK_KP8 ) )
	        {
	            _trackPos.x -= _mView.CNV( 0, 1 ) * 50;
	            _trackPos.y -= _mView.CNV( 1, 1 ) * 50;
	        }

	        if ( SDL_KEYDOWN( keyb, SDLK_KP2 ) )
	        {
	            _trackPos.x += _mView.CNV( 0, 1 ) * 50;
	            _trackPos.y += _mView.CNV( 1, 1 ) * 50;
	        }

	        if ( SDL_KEYDOWN( keyb, SDLK_KP4 ) )
	        {
	            _trackPos.x += _mView.CNV( 0, 0 ) * 50;
	            _trackPos.y += _mView.CNV( 1, 0 ) * 50;
	        }

	        if ( SDL_KEYDOWN( keyb, SDLK_KP6 ) )
	        {
	            _trackPos.x -= _mView.CNV( 0, 0 ) * 10;
	            _trackPos.y -= _mView.CNV( 1, 0 ) * 10;
	        }

	        if ( SDL_KEYDOWN( keyb, SDLK_KP7 ) )
	        {
	            _turnZAdd += DEFAULT_TURN_KEY;
	        }

	        if ( SDL_KEYDOWN( keyb, SDLK_KP9 ) )
	        {
	            _turnZAdd -= DEFAULT_TURN_KEY;
	        }

	        _trackPos.z = 128 + ego_mesh_t::get_level( pmesh, PointWorld(_trackPos.x, _trackPos.y));

       break;

	    // a camera movement mode for re-focusing in on a bunch of players
	    case CAM_RESET:
	    {
	        fvec3_t sum_pos;
	        float   sum_wt, sum_level;

	        sum_wt    = 0.0f;
	        sum_level = 0.0f;
			sum_pos = fvec3_t::zero;

	        for(CHR_REF ichr : _trackList)
	        {
	            Object * pchr = NULL;

	            if ( !_gameObjects.exists( ichr ) ) continue;
	            pchr = _gameObjects.get( ichr );

	            if ( !pchr->alive ) continue;

	            sum_pos.x += pchr->getPosX();
	            sum_pos.y += pchr->getPosY();
	            sum_pos.z += pchr->getPosZ() + pchr->chr_min_cv.maxs[OCT_Z] * 0.9f;
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
	    break;

	    // a camera mode for focusing in on the players that are actually doing something.
	    // "Show me the drama!"
	    case CAM_PLAYER:
	    {
	        Object * local_chr_ptrs[MAX_PLAYER];
	        int local_chr_count = 0;

	        // count the number of local players, first
	        local_chr_count = 0;
	        for(CHR_REF ichr : _trackList)
	        {
	            Object * pchr = NULL;

	            if ( !_gameObjects.exists( ichr ) ) continue;
	            pchr = _gameObjects.get( ichr );

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

	            new_track.x = local_chr_ptrs[0]->getPosX();
	            new_track.y = local_chr_ptrs[0]->getPosY();
	            new_track.z = local_chr_ptrs[0]->getPosZ();
	            new_track_level = local_chr_ptrs[0]->enviro.level + 128;
	        }
	        else
	        {
	            // use the characer's "activity" to average the position the camera is viewing

	            fvec3_t sum_pos;
	            float   sum_wt, sum_level;

	            sum_wt    = 0.0f;
	            sum_level = 0.0f;
				sum_pos = fvec3_t::zero;

	            for ( int cnt = 0; cnt < local_chr_count; cnt++ )
	            {
	                Object * pchr;
	                float weight1, weight2, weight;

	                // we JUST checked the validity of these characters. No need to do it again?
	                pchr = local_chr_ptrs[ cnt ];

	                // weight it by the character's velocity^2, so that
	                // inactive characters don't control the camera
	                weight1 = pchr->vel.dot(pchr->vel);

	                // make another weight based on button-pushing
	                weight2 = pchr->latch.b.none() ? 0 : 127;

	                // I would weight this by the amount of damage that the character just sustained,
	                // but there is no real way to do this?

	                // get the maximum effect
	                weight =  std::max( weight1, weight2 );

	                // The character is on foot
	                sum_pos.x += pchr->getPosX() * weight;
	                sum_pos.y += pchr->getPosY() * weight;
	                sum_pos.z += pchr->getPosZ() * weight;
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
	    break;
	}


    if ( CAM_RESET == _moveMode )
    {
        // just set the position
        _trackPos.x = new_track.x;
        _trackPos.y = new_track.y;
        _trackPos.z = new_track.z;
        _trackLevel = new_track_level;

        // reset the camera mode
        _moveMode = CAM_PLAYER;
    }
    else
    {
        // smoothly interpolate the camera tracking position
        _trackPos.x = 0.9f * _trackPos.x + 0.1f * new_track.x;
        _trackPos.y = 0.9f * _trackPos.y + 0.1f * new_track.y;
        _trackPos.z = 0.9f * _trackPos.z + 0.1f * new_track.z;
        _trackLevel = 0.9f * _trackLevel + 0.1f * new_track_level;
    }
}

/*
std::forward_list<CHR_REF> Camera::createTrackList()
{
	std::forward_list<CHR_REF> trackList;

    // scan the PlaStack for objects the camera can track
    for ( PLA_REF ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        player_t * ppla = PlaStack_get_ptr( ipla );

        if ( !ppla->valid || !_gameObjects.exists( ppla->index ) ) continue;

        // add in a valid character
        trackList.push_front(ppla->index);
    }

    return trackList;
}
*/

void Camera::update(const ego_mesh_t * pmesh)
{
    PLA_REF ipla;

    // update the _turnTime counter
    if ( CAM_TURN_NONE != _turnMode )
    {
        _turnTime = 255;
    }
    else if ( _turnTime > 0 )
    {
        _turnTime--;
    }

    // Camera controls
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        player_t *ppla;

        //Don't do invalid players
        if ( INVALID_PLA( ipla ) ) continue;
        ppla = PlaStack.get_ptr( ipla );

        //Handle camera control from this player
        readInput( ppla->pdevice );
    }

    // update the special camera effects like grog
    updateEffects();

    // update the average position of the tracked characters
    updateTrack( pmesh );

    // move the camera center, if need be
    updateCenter();

    // make the zadd and zoom work together
    updateZoom();

    // update the position of the camera
    updatePosition();

    // set the view matrix
    makeMatrix();
}

void Camera::readInput( input_device_t *pdevice )
{
    int type;
    bool autoturn_camera;

    //Don't do network players
    if ( NULL == pdevice ) return;
    type = pdevice->device_type;

    //If the device isn't enabled there is no point in continuing
    if ( !input_device_is_enabled( pdevice ) ) return;

    //Autoturn camera only works in single player and when it is enabled
    autoturn_camera = ( CAM_TURN_GOOD == _turnMode ) && ( 1 == local_stats.player_count );

    //No auto camera
    if ( INPUT_DEVICE_MOUSE == type )
    {
        // Mouse control

        //Auto camera
        if ( autoturn_camera )
        {
            if ( !input_device_control_active( pdevice,  CONTROL_CAMERA ) )
            {
                _turnZAdd -= ( mous.x * 0.5f );
            }
        }

        //Normal camera
        else if ( input_device_control_active( pdevice,  CONTROL_CAMERA ) )
        {
            _turnZAdd += ( mous.x / 3.0f );
            _zaddGoto  += static_cast<float>(mous.y) / 3.0f;

            _turnTime = DEFAULT_TURN_TIME;  // Sticky turn...
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
                _turnZAdd -= pjoy->x * DEFAULT_TURN_JOY;
            }
        }

        //Normal camera
        else if ( input_device_control_active( pdevice, CONTROL_CAMERA ) )
        {
            _turnZAdd += pjoy->x * DEFAULT_TURN_JOY;
            _zaddGoto  += pjoy->y * DEFAULT_TURN_JOY;

            _turnTime = DEFAULT_TURN_TIME;  // Sticky turn...
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
                _turnZAdd += DEFAULT_TURN_KEY;
            }
            if ( input_device_control_active( pdevice,  CONTROL_RIGHT ) )
            {
                _turnZAdd -= DEFAULT_TURN_KEY;
            }
        }
        else
        {
            // Normal camera

            int _turn_z_diff = 0;

            // rotation
            if ( input_device_control_active( pdevice,  CONTROL_CAMERA_LEFT ) )
            {
                _turn_z_diff += DEFAULT_TURN_KEY;
            }
            if ( input_device_control_active( pdevice,  CONTROL_CAMERA_RIGHT ) )
            {
                _turn_z_diff -= DEFAULT_TURN_KEY;
            }

            // Sticky turn?
            if ( 0 != _turn_z_diff )
            {
                _turnZAdd += _turn_z_diff;
                _turnTime   = DEFAULT_TURN_TIME;
            }

            //zoom
            if ( input_device_control_active( pdevice,  CONTROL_CAMERA_OUT ) )
            {
                _zaddGoto += DEFAULT_TURN_KEY;
            }
            if ( input_device_control_active( pdevice,  CONTROL_CAMERA_IN ) )
            {
                _zaddGoto -= DEFAULT_TURN_KEY;
            }
        }
    }
}

void Camera::reset( const ego_mesh_t * pmesh )
{
    // constant values
    _trackLevel   = 0.0f;
    _zoom         = CAM_ZOOM_AVG;
    _zadd         = CAM_ZADD_AVG;
    _zaddGoto     = CAM_ZADD_AVG;
    _zGoto        = CAM_ZADD_AVG;
    _turnZRad     = -Ego::Math::piOverFour<float>();
    _turnZAdd     = 0.0f;
    _roll         = 0.0f;

    // derived values
    _center.x     = pmesh->gmem.edge_x * 0.5f;
    _center.y     = pmesh->gmem.edge_y * 0.5f;
    _center.z     = 0.0f;

	_trackPos = _center;
	_pos = _center;

    _pos.x += _zoom * SIN( _turnZRad );
    _pos.y += _zoom * COS( _turnZRad );
    _pos.z += CAM_ZADD_MAX;

    _turnZOne   = RAD_TO_ONE( _turnZRad );
    _ori.facing_z = ONE_TO_TURN( _turnZOne ) ;

    // get optional parameters
    _swing      = _options.swing;
    _swingRate  = _options.swingRate;
    _swingAmp   = _options.swingAmp;
    _turnMode   = _options.turnMode;

    // make sure you are looking at the players
    resetTarget( pmesh );
}

void Camera::resetTarget( const ego_mesh_t * pmesh )
{
	CameraTurnMode turnModeSave;
	CameraMode moveModeSave;

    // save some values
    turnModeSave = _turnMode;
    moveModeSave = _moveMode;

    // get back to the default view matrix
    resetView();

    // specify the modes that will make the camera point at the players
    _turnMode = CAM_TURN_AUTO;
    _moveMode = CAM_RESET;

    // If you use CAM_RESET, camera_move() automatically restores _moveMode
    // to its default setting
    update(pmesh);

    // fix the center position
    _center.x = _trackPos.x;
    _center.y = _trackPos.y;

    // restore the modes
    _turnMode = turnModeSave;
    _moveMode = moveModeSave;

    // reset the turn time
    _turnTime = 0;
}

void Camera::updateEffects()
{
    float local_swingamp = _swingAmp;
    
    _motionBlurOld = _motionBlur;

    // Fade out the motion blur
    if ( _motionBlur > 0 )
    {
        _motionBlur *= 0.99f; //Decay factor
        if ( _motionBlur < 0.001f ) _motionBlur = 0;
    }

    // Swing the camera if players are groggy
    if ( local_stats.grog_level > 0 )
    {
        float zoom_add;
        _swing = ( _swing + 120 ) & 0x3FFF;
        local_swingamp = std::max( local_swingamp, 0.175f );

        zoom_add = ( 0 == ((( int )local_stats.grog_level ) % 2 ) ? 1 : - 1 ) * DEFAULT_TURN_KEY * local_stats.grog_level * 0.35f;

        _zaddGoto   = _zaddGoto + zoom_add;

        _zaddGoto = CLIP( _zaddGoto, CAM_ZADD_MIN, CAM_ZADD_MAX );
    }

    //Rotate camera if they are dazed
    if ( local_stats.daze_level > 0 )
    {
        _turnZAdd = local_stats.daze_level * DEFAULT_TURN_KEY * 0.5f;
    }
    
    // Apply motion blur
    if ( local_stats.daze_level > 0 || local_stats.grog_level > 0 )
        _motionBlur = std::min( 0.95f, 0.5f + 0.03f * std::max( local_stats.daze_level, local_stats.grog_level ));

    //Apply camera swinging
    //mat_Multiply( _mView.v, mat_Translate( tmp1.v, pos.x, -pos.y, pos.z ), _mViewSave.v );  // xgg
    if ( local_swingamp > 0.001f )
    {
        _roll = turntosin[_swing] * local_swingamp;
        //mat_Multiply( _mView.v, mat_RotateY( tmp1.v, roll ), mat_Copy( tmp2.v, _mView.v ) );
    }
    // If the camera stops swinging for some reason, slowly return to _original position
    else if ( 0 != _roll )
    {
        _roll *= 0.9875f;            //Decay factor
        //mat_Multiply( _mView.v, mat_RotateY( tmp1.v, _roll ), mat_Copy( tmp2.v, _mView.v ) );

        // Come to a standstill at some point
        if ( std::fabs( _roll ) < 0.001f )
        {
            _roll = 0;
            _swing = 0;
        }
    }

    _swing = ( _swing + _swingRate ) & 0x3FFF;
}

void Camera::setScreen( float xmin, float ymin, float xmax, float ymax )
{
    // set the screen
    _screen.xmin = xmin;
    _screen.ymin = ymin;
    _screen.xmax = xmax;
    _screen.ymax = ymax;

    //Update projection after setting size
    float frustum_near, frustum_far, aspect_ratio;

    //---- set the camera's projection matrix
    aspect_ratio = ( _screen.xmax - _screen.xmin ) / ( _screen.ymax - _screen.ymin );

    // the nearest we will have to worry about is 1/2 of a tile
    frustum_near = GRID_ISIZE * 0.25f;

    // set the maximum depth to be the "largest possible size" of a mesh
    frustum_far  = GRID_ISIZE * 256 * Ego::Math::sqrtTwo<float>();

    updateProjection(DEFAULT_FOV, aspect_ratio, frustum_near, frustum_far);
}

void Camera::initialize(int renderList, int doList)
{
    // make the default viewport fullscreen
    _screen.xmin = 0.0f;
    _screen.xmax = sdl_scr.x;
    _screen.ymin = 0.0f;
    _screen.ymax = sdl_scr.y;

    _renderList = renderList;
    _doList = doList;
}

void Camera::addTrackTarget(const CHR_REF target)
{
    _trackList.push_front(target);
}

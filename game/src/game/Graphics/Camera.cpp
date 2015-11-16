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

#include "game/game.h" // TODO: remove only needed for mesh

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
    _viewMatrix(),
    _projectionMatrix(),
    _frustumInvalid(true),
    _frustum(),
    _moveMode(CameraMovementMode::Player),

    _turnMode(_options.turnMode),
    _turnTime(DEFAULT_TURN_TIME),

    _position(),
    _ori(),

    _trackPos(),
    _trackLevel(0.0f),

    _zoom(CAM_ZOOM_AVG),
    _center{0.0f, 0.0f, 0.0f},
    _zadd(CAM_ZADD_AVG),
    _zaddGoto(CAM_ZADD_AVG),
    _zGoto(CAM_ZADD_AVG),

    _turnZ_radians(-Ego::Math::piOverFour<float>()),
    _turnZ_turns(RadiansToTurns(_turnZ_radians)),
    _turnZAdd(0.0f),
    _turnZSustain(0.60f),

    _forward{0.0f, 0.0f, 0.0f},
    _up{0.0f, 0.0f, 0.0f},
    _right{0.0f, 0.0f, 0.0f},

    // Special effects.
    _motionBlur(0.0f),
    _motionBlurOld(0.0f),
    _swing(_options.swing),
    _swingRate(_options.swingRate),
    _swingAmp(_options.swingAmp),
    _roll(0.0f),

    // Extended camera data.
    _trackList(),
    _screen(),
    _lastFrame(-1),
    _tileList(nullptr),
    _entityList(nullptr)
{
    // Derived values.
    _trackPos = _center;
    _position = _center + Vector3f(_zoom * std::sin(_turnZ_radians), _zoom * std::cos(_turnZ_radians), CAM_ZADD_MAX);

    _turnZ_turns = RadiansToTurns(_turnZ_radians);
    _ori.facing_z = TurnsToFacing(_turnZ_turns);
    resetView();

    // Get renderlist manager pointer.
    renderlist_mgr_t *rmgr_ptr = gfx_system_get_renderlist_mgr();
    if (!rmgr_ptr)
    {
        throw std::runtime_error("failed to get renderlist manager");
    }

    // Get dolist manager pointer.
    dolist_mgr_t *dmgr_ptr = gfx_system_get_dolist_mgr();
    if (!dmgr_ptr)
    {
        throw std::runtime_error("failed to get dolist manager");
    }

    // Lock a tile list for this camera.
    _tileList = rmgr_ptr->acquire();
    // Connect the tile list to the mesh.
    _tileList->setMesh(_currentModule->getMeshPointer());

    // Lock an entity list for this camera.
    _entityList = dmgr_ptr->acquire();

    // Assume that the camera is fullscreen.
    setScreen(0, 0, sdl_scr.x, sdl_scr.y);
}

Camera::~Camera()
{
    // Free any locked tile list.
    _tileList = nullptr;

    // Free any locked entity list.
    _entityList = nullptr;
}

float Camera::multiplyFOV(const float old_fov_deg, const float factor)
{
    float old_fov_rad = Ego::Math::degToRad(old_fov_deg);

    float new_fov_rad = 2.0f * std::atan(factor * std::tan(old_fov_rad * 0.5f));
    float new_fov_deg = Ego::Math::radToDeg(new_fov_rad);

    return new_fov_deg;
}

void Camera::updateProjection(const float fov_deg, const float aspect_ratio, const float frustum_near, const float frustum_far)
{
    _projectionMatrix = Ego::Math::Transform::perspective(fov_deg, aspect_ratio, frustum_near, frustum_far);

    // Invalidate the frustum.
    _frustumInvalid = true;
}

void Camera::resetView()
{
    float roll_deg = Ego::Math::radToDeg(_roll);

    // Check for stupidity.
    if (_position != _center)
    {
        static const Vector3f Z = Vector3f(0.0f, 0.0f, 1.0f);
        Matrix4f4f tmp = Ego::Math::Transform::scaling(Vector3f(-1.0f, 1.0f, 1.0f)) *  Ego::Math::Transform::rotation(Z, roll_deg);
        _viewMatrix = tmp *  Ego::Math::Transform::lookAt(_position, _center, Z);
    }
    // Invalidate the frustum.
    _frustumInvalid = true;
}

void Camera::updatePosition()
{
    // Update the height.
    _zGoto = _zadd;

    // Update the turn.
#if 0
    if ( 0 != _turnTime )
    {
        _turnZRad = std::atan2(_center[kY] - pos[kY], _center[kX] - pos[kX]);  // xgg
        _turnZOne = RadiansToTurns(_turnZRad);
        _ori.facing_z = TurnsToFacing(_turnZ_turns);
    }
#endif

    // Update the camera position.
    TURN_T turnsin = TO_TURN(_ori.facing_z);
    Vector3f pos_new = _center + Vector3f(_zoom * turntosin[turnsin], _zoom * turntocos[turnsin], _zGoto);

    // Make the camera motion smooth.
    _position = _position * 0.9f + pos_new * 0.1f; /// @todo Use Ego::Math::lerp.
}

void Camera::makeMatrix()
{
    resetView();

    // Pre-compute some camera vectors.
    mat_getCamForward(_viewMatrix, _forward);
    _forward.normalize();

    mat_getCamUp(_viewMatrix, _up);
    _up.normalize();

    mat_getCamRight(_viewMatrix, _right);
    _right.normalize();
}

void Camera::updateZoom()
{
    // Update zadd.
    _zaddGoto = Ego::Math::constrain(_zaddGoto, CAM_ZADD_MIN, CAM_ZADD_MAX);
    _zadd = 0.9f * _zadd  + 0.1f * _zaddGoto; /// @todo Use Ego::Math::lerp.

    // Update zoom.
    float percentmax = (_zaddGoto - CAM_ZADD_MIN ) / (float )( CAM_ZADD_MAX - CAM_ZADD_MIN);
    float percentmin = 1.0f - percentmax;
    _zoom = (CAM_ZOOM_MIN * percentmin) + (CAM_ZOOM_MAX * percentmax);

    // update _turn_z
    if (std::abs( _turnZAdd ) < 0.5f)
    {
        _turnZAdd = 0.0f;
    }
    else
    {
        //Make it wrap around
        float newAngle = static_cast<float>(_ori.facing_z) +_turnZAdd;
        if (newAngle < 0.0f) {
            newAngle += 0xFFFF;
        } else if(newAngle > 0xFFFF) {
            newAngle -= 0xFFFF;
        }
        _ori.facing_z = newAngle;

        _turnZ_turns = FacingToTurns(_ori.facing_z);
        _turnZ_radians = TurnsToRadians(_turnZ_turns);
    }
    _turnZAdd *= _turnZSustain;
}

void Camera::updateCenter()
{
    // Center on target for doing rotation ...
    if ( 0 != _turnTime )
    {
        _center[kX] = _center[kX] * 0.9f + _trackPos[kX] * 0.1f;
        _center[kY] = _center[kY] * 0.9f + _trackPos[kY] * 0.1f;
    }
    else
    {
        // Determine tracking direction.
        Vector3f track_vec = _trackPos - _position;

        // Determine the size of the dead zone.
        float track_fov = DEFAULT_FOV * 0.25f;
        float track_dist = track_vec.length();
        float track_size = track_dist * std::tan(track_fov);
        float track_size_x = track_size;
        float track_size_y = track_size;  /// @todo adjust this based on the camera viewing angle

        // Calculate the difference between the center of the tracked characters
        // and the center of the camera look to look at.
        Vector2f diff = Vector2f(_trackPos[kX], _trackPos[kY]) - Vector2f(_center[kX], _center[kY]);

        // Get 2d versions of the camera's right and up vectors.
        Vector2f vrt(_right[kX], _right[kY]);
        vrt.normalize();

        Vector2f vup(_up[kX], _up[kY]);
        vup.normalize();

        // project the diff vector into this space
        float diff_rt = vrt.dot(diff);
        float diff_up = vup.dot(diff);

        // Get ready to scroll ...
        Vector2f scroll;
        if (diff_rt < -track_size_x)
        {
            // Scroll left
            scroll += vrt * (diff_rt + track_size_x);
        }
        if (diff_rt > track_size_x)
        {
            // Scroll right.
            scroll += vrt * (diff_rt - track_size_x);
        }

        if (diff_up > track_size_y)
        {
            // Scroll down.
            scroll += vup * (diff_up - track_size_y);
        }

        if (diff_up < -track_size_y)
        {
            // Scroll up.
            scroll += vup * (diff_up + track_size_y);
        }

        // Scroll.
        _center[XX] += scroll[XX];
        _center[YY] += scroll[YY];
    }

    // _center.z always approaches _trackPos.z
    _center[kZ] = _center[kZ] * 0.9f + _trackPos[kZ] * 0.1f; /// @todo Use Ego::Math::lerp
}

void Camera::updateTrack(const ego_mesh_t *mesh)
{
    // The default camera motion is to do nothing.
    Vector3f new_track = _trackPos;
    float new_track_level = _trackLevel;

    switch(_moveMode)
    {
    // The camera is controlled by the keypad.
    case CameraMovementMode::Free:
            if (keyb.is_key_down(SDLK_KP_8))
            {
                _trackPos[kX] -= _viewMatrix(1, 0) * 50;
                _trackPos[kY] -= _viewMatrix(1, 1) * 50;
            }

            if (keyb.is_key_down(SDLK_KP_2))
            {
                _trackPos[kX] += _viewMatrix(1, 0) * 50;
                _trackPos[kY] += _viewMatrix(1, 1) * 50;
            }

            if (keyb.is_key_down(SDLK_KP_4))
            {
                _trackPos[kX] += _viewMatrix(0, 0) * 50;
                _trackPos[kY] += _viewMatrix(0, 1) * 50;
            }

            if (keyb.is_key_down(SDLK_KP_6))
            {
                _trackPos[kX] -= _viewMatrix(0, 0) * 10;
                _trackPos[kY] -= _viewMatrix(0, 1) * 10;
            }

            if (keyb.is_key_down(SDLK_KP_7))
            {
                _turnZAdd += DEFAULT_TURN_KEY;
            }

            if (keyb.is_key_down(SDLK_KP_9))
            {
                _turnZAdd -= DEFAULT_TURN_KEY;
            }

            _trackPos[kZ] = 128 + mesh->getElevation(Vector2f(_trackPos[kX], _trackPos[kY]));

       break;

    // The camera is (re-)focuses in on a one or more objects.
    case CameraMovementMode::Reset:
        {
            float sum_wt    = 0.0f;
            float sum_level = 0.0f;
            Vector3f sum_pos = Vector3f::zero();

            for(ObjectRef objectRef : _trackList)
            {
                if (!_currentModule->getObjectHandler().exists(objectRef)) continue;
                Object *object = _currentModule->getObjectHandler().get(objectRef);

                if (!object->isAlive()) continue;

                sum_pos += object->getPosition() + Vector3f(0.0f, 0.0f, object->chr_min_cv._maxs[OCT_Z] * 0.9f);
                sum_level += object->enviro.level;
                sum_wt += 1.0f;
            }

            // If any of the characters is doing anything.
            if (sum_wt > 0.0f)
            {
                new_track = sum_pos * (1.0f / sum_wt);
                new_track_level = sum_level / sum_wt;
            }
        }
        break;

    // The camera is (re-)focuses in on a one or more player objects.
    // "Show me the drama!"
    case CameraMovementMode::Player:
        {
            Object *local_chr_ptrs[MAX_PLAYER];
            int local_chr_count = 0;

            // Count the number of local players, first.
            local_chr_count = 0;
            for(ObjectRef objectRef : _trackList)
            {
                if (!_currentModule->getObjectHandler().exists(objectRef)) continue;
                Object *object = _currentModule->getObjectHandler().get(objectRef);

                if (!object->isAlive()) continue;

                local_chr_ptrs[local_chr_count] = object;
                local_chr_count++;
            }

            if (0 == local_chr_count)
            {
                // Do nothing.
            }
            else if (1 == local_chr_count)
            {
                // Copy from the one character.
                new_track = local_chr_ptrs[0]->getPosition();
                new_track_level = local_chr_ptrs[0]->enviro.level + 128;
            }
            else
            {
                // Use the characer's "activity" to average the position the camera is viewing.
                float sum_wt    = 0.0f;
                float sum_level = 0.0f;
                Vector3f sum_pos = Vector3f::zero();

                for (int cnt = 0; cnt < local_chr_count; ++cnt)
                {
                    // We JUST checked the validity of these characters. No need to do it again?
                    Object *pchr = local_chr_ptrs[cnt];

                    // Weight it by the character's velocity^2, so that
                    // inactive characters don't control the camera.
                    float weight1 = pchr->vel.dot(pchr->vel);

                    // Make another weight based on button-pushing.
                    float weight2 = pchr->latch.b.none() ? 0 : 127;

                    // I would weight this by the amount of damage that the character just sustained,
                    // but there is no real way to do this?

                    // Get the maximum effect.
                    float weight = std::max(weight1, weight2);

                    // The character is on foot.
                    sum_pos += pchr->getPosition() * weight;
                    sum_level += (pchr->enviro.level + 128) * weight;
                    sum_wt += weight;
                }

                // If any of the characters is doing anything.
                if (sum_wt > 0.0f)
                {
                    new_track = sum_pos * (1.0f / sum_wt);
                    new_track_level = sum_level / sum_wt;
                }
            }
        }
        break;
    }


    if (CameraMovementMode::Reset == _moveMode)
    {
        // Just set the position.
        _trackPos = new_track;
        _trackLevel = new_track_level;

        // Reset the camera mode.
        _moveMode = CameraMovementMode::Player;
    }
    else
    {
        // Smoothly interpolate the camera tracking position.
        _trackPos = _trackPos * 0.9f + new_track * 0.1f;           /// @todo Use Ego::Math::lerp.
        _trackLevel = 0.9f * _trackLevel + 0.1f * new_track_level; /// @todo Use Ego::Math::lerp.
    }
}

void Camera::update(const ego_mesh_t *mesh)
{
    // Update the _turnTime counter.
    if (CameraTurnMode::None != _turnMode)
    {
        _turnTime = 255;
    }
    else if (_turnTime > 0)
    {
        _turnTime--;
    }

    // Camera controls.
    for (PLA_REF ipla = 0; ipla < MAX_PLAYER; ++ipla)
    {
        // Don't do invalid players.
        if (INVALID_PLA(ipla))
        {
            continue;
        }
        player_t *ppla = PlaStack.get_ptr(ipla);

        // Handle camera control from this player.
        readInput(ppla->pdevice);
    }

    // Update the special camera effects like grog.
    updateEffects();

    // Update the average position of the tracked characters.
    updateTrack(mesh);

    // Move the camera center, if need be.
    updateCenter();

    // Make the zadd and zoom work together.
    updateZoom();

    // Update the position of the camera.
    updatePosition();

    // Set the view matrix.
    makeMatrix();
}

void Camera::readInput(input_device_t *pdevice)
{
    // Don't do network players.
    if (!pdevice) return;
    int type = pdevice->device_type;

    // If the device isn't enabled there is no point in continuing.
    if (!input_device_is_enabled(pdevice)) return;

    // Autoturn camera only works in single player and when it is enabled.
    bool autoturn_camera = (CameraTurnMode::Good == _turnMode) && (1 == local_stats.player_count);

    // No auto camera.
    if (INPUT_DEVICE_MOUSE == type)
    {
        // Mouse control:

        // Autoturn camera.
        if (autoturn_camera)
        {
            if (!input_device_control_active(pdevice, CONTROL_CAMERA))
            {
                _turnZAdd -= mous.x * 0.5f;
            }
        }
        // Normal camera.
        else if (input_device_control_active(pdevice, CONTROL_CAMERA))
        {
            _turnZAdd += mous.x / 3.0f;
            _zaddGoto += static_cast<float>(mous.y) / 3.0f;

            _turnTime = DEFAULT_TURN_TIME;  // Sticky turn ...
        }
    }
    else if (IS_VALID_JOYSTICK(type))
    {
        // Joystick camera controls:

        int ijoy = type - INPUT_DEVICE_JOY;

        // Figure out which joystick this is.
        joystick_data_t *pjoy = joy_lst + ijoy;

        // Autoturn camera.
        if (autoturn_camera)
        {
            if (!input_device_control_active(pdevice, CONTROL_CAMERA))
            {
                _turnZAdd -= pjoy->x * DEFAULT_TURN_JOY;
            }
        }
        // Normal camera.
        else if (input_device_control_active( pdevice, CONTROL_CAMERA))
        {
            _turnZAdd += pjoy->x * DEFAULT_TURN_JOY;
            _zaddGoto += pjoy->y * DEFAULT_TURN_JOY;

            _turnTime = DEFAULT_TURN_TIME;  // Sticky turn ...
        }
    }
    else
    {
        // INPUT_DEVICE_KEYBOARD and any unknown device end up here:

        // Autoturn camera.
        if (autoturn_camera)
        {
            if (input_device_control_active(pdevice,  CONTROL_LEFT))
            {
                _turnZAdd += DEFAULT_TURN_KEY;
            }
            if (input_device_control_active(pdevice,  CONTROL_RIGHT))
            {
                _turnZAdd -= DEFAULT_TURN_KEY;
            }
        }
        // Normal camera.
        else
        {
            int _turn_z_diff = 0;

            // Rotation.
            if (input_device_control_active(pdevice, CONTROL_CAMERA_LEFT))
            {
                _turn_z_diff += DEFAULT_TURN_KEY;
            }
            if (input_device_control_active(pdevice, CONTROL_CAMERA_RIGHT))
            {
                _turn_z_diff -= DEFAULT_TURN_KEY;
            }

            // Sticky turn?
            if (0 != _turn_z_diff)
            {
                _turnZAdd += _turn_z_diff;
                _turnTime   = DEFAULT_TURN_TIME;
            }

            // Zoom.
            if (input_device_control_active(pdevice, CONTROL_CAMERA_OUT))
            {
                _zaddGoto += DEFAULT_TURN_KEY;
            }
            if (input_device_control_active(pdevice,  CONTROL_CAMERA_IN))
            {
                _zaddGoto -= DEFAULT_TURN_KEY;
            }
        }
    }
}

void Camera::reset(const ego_mesh_t *mesh)
{
    // Defaults.
    _trackLevel   = 0.0f;
    _zoom         = CAM_ZOOM_AVG;
    _zadd = CAM_ZADD_AVG;
    _zaddGoto = CAM_ZADD_AVG;
    _zGoto = CAM_ZADD_AVG;
    _turnZ_radians = -Ego::Math::piOverFour<float>();
    _turnZAdd = 0.0f;
    _roll = 0.0f;

    // Derived values.
    _center[kX]     = mesh->_tmem._edge_x * 0.5f;
    _center[kY]     = mesh->_tmem._edge_y * 0.5f;
    _center[kZ]     = 0.0f;

    _trackPos = _center;
    _position = _center;

    _position[kX] += _zoom * std::sin(_turnZ_radians);
    _position[kY] += _zoom * std::cos(_turnZ_radians);
    _position[kZ] += CAM_ZADD_MAX;

    _turnZ_turns = RadiansToTurns(_turnZ_radians);
    _ori.facing_z = TurnsToFacing(_turnZ_turns);

    // Get optional parameters.
    _swing = _options.swing;
    _swingRate = _options.swingRate;
    _swingAmp = _options.swingAmp;
    _turnMode = _options.turnMode;

    // Make sure you are looking at the players.
    resetTarget(mesh);
}

void Camera::resetTarget(const ego_mesh_t *mesh)
{
    // Save some values.
    CameraTurnMode turnModeSave = _turnMode;
    CameraMovementMode moveModeSave = _moveMode;

    // Get back to the default view matrix.
    resetView();

    // Specify the modes that will make the camera point at the players.
    _turnMode = CameraTurnMode::Auto;
    _moveMode = CameraMovementMode::Reset;

    // If you use Camera::MoveMode::Reset,
    // Camera::update() automatically restores _moveMode to its default setting.
    update(mesh);

    // Fix the center position.
    _center[kX] = _trackPos[kX];
    _center[kY] = _trackPos[kY];

    // Restore the modes.
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
        _swing = (_swing + 120) & 0x3FFF;
        local_swingamp = std::max(local_swingamp, 0.175f);

        zoom_add = ( 0 == ((( int )local_stats.grog_level ) % 2 ) ? 1 : - 1 ) * DEFAULT_TURN_KEY * local_stats.grog_level * 0.35f;

        _zaddGoto   = _zaddGoto + zoom_add;

        _zaddGoto = Ego::Math::constrain(_zaddGoto, CAM_ZADD_MIN, CAM_ZADD_MAX);
    }

    //Rotate camera if they are dazed
    if ( local_stats.daze_level > 0 )
    {
        _turnZAdd = local_stats.daze_level * DEFAULT_TURN_KEY * 0.5f;
    }
    
    // Apply motion blur
    if ( local_stats.daze_level > 0 || local_stats.grog_level > 0 ) {
        _motionBlur = std::min(0.95f, 0.5f + 0.03f * std::max(local_stats.daze_level, local_stats.grog_level));
    }

    //Apply camera swinging
    //mat_Multiply( _mView.v, mat_Translate( tmp1.v, pos[kX], -pos[kY], pos[kZ] ), _mViewSave.v );  // xgg
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
    // Set the screen rectangle.
    _screen.xmin = xmin;
    _screen.ymin = ymin;
    _screen.xmax = xmax;
    _screen.ymax = ymax;

    // Update projection after setting size.
    float aspect_ratio = (_screen.xmax - _screen.xmin) / (_screen.ymax - _screen.ymin);
    // The nearest we will have to worry about is 1/2 of a tile.
    float frustum_near = Info<int>::Grid::Size() * 0.25f;
    // Set the maximum depth to be the "largest possible size" of a mesh.
    float frustum_far  = Info<int>::Grid::Size() * 256 * Ego::Math::sqrtTwo<float>();
    updateProjection(DEFAULT_FOV, aspect_ratio, frustum_near, frustum_far);
}

void Camera::initialize(std::shared_ptr<Ego::Graphics::TileList> tileList, std::shared_ptr<Ego::Graphics::EntityList> entityList)
{
    // Make the default viewport fullscreen.
    _screen.xmin = 0.0f;
    _screen.xmax = sdl_scr.x;
    _screen.ymin = 0.0f;
    _screen.ymax = sdl_scr.y;

    _tileList = tileList;
    _entityList = entityList;
}

void Camera::addTrackTarget(ObjectRef targetRef)
{
    _trackList.push_front(targetRef);
}

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
#include "game/Logic/Player.hpp"
#include "egolib/InputControl/InputDevice.hpp"
#include "game/Graphics/TileList.hpp"
#include "game/Graphics/EntityList.hpp"
#include "egolib/Graphics/Viewport.hpp"

#include "game/game.h" // TODO: remove only needed for mesh

#include "game/mesh.h"
#include "egolib/Entities/_Include.hpp"

const Ego::Math::Degrees Camera::DEFAULT_FOV = Ego::Math::Degrees(60.0f);

const float Camera::DEFAULT_TURN_JOY = 64;

const float Camera::DEFAULT_TURN_KEY = DEFAULT_TURN_JOY;

const uint8_t Camera::DEFAULT_TURN_TIME = 16;

const float Camera::CAM_ZADD_AVG = (0.5f * (CAM_ZADD_MIN + CAM_ZADD_MAX));
const float Camera::CAM_ZOOM_AVG = (0.5f * (CAM_ZOOM_MIN + CAM_ZOOM_MAX));

Camera::Camera(const CameraOptions &options) :
    _options(options),

    _frustumInvalid(true),
    _frustum(),
    _moveMode(CameraMovementMode::Player),

    _turnMode(_options.turnMode),
    _turnTime(DEFAULT_TURN_TIME),

    _ori(),

    _trackPos(),

    _zoom(CAM_ZOOM_AVG),
    _center{0.0f, 0.0f, 0.0f},
    _zadd(CAM_ZADD_AVG),
    _zaddGoto(CAM_ZADD_AVG),
    _zGoto(CAM_ZADD_AVG),
    _pitch(id::pi_over<float, 2>()),

    _turnZ_radians(-id::pi_over<float,4>()),
    _turnZ_turns(id::semantic_cast<Ego::Math::Turns>(_turnZ_radians)),
    _turnZAdd(0.0f),

    m_viewMatrix(),
    m_projectionMatrix(),
    m_position(),
    m_forward{0.0f, 0.0f, 0.0f},
    m_up{0.0f, 0.0f, 0.0f},
    m_right{0.0f, 0.0f, 0.0f},
    m_viewport(std::make_unique<Ego::Graphics::Viewport>()),

    // Special effects.
    _motionBlur(0.0f),
    _motionBlurOld(0.0f),
    _swing(_options.swing),
    _swingRate(_options.swingRate),
    _swingAmp(_options.swingAmp),
    _roll(0.0f),

    // Extended camera data.
    _trackList(),
    _lastFrame(-1),
    _tileList(std::make_shared<Ego::Graphics::TileList>()),
    _entityList(std::make_shared<Ego::Graphics::EntityList>())
{
    // Derived values.
    _trackPos = _center;
    m_position = _center + Vector3f(_zoom * std::sin(_turnZ_radians), _zoom * std::cos(_turnZ_radians), CAM_ZADD_MAX);

    _turnZ_turns = id::semantic_cast<Ego::Math::Turns>(_turnZ_radians);
    _ori.facing_z = TurnToFacing(_turnZ_turns);
    resetView();

    // Assume that the camera is fullscreen.
    setScreen(0, 0, Ego::GraphicsSystem::get().window->getSize().width(), Ego::GraphicsSystem::get().window->getSize().height());
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

void Camera::updateProjection(const Ego::Math::Degrees& fov, const float aspect_ratio, const float frustum_near, const float frustum_far)
{
    m_projectionMatrix = Ego::Math::Transform::perspective(fov, aspect_ratio, frustum_near, frustum_far);

    // Invalidate the frustum.
    _frustumInvalid = true;
}

void Camera::resetView()
{
    // Check for stupidity.
    if (m_position != _center)
    {
        static const Vector3f Z = Vector3f(0.0f, 0.0f, 1.0f);
        Matrix4f4f tmp = Ego::Math::Transform::scaling(Vector3f(-1.0f, 1.0f, 1.0f)) *  Ego::Math::Transform::rotation(Z, Ego::Math::Degrees(Ego::Math::radToDeg(_roll)));
        m_viewMatrix = tmp * Ego::Math::Transform::lookAt(m_position, _center, Z);
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
        _turnZRad = Ego::Math::Radians(std::atan2(_center[kY] - pos[kY], _center[kX] - pos[kX]));  // xgg
        _turnZOne = Ego::Math::Turns(_turnZRad);
        _ori.facing_z = TurnsToFacing(_turnZ_turns);
    }
#endif

    // Update the camera position.
    Vector3f pos_new = _center + Vector3f(_zoom * std::sin(_ori.facing_z), _zoom * std::cos(_ori.facing_z), _zGoto);

    if(id::euclidean_norm(m_position-pos_new) < Info<float>::Grid::Size()*8.0f) {
        // Make the camera motion smooth using a low-pass filter
        m_position = m_position * 0.9f + pos_new * 0.1f; /// @todo Use Ego::Math::lerp.
    }
    else {
        //Teleport camera if error becomes too large
        m_position = pos_new;
        _center = _trackPos;
    }
}

void Camera::makeMatrix()
{
    resetView();

    // Pre-compute some camera vectors.
    m_forward = mat_getCamForward(m_viewMatrix);
    m_forward = normalize(m_forward).first;

    m_up = mat_getCamUp(m_viewMatrix);
    m_up = normalize(m_up).first;

    m_right = mat_getCamRight(m_viewMatrix);
    m_right = normalize(m_right).first;
}

void Camera::updateZoom()
{
    // Update zadd.
    _zaddGoto = Ego::Math::constrain(_zaddGoto, CAM_ZADD_MIN, CAM_ZADD_MAX);
    _zadd = 0.9f * _zadd  + 0.1f * _zaddGoto; /// @todo Use Ego::Math::lerp.

    // Update zoom.
    float percentmax = (_zaddGoto - CAM_ZADD_MIN) / static_cast<float>(CAM_ZADD_MAX - CAM_ZADD_MIN);
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
        int32_t newAngle = static_cast<int32_t>(static_cast<float>(FACING_T(_ori.facing_z)) +_turnZAdd);
        _ori.facing_z = Facing(uint16_t(Facing(newAngle)));

        _turnZ_turns = FacingToTurn(Facing(_ori.facing_z));
        _turnZ_radians = id::semantic_cast<Ego::Math::Radians>(_turnZ_turns);
    }
    _turnZAdd *= TURN_Z_SUSTAIN;
}

void Camera::updateCenter()
{
    // Center on target for doing rotation ...
    if (0 != _turnTime)
    {
        _center.x() = _center.x() * 0.9f + _trackPos.x() * 0.1f;
        _center.y() = _center.y() * 0.9f + _trackPos.y() * 0.1f;
    }
    else
    {
        // Determine tracking direction.
        Vector3f trackError = _trackPos - m_position;

        // Determine the size of the dead zone.
        Ego::Math::Degrees track_fov = DEFAULT_FOV * 0.25f;
        float track_dist = id::euclidean_norm(trackError);
        float track_size = track_dist * std::tan(track_fov);
        float track_size_x = track_size;
        float track_size_y = track_size;  /// @todo adjust this based on the camera viewing angle

        // Calculate the difference between the center of the tracked characters
        // and the center of the camera look to look at.
        Vector2f diff = Vector2f(_trackPos.x(), _trackPos.y()) - Vector2f(_center.x(), _center.y());

        // Get 2d versions of the camera's right and up vectors.
        Vector2f vrt(m_right.x(), m_right.y());
        vrt = normalize(vrt).first;

        Vector2f vup(m_up.x(), m_up.y());
        vup = normalize(vup).first;

        // project the diff vector into this space
        float diff_rt = dot(vrt, diff);
        float diff_up = dot(vup, diff);

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
        _center.x() += scroll.x();
        _center.y() += scroll.y();
    }

    // _center.z always approaches _trackPos.z
    _center.z() = _center.z() * 0.9f + _trackPos.z() * 0.1f; /// @todo Use Ego::Math::lerp
}

void Camera::updateFreeControl()
{
    auto& inputSystem = Ego::Input::InputSystem::get();
    float moveSpeed = 25.0f;
    if(inputSystem.isKeyDown(SDLK_LSHIFT) || inputSystem.isKeyDown(SDLK_RSHIFT)) {
        moveSpeed += 25.0f;
    }

    //Forward and backwards
    if (inputSystem.isKeyDown(SDLK_KP_2) || inputSystem.isKeyDown(SDLK_DOWN)) {
        _center.x() += std::sin(_turnZ_radians) * moveSpeed;
        _center.y() += std::cos(_turnZ_radians) * moveSpeed;
    }
    else if (inputSystem.isKeyDown(SDLK_KP_8) || inputSystem.isKeyDown(SDLK_UP)) {
        _center.x() -= std::sin(_turnZ_radians) * moveSpeed;
        _center.y() -= std::cos(_turnZ_radians) * moveSpeed;
    }
    
    //Left and right
    if (inputSystem.isKeyDown(SDLK_KP_4) || inputSystem.isKeyDown(SDLK_LEFT)) {
        _center.x() -= std::sin(_turnZ_radians + Ego::Math::Radians(id::pi<float>() * 0.5f)) * moveSpeed;
        _center.y() -= std::cos(_turnZ_radians + Ego::Math::Radians(id::pi<float>() * 0.5f)) * moveSpeed;
    }
    else if (inputSystem.isKeyDown(SDLK_KP_6) || inputSystem.isKeyDown(SDLK_RIGHT)) {
        _center.x() += std::sin(_turnZ_radians + Ego::Math::Radians(id::pi<float>() * 0.5f)) * moveSpeed;
        _center.y() += std::cos(_turnZ_radians + Ego::Math::Radians(id::pi<float>() * 0.5f)) * moveSpeed;
    }
    
    //Rotate left or right
    if (inputSystem.isKeyDown(SDLK_KP_7)) {
        _turnZAdd += DEFAULT_TURN_KEY * 2.0f;
    }
    else if (inputSystem.isKeyDown(SDLK_KP_9)) {
        _turnZAdd -= DEFAULT_TURN_KEY * 2.0f;
    }

    //Up and down
    if (inputSystem.isKeyDown(SDLK_KP_PLUS) || inputSystem.isKeyDown(SDLK_SPACE)) {
        _center.z() -= moveSpeed * 0.2f;
    }
    else if (inputSystem.isKeyDown(SDLK_KP_MINUS) || inputSystem.isKeyDown(SDLK_LCTRL)) {
        _center.z() += moveSpeed * 0.2f;
    }

    //Pitch camera
    if(inputSystem.isKeyDown(SDLK_PAGEDOWN)) {
        _pitch += Ego::Math::degToRad(7.5f);
    }
    else if(inputSystem.isKeyDown(SDLK_PAGEUP)) {
        _pitch -= Ego::Math::degToRad(7.5f);
    }

    //Constrain between 0 and 90 degrees pitch (and a little extra to avoid singularities)
    _pitch = Ego::Math::constrain(_pitch, 0.05f, id::pi<float>() - 0.05f);

    //Prevent the camera target from being below the mesh
    _center.z() = std::max(_center.z(), _currentModule->getMeshPointer()->getElevation(Vector2f(_center.x(), _center.y())));

    //Calculate camera position from desired zoom and rotation
    m_position.x() = _center.x() + _zoom * std::sin(_turnZ_radians);
    m_position.y() = _center.y() + _zoom * std::cos(_turnZ_radians);
    m_position.z() = _center.z() + _zoom * _pitch;

    //Prevent the camera from being below the mesh
    m_position.z() = std::max(m_position.z(), 180.0f + _currentModule->getMeshPointer()->getElevation(Vector2f(m_position.x(), m_position.y())));

    updateZoom();
    makeMatrix();

    _trackPos = _center;
}

void Camera::updateTrack()
{
    // The default camera motion is to do nothing.
    Vector3f new_track = _trackPos;

    switch(_moveMode)
    {

    // The camera is (re-)focuses in on a one or more objects.
    case CameraMovementMode::Reset:
        {
            float sum_wt    = 0.0f;
            float sum_level = 0.0f;
            Vector3f sum_pos = id::zero<Vector3f>();

            for(ObjectRef objectRef : _trackList)
            {
                const std::shared_ptr<Object>& object = _currentModule->getObjectHandler()[objectRef];
                if (!object || object->isTerminated() || !object->isAlive()) continue;

                sum_pos += object->getPosition() + Vector3f(0.0f, 0.0f, object->chr_min_cv._maxs[OCT_Z] * 0.9f);
                sum_level += object->getObjectPhysics().getGroundElevation();
                sum_wt += 1.0f;
            }

            // If any of the characters is doing anything.
            if (sum_wt > 0.0f)
            {
                new_track = sum_pos * (1.0f / sum_wt);
            }
        }

        // Reset the camera mode.
        _moveMode = CameraMovementMode::Player;
    break;

    // The camera is (re-)focuses in on a one or more player objects.
    // "Show me the drama!"
    case CameraMovementMode::Player:
        {
            std::vector<std::shared_ptr<Object>> trackedPlayers;

            // Count the number of local players, first.
            for(ObjectRef objectRef : _trackList)
            {
                const std::shared_ptr<Object> &object = _currentModule->getObjectHandler()[objectRef];
                if (!object || object->isTerminated() || !object->isAlive()) continue;

                trackedPlayers.push_back(object);
            }

            if (trackedPlayers.empty())
            {
                // Do nothing.
            }
            else if (1 == trackedPlayers.size())
            {
                // Copy from the one character.
                _trackPos = trackedPlayers[0]->getPosition();
            }
            else
            {
                // Use the characer's "activity" to average the position the camera is viewing.
                float sum_wt    = 0.0f;
                float sum_level = 0.0f;
                Vector3f sum_pos = id::zero<Vector3f>();

                for(const std::shared_ptr<Object> &pchr : trackedPlayers)
                {
                    // Weight it by the character's velocity^2, so that
                    // inactive characters don't control the camera.
                    float weight1 = dot(pchr->getVelocity(), pchr->getVelocity());

                    // Make another weight based on button-pushing.
                    float weight2 = pchr->isAnyLatchButtonPressed() ? 127 : 0;

                    // I would weight this by the amount of damage that the character just sustained,
                    // but there is no real way to do this?

                    // Get the maximum effect.
                    float weight = std::max(weight1, weight2);

                    // The character is on foot.
                    sum_pos += pchr->getPosition() * weight;
                    sum_level += (pchr->getObjectPhysics().getGroundElevation() + 128) * weight;
                    sum_wt += weight;
                }

                // If any of the characters is doing anything.
                if (sum_wt > 0.0f)
                {
                    new_track = sum_pos * (1.0f / sum_wt);
                }
            }
        }
        break;

    default:
        break;
    }


    if (CameraMovementMode::Player == _moveMode)
    {
        // Smoothly interpolate the camera tracking position.
        _trackPos = _trackPos * 0.9f + new_track * 0.1f;           /// @todo Use Ego::Math::lerp.
    }
    else
    {
        // Just set the position.
        _trackPos = new_track;
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
    for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList()) {
        readInput(player->getInputDevice());
    }

    // Update the special camera effects like swinging and blur
    updateEffects();

    // Update the average position of the tracked characters
    updateTrack();

    // Move the camera center, if need be.
    updateCenter();

    // Make the zadd and zoom work together.
    updateZoom();

    // Update the position of the camera.
    updatePosition();

    // Set the view matrix.
    makeMatrix();
}

void Camera::readInput(const Ego::Input::InputDevice &device)
{

    auto& inputSystem = Ego::Input::InputSystem::get();
    // Autoturn camera only works in single player and when it is enabled.
    bool autoturn_camera = (CameraTurnMode::Good == _turnMode) && (1 == local_stats.player_count);

    switch(device.getDeviceType())
    {
        // Mouse control
        case Ego::Input::InputDevice::InputDeviceType::MOUSE:
        {
            // Autoturn camera.
            if (autoturn_camera)
            {
                if (!device.isButtonPressed(Ego::Input::InputDevice::InputButton::CAMERA_CONTROL))
                {
                _turnZAdd -= inputSystem.getMouseMovement().x() * 0.5f;
                }
            }
            // Normal camera.
            else if (device.isButtonPressed(Ego::Input::InputDevice::InputButton::CAMERA_CONTROL))
            {
            _turnZAdd += inputSystem.getMouseMovement().x() / 3.0f;
            _zaddGoto += static_cast<float>(inputSystem.getMouseMovement().y()) / 3.0f;

                _turnTime = DEFAULT_TURN_TIME;  // Sticky turn ...
            }            
        }
        break;

        //TODO: Not implemented
        /*
        case INPUT_DEVICE_JOYSTICK:
        {
            // Joystick camera controls:

            int ijoy = type - INPUT_DEVICE_JOY;

            // Figure out which joystick this is.
        joystick_data_t *pjoy = inputSystem.joysticks[ijoy].get();

            // Autoturn camera.
            if (autoturn_camera)
            {
                if (!device.isButtonPressed(Ego::Input::InputDevice::InputButton::CONTROL_CAMERA))
                {
                    _turnZAdd -= pjoy->x * DEFAULT_TURN_JOY;
                }
            }
            // Normal camera.
        else if (input_device_t::control_active( pdevice, CONTROL_CAMERA))
            {
                _turnZAdd += pjoy->x * DEFAULT_TURN_JOY;
                _zaddGoto += pjoy->y * DEFAULT_TURN_JOY;

                _turnTime = DEFAULT_TURN_TIME;  // Sticky turn ...
            }
        }
        break;
        */

        // INPUT_DEVICE_KEYBOARD and any unknown device end up here
        case Ego::Input::InputDevice::InputDeviceType::KEYBOARD:
        default:
        {
            // Autoturn camera.
            if (autoturn_camera)
            {
                if (device.isButtonPressed( Ego::Input::InputDevice::InputButton::MOVE_LEFT))
                {
                    _turnZAdd += DEFAULT_TURN_KEY;
                }
                if (device.isButtonPressed( Ego::Input::InputDevice::InputButton::MOVE_RIGHT))
                {
                    _turnZAdd -= DEFAULT_TURN_KEY;
                }
            }
            // Normal camera.
            else
            {
                int _turn_z_diff = 0;

                // Rotation.
                if (device.isButtonPressed(Ego::Input::InputDevice::InputButton::CAMERA_LEFT))
                {
                    _turn_z_diff += DEFAULT_TURN_KEY;
                }
                if (device.isButtonPressed(Ego::Input::InputDevice::InputButton::CAMERA_RIGHT))
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
                if (device.isButtonPressed(Ego::Input::InputDevice::InputButton::CAMERA_ZOOM_OUT))
                {
                    _zaddGoto += DEFAULT_TURN_KEY;
                }
                if (device.isButtonPressed( Ego::Input::InputDevice::InputButton::CAMERA_ZOOM_IN))
                {
                    _zaddGoto -= DEFAULT_TURN_KEY;
                }
            }
        }        
        break;
    }
}

void Camera::reset(const ego_mesh_t *mesh)
{
    // Defaults.
    _zoom = CAM_ZOOM_AVG;
    _zadd = CAM_ZADD_AVG;
    _zaddGoto = CAM_ZADD_AVG;
    _zGoto = CAM_ZADD_AVG;
    _turnZ_radians = Ego::Math::Radians(-id::pi_over<float, 4>());
    _turnZAdd = 0.0f;
    _roll = 0.0f;

    // Derived values.
    _center.x()     = mesh->_tmem._edge_x * 0.5f;
    _center.y()     = mesh->_tmem._edge_y * 0.5f;
    _center.z()     = 0.0f;

    _trackPos = _center;
    m_position = _center;

    m_position.x() += _zoom * std::sin(_turnZ_radians);
    m_position.y() += _zoom * std::cos(_turnZ_radians);
    m_position.z() += CAM_ZADD_MAX;

    _turnZ_turns = id::semantic_cast<Ego::Math::Turns>(_turnZ_radians);
    _ori.facing_z = TurnToFacing(_turnZ_turns);

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
    _center.x() = _trackPos.x();
    _center.y() = _trackPos.y();

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
        _roll = TLT::get().sin(_swing) * local_swingamp;
        //mat_Multiply( _mView.v, mat_RotateY( tmp1.v, roll ), mat_Copy( tmp2.v, _mView.v ) );
    }
    // If the camera stops swinging for some reason, slowly return to _original position
    else if ( 0 != _roll )
    {
        _roll *= 0.9875f;            //Decay factor
        //mat_Multiply( _mView.v, mat_RotateY( tmp1.v, _roll ), mat_Copy( tmp2.v, _mView.v ) );

        // Come to a standstill at some point
        if ( std::abs( _roll ) < 0.001f )
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
    m_viewport->setLeftPixels(xmin);
    m_viewport->setTopPixels(ymin);
    m_viewport->setWidthPixels(xmax - xmin);
    m_viewport->setHeightPixels(ymax - ymin);

    // Update projection after setting size.
    float aspect_ratio = m_viewport->getWidthPixels()
                       / m_viewport->getHeightPixels();
    // The nearest we will have to worry about is 1/2 of a tile.
    float frustum_near = Info<int>::Grid::Size() * 0.25f;
    // Set the maximum depth to be the "largest possible size" of a mesh.
    float frustum_far  = Info<int>::Grid::Size() * 256 * id::sqrt_two<float>();
    updateProjection(DEFAULT_FOV, aspect_ratio, frustum_near, frustum_far);
}

void Camera::addTrackTarget(ObjectRef targetRef)
{
    //Make sure the target is valid
    const std::shared_ptr<Object>& object = _currentModule->getObjectHandler()[targetRef];
    if(!object) {
        return;
    }

    //Initialize camera position on spawn
    if(_trackList.empty()) {
        _trackPos = object->getPosition();
        _center = _trackPos;
        m_position = _center + Vector3f(_zoom * std::sin(_ori.facing_z), _zoom * std::cos(_ori.facing_z), _zGoto);
    }

    _trackList.push_front(targetRef);
}

void Camera::setPosition(const Vector3f &position)
{
    _center = position;
}

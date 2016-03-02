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

/// @file game/Logic/Player.cpp
/// @author Zefz aka Johan Jansen

#include "game/Logic/Player.hpp"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Entities/_Include.hpp"
#include "game/game.h"
#include "game/Core/GameEngine.hpp"
#include "game/GameStates/PlayingState.hpp"
#include "game/ObjectAnimation.h"

namespace Ego
{

Player::Player(const std::shared_ptr<Object>& object, input_device_t *pdevice) :
    _object(object),
    _unspentLevelUp(false),

    _currentCharge(0),
    _maxCharge(0),
    _chargeBarFrame(0),
    _chargeTick(0),

    _inventoryMode(false),
    _inventorySlot(0),
    _inventoryCooldown(0),

    _questLog(),

    _pdevice(pdevice),
    _localLatch()
{
    // initialize the latches
    _localLatch.clear();
}

std::shared_ptr<Object> Player::getObject() const
{
    return _object.lock();
}

input_device_t* Player::getInputDevice()
{
    return _pdevice;
}

Ego::QuestLog& Player::getQuestLog()
{
    return _questLog;
}

void Player::updateLatches()
{
    // is the device a local device or an internet device?
    input_device_t *pdevice = getInputDevice();
    if (nullptr == pdevice) {
        return;
    }

    //Ensure this player is controlling a valid object
    std::shared_ptr<Object> object = getObject();
    if(!object || object->isTerminated()) {
        return;
    }

    //No need to continue if device is not enabled
    if ( !input_device_is_enabled(pdevice) ) {
        return;
    }

    // find the camera that is following this character
    const auto &pcam = CameraSystem::get()->getCamera(object->getObjRef());
    if (!pcam) {
        return;
    }

    // fast camera turn if it is enabled and there is only 1 local player
    bool fast_camera_turn = ( 1 == local_stats.player_count ) && ( CameraTurnMode::Good == pcam->getTurnMode() );

    // Clear the player's latch buffers
    latch_t sum;
    sum.clear();

    Vector2f movementInput = Vector2f::zero();
    Vector2f joy_pos = Vector2f::zero();

    // generate the transforms relative to the camera
    // this needs to be changed for multicamera
    TLT::Index turnsin = TLT::get().fromFacing( pcam->getOrientation().facing_z );
    float fsin    = TLT::get().sin(turnsin);
    float fcos    = TLT::get().cos(turnsin);

    float scale;

    switch(pdevice->device_type)
    {
        // Mouse routines
        case INPUT_DEVICE_MOUSE:
            if (fast_camera_turn || !input_device_control_active(pdevice,  CONTROL_CAMERA))  // Don't allow movement in camera control mode
            {
                float dist = std::sqrt( mous.x * mous.x + mous.y * mous.y );
                if (dist > 0)
                {
                    scale = mous.sense / dist;
                    if ( dist < mous.sense )
                    {
                        scale = dist / mous.sense;
                    }

                    if ( mous.sense != 0 )
                    {
                        scale /= mous.sense;
                    }

                    joy_pos[XX] = mous.x * scale;
                    joy_pos[YY] = mous.y * scale;

                    //if ( fast_camera_turn && !input_device_control_active( pdevice,  CONTROL_CAMERA ) )  joy_pos.x = 0;

                    movementInput.x() = ( joy_pos[XX] * fcos + joy_pos[YY] * fsin );
                    movementInput.y() = ( -joy_pos[XX] * fsin + joy_pos[YY] * fcos );
                }
            }
        break;

        // Keyboard routines
        case INPUT_DEVICE_KEYBOARD:
            if ( fast_camera_turn || !input_device_control_active( pdevice, CONTROL_CAMERA ) )
            {
                if ( input_device_control_active( pdevice,  CONTROL_RIGHT ) )   joy_pos[XX]++;
                if ( input_device_control_active( pdevice,  CONTROL_LEFT ) )    joy_pos[XX]--;
                if ( input_device_control_active( pdevice,  CONTROL_DOWN ) )    joy_pos[YY]++;
                if ( input_device_control_active( pdevice,  CONTROL_UP ) )      joy_pos[YY]--;

                if ( fast_camera_turn )  joy_pos[XX] = 0;

                movementInput.x() = ( joy_pos[XX] * fcos + joy_pos[YY] * fsin );
                movementInput.y() = ( -joy_pos[XX] * fsin + joy_pos[YY] * fcos );
            }
        break;

        // Joystick routines
        default:
            if (IS_VALID_JOYSTICK(pdevice->device_type))
            {
                //Figure out which joystick we are using
                joystick_data_t *joystick;
                joystick = joy_lst + ( pdevice->device_type - MAX_JOYSTICK );

                if ( fast_camera_turn || !input_device_control_active( pdevice, CONTROL_CAMERA ) )
                {
                    joy_pos[XX] = joystick->x;
                    joy_pos[YY] = joystick->y;

                    float dist = joy_pos.length_2();
                    if ( dist > 1.0f )
                    {
                        scale = 1.0f / std::sqrt( dist );
                        joy_pos *= scale;
                    }

                    if ( fast_camera_turn && !input_device_control_active( pdevice, CONTROL_CAMERA ) )  joy_pos[XX] = 0;

                    movementInput.x() = ( joy_pos[XX] * fcos + joy_pos[YY] * fsin );
                    movementInput.y() = ( -joy_pos[XX] * fsin + joy_pos[YY] * fcos );
                }
            }
            else
            {
                // unknown device type.
                pdevice = nullptr;
                return;
            }
        break;
    }

    // Update movement (if any)
    sum.x += movementInput.x();
    sum.y += movementInput.y();

    // Read control buttons
    if (!_inventoryMode)
    {
        if ( input_device_control_active( pdevice, CONTROL_JUMP ) ) 
            sum.b[LATCHBUTTON_JUMP] = true;
        if ( input_device_control_active( pdevice, CONTROL_LEFT_USE ) )
            sum.b[LATCHBUTTON_LEFT] = true;
        if ( input_device_control_active( pdevice, CONTROL_LEFT_GET ) )
            sum.b[LATCHBUTTON_ALTLEFT] = true;
        if ( input_device_control_active( pdevice, CONTROL_RIGHT_USE ) )
            sum.b[LATCHBUTTON_RIGHT] = true;
        if ( input_device_control_active( pdevice, CONTROL_RIGHT_GET ) )
            sum.b[LATCHBUTTON_ALTRIGHT] = true;

        // Now update movement and input
        input_device_add_latch(pdevice, sum.x, sum.y);

        _localLatch.x = pdevice->latch.x;
        _localLatch.y = pdevice->latch.y;
        _localLatch.b = sum.b;
    }

    //inventory mode
    else if (_inventoryCooldown < update_wld)
    {
        int new_selected = _inventorySlot;

        //dirty hack here... mouse seems to be inverted in inventory mode?
        if (pdevice->device_type == INPUT_DEVICE_MOUSE)
        {
            joy_pos[XX] = -joy_pos[XX];
            joy_pos[YY] = -joy_pos[YY];
        }

        //handle inventory movement
        if ( joy_pos[XX] < 0 )       new_selected--;
        else if ( joy_pos[XX] > 0 )  new_selected++;

        //clip to a valid value
        if ( _inventorySlot != new_selected )
        {
            _inventoryCooldown = update_wld + 5;

            //Make inventory movement wrap around
            if(new_selected < 0) {
                new_selected = object->getInventory().getMaxItems() - 1;
            }
            else if(new_selected >= object->getInventory().getMaxItems()) {
                _inventorySlot = 0;
            }
            else {
                _inventorySlot = new_selected;
            }
        }

        //handle item control
        if ( object->inst.action_ready && 0 == object->reload_timer )
        {
            //handle LEFT hand control
            if ( input_device_control_active( pdevice, CONTROL_LEFT_USE ) || input_device_control_active(pdevice, CONTROL_LEFT_GET) )
            {
                //put it away and swap with any existing item
                Inventory::swap_item(object->getObjRef(), _inventorySlot, SLOT_LEFT, false);

                // Make it take a little time
                chr_play_action(object.get(), ACTION_MG, false);
                object->reload_timer = Inventory::PACKDELAY;
            }

            //handle RIGHT hand control
            if ( input_device_control_active( pdevice, CONTROL_RIGHT_USE) || input_device_control_active( pdevice, CONTROL_RIGHT_GET) )
            {
                // put it away and swap with any existing item
                Inventory::swap_item(object->getObjRef(), _inventorySlot, SLOT_RIGHT, false);

                // Make it take a little time
                chr_play_action(object.get(), ACTION_MG, false);
                object->reload_timer = Inventory::PACKDELAY;
            }
        }

        //empty any movement
        _localLatch.x = 0;
        _localLatch.y = 0;
    }

    //enable inventory mode?
    if ( update_wld > _inventoryCooldown && input_device_control_active( pdevice, CONTROL_INVENTORY ) )
    {
        for(uint8_t ipla = 0; ipla < _currentModule->getPlayerList().size(); ++ipla) {
            if(_currentModule->getPlayer(ipla).get() == this) {
                _gameEngine->getActivePlayingState()->displayCharacterWindow(ipla);
                _inventoryCooldown = update_wld + ( ONESECOND / 4 );
                break;
            }
        }
    }

    //Enter or exit stealth mode?
    if(input_device_control_active(pdevice, CONTROL_SNEAK) && update_wld > _inventoryCooldown) {
        if(!object->isStealthed()) {
            object->activateStealth();
        }
        else {
            object->deactivateStealth();
        }
        _inventoryCooldown = update_wld + ONESECOND;
    }

    //Finally, actually copy player latch into the object
    object->latch = _localLatch;
}

void Player::setLatch(size_t latch, bool value)
{
    _localLatch.b[latch] = value;
}

void Player::setChargeBar(const uint32_t currentCharge, const uint32_t maxCharge, const uint32_t chargeTick)
{
    _maxCharge = maxCharge;
    _currentCharge = Ego::Math::constrain<uint32_t>(currentCharge, 0, maxCharge);
    _chargeTick = chargeTick;
    _chargeBarFrame = _gameEngine->getCurrentUpdateFrame() + 10;    
}

bool Player::hasUnspentLevel() const
{
    return _unspentLevelUp;
}

void Player::setLevelUpIndicator(const bool hasLevelUp)
{
    _unspentLevelUp = hasLevelUp;
}

void Player::setInventoryMode(const bool inventoryMode)
{
    _inventoryMode = inventoryMode;
}

uint8_t Player::getSelectedInventorySlot() const
{
    return _inventorySlot;
}

void Player::setSelectedInventorySlot(const uint8_t slot)
{
    _inventorySlot = slot;
}

uint32_t Player::getBarPipWidth() const
{
    return _chargeTick;
}

uint32_t Player::getBarCurrentCharge() const
{
    return _currentCharge;
}

uint32_t Player::getBarMaxCharge() const
{
    return _maxCharge;
}

uint32_t Player::getChargeBarFrame() const
{
    return _chargeBarFrame;
}

} //namespace Ego
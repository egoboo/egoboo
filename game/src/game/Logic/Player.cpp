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
    _localLatch(),
    _tlatch_count(),
    _tlatch(),
    _net_latch()
{
    // initialize the latches
    _localLatch.clear();
    _net_latch.clear();

    // initialize the tlatch array
    tlatch_ary_init(_tlatch, MAXLAG);
}

std::shared_ptr<Object> Player::getObject() const
{
    return _object.lock();
}

input_device_t* Player::getInputDevice()
{
    return _pdevice;
}

std::unordered_map<IDSZ2, int>& Player::getQuestLog()
{
    return _questLog;
}

void Player::updateLatches()
{
    /// @author ZZ
    /// @details This function converts input readings to latch settings, so players can
    ///    move around

    // is the device a local device or an internet device?
    input_device_t *pdevice = getInputDevice();
    if (nullptr == pdevice) return;

    //No need to continue if device is not enabled
    if ( !input_device_is_enabled( pdevice ) ) return;

    // find the camera that is pointing at this character
    auto pcam = CameraSystem::get()->getCamera(getObject()->getObjRef());
    if (!pcam) return;

    // fast camera turn if it is enabled and there is only 1 local player
    bool fast_camera_turn = ( 1 == local_stats.player_count ) && ( CameraTurnMode::Good == pcam->getTurnMode() );

    // Clear the player's latch buffers
    latch_t sum;
    sum.clear();

    Vector2f joy_new = Vector2f::zero(),
             joy_pos = Vector2f::zero();

    // generate the transforms relative to the camera
    // this needs to be changed for multicamera
    TURN_T turnsin = TO_TURN( pcam->getOrientation().facing_z );
    float fsin    = turntosin[turnsin];
    float fcos    = turntocos[turnsin];

    float scale;
    if ( INPUT_DEVICE_MOUSE == pdevice->device_type )
    {
        // Mouse routines

        if ( fast_camera_turn || !input_device_control_active( pdevice,  CONTROL_CAMERA ) )  // Don't allow movement in camera control mode
        {
            float dist = std::sqrt( mous.x * mous.x + mous.y * mous.y );
            if ( dist > 0 )
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

                joy_new[XX] = ( joy_pos[XX] * fcos + joy_pos[YY] * fsin );
                joy_new[YY] = ( -joy_pos[XX] * fsin + joy_pos[YY] * fcos );
            }
        }
    }

    else if ( INPUT_DEVICE_KEYBOARD == pdevice->device_type )
    {
        // Keyboard routines

        if ( fast_camera_turn || !input_device_control_active( pdevice, CONTROL_CAMERA ) )
        {
            if ( input_device_control_active( pdevice,  CONTROL_RIGHT ) )   joy_pos[XX]++;
            if ( input_device_control_active( pdevice,  CONTROL_LEFT ) )    joy_pos[XX]--;
            if ( input_device_control_active( pdevice,  CONTROL_DOWN ) )    joy_pos[YY]++;
            if ( input_device_control_active( pdevice,  CONTROL_UP ) )      joy_pos[YY]--;

            if ( fast_camera_turn )  joy_pos[XX] = 0;

            joy_new[XX] = ( joy_pos[XX] * fcos + joy_pos[YY] * fsin );
            joy_new[YY] = ( -joy_pos[XX] * fsin + joy_pos[YY] * fcos );
        }
    }
    else if ( IS_VALID_JOYSTICK( pdevice->device_type ) )
    {
        // Joystick routines

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

            joy_new[XX] = ( joy_pos[XX] * fcos + joy_pos[YY] * fsin );
            joy_new[YY] = ( -joy_pos[XX] * fsin + joy_pos[YY] * fcos );
        }
    }

    else
    {
        // unknown device type.
        pdevice = nullptr;
    }

    // Update movement (if any)
    sum.x += joy_new[XX];
    sum.y += joy_new[YY];

    // Read control buttons
    if ( !_inventoryMode )
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
        input_device_add_latch( pdevice, sum.x, sum.y );

        _localLatch.x = pdevice->latch.x;
        _localLatch.y = pdevice->latch.y;
        _localLatch.b = sum.b;
    }

    //inventory mode
    else if (_inventoryCooldown < update_wld)
    {
        int new_selected = _inventorySlot;
        std::shared_ptr<Object> pchr = getObject();

        //dirty hack here... mouse seems to be inverted in inventory mode?
        if ( pdevice->device_type == INPUT_DEVICE_MOUSE )
        {
            joy_pos[XX] = - joy_pos[XX];
            joy_pos[YY] = - joy_pos[YY];
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
                new_selected = pchr->getInventory().getMaxItems() - 1;
            }
            else if(new_selected >= pchr->getInventory().getMaxItems()) {
                _inventorySlot = 0;
            }
            else {
                _inventorySlot = new_selected;
            }
        }

        //handle item control
        if ( pchr->inst.action_ready && 0 == pchr->reload_timer )
        {
            //handle LEFT hand control
            if ( input_device_control_active( pdevice, CONTROL_LEFT_USE ) || input_device_control_active(pdevice, CONTROL_LEFT_GET) )
            {
                //put it away and swap with any existing item
                Inventory::swap_item(pchr->getObjRef(), _inventorySlot, SLOT_LEFT, false);

                // Make it take a little time
                chr_play_action(pchr.get(), ACTION_MG, false);
                pchr->reload_timer = PACKDELAY;
            }

            //handle RIGHT hand control
            if ( input_device_control_active( pdevice, CONTROL_RIGHT_USE) || input_device_control_active( pdevice, CONTROL_RIGHT_GET) )
            {
                // put it away and swap with any existing item
                Inventory::swap_item(pchr->getObjRef(), _inventorySlot, SLOT_RIGHT, false);

                // Make it take a little time
                chr_play_action(pchr.get(), ACTION_MG, false);
                pchr->reload_timer = PACKDELAY;
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
        if(!getObject()->isStealthed()) {
            getObject()->activateStealth();
        }
        else {
            getObject()->deactivateStealth();
        }
        _inventoryCooldown = update_wld + ONESECOND;
    }

    //TODO: Remove this? (used to be part of latch networking)
    int index = _tlatch_count;
    if ( index < MAXLAG )
    {
        time_latch_t &ptlatch = _tlatch[index];

        ptlatch.button = _localLatch.b.to_ulong();

        // reduce the resolution of the motion to match the network packets
        ptlatch.x = std::floor(_localLatch.x * SHORTLATCH ) / SHORTLATCH;
        ptlatch.y = std::floor(_localLatch.y * SHORTLATCH ) / SHORTLATCH;

        ptlatch.time = update_wld;

        _tlatch_count++;
    }
}

void Player::setLatch(size_t latch, bool value)
{
    _localLatch.b[latch] = value;
}

//--------------------------------------------------------------------------------------------
void Player::net_unbuffer_player_latches()
{
    /// @author ZZ
    /// @details This function sets character latches based on player input to the host

    // get the "network" latch for each valid player
    for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList())
    {
        time_latch_t *tlatch_list = player->_tlatch;

        // copy the latch from last time
        latch_t tmp_latch = player->_net_latch;

        // what are the minimum and maximum indices that can be applies this update?
        Uint32 tnc;
        for (tnc = 0; tnc < player->_tlatch_count; ++tnc)
        {
            int dt;

            dt = update_wld - tlatch_list[tnc].time;

            if (dt < 0)
                break;
        }
        Uint32 latch_count = tnc;

        if ( 1 == latch_count )
        {
            // there is just one valid latch
            tmp_latch.x = tlatch_list[0].x;
            tmp_latch.y = tlatch_list[0].y;
            tmp_latch.b = tlatch_list[0].button;

            //log_info( "<<%1.4f, %1.4f>, 0x%x>, Just one latch for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, _currentModule->getObjectHandler().get(ppla->index)->Name );
        }
        else if ( latch_count > 1 )
        {
            int weight, weight_sum;
            int dt;

            // estimate the best latch value by weighting latches that are back in time
            // by dt*dt. This estimates the effect of actually integrating the position over
            // that much time without the hastle of actually integrating the trajectory.

            // blank the current latch so that we can sum the latch values
            tmp_latch.clear();

            // apply the latch
            weight_sum = 0;
            for ( tnc = 0; tnc < latch_count; tnc++ )
            {
                dt = update_wld - tlatch_list[tnc].time;

                weight      = ( dt + 1 ) * ( dt + 1 );

                weight_sum  += weight;
                tmp_latch.x += tlatch_list[tnc].x * weight;
                tmp_latch.y += tlatch_list[tnc].y * weight;
                SET_BIT( tmp_latch.b, tlatch_list[tnc].button );
            }

            if ( weight_sum > 0.0f )
            {
                tmp_latch.x /= ( float )weight_sum;
                tmp_latch.y /= ( float )weight_sum;
            }

            //log_info( "<<%1.4f, %1.4f>, 0x%x>, %d, multiple latches for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, latch_count, _currentModule->getObjectHandler().get(ppla->index)->Name );
        }
        else
        {
            // there are no valid latches
            // do nothing. this lets the old value of the latch persist.
            // this might be a decent guess as to what to do if a packet was
            // dropped?
            //log_info( "<<%1.4f, %1.4f>, 0x%x>, latch dead reckoning for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, _currentModule->getObjectHandler().get(ppla->index)->Name );
        }

        if ( latch_count >= player->_tlatch_count )
        {
            // we have emptied all of the latches
            player->_tlatch_count = 0;
        }
        else if ( latch_count > 0 )
        {
            int index;

            // concatenate the list
            for ( tnc = latch_count, index = 0; tnc < player->_tlatch_count; tnc++, index++ )
            {
                tlatch_list[index].x      = tlatch_list[tnc].x;
                tlatch_list[index].y      = tlatch_list[tnc].y;
                tlatch_list[index].button = tlatch_list[tnc].button;
                tlatch_list[index].time   = tlatch_list[tnc].time;
            }
            player->_tlatch_count = index;
        }

        // fix the network latch
        player->_net_latch = tmp_latch;
    }

    // set the player latch
    for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList())
    {
        player->getObject()->latch = player->_net_latch;
    }

    // Let players respawn
    for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList())
    {
        const std::shared_ptr<Object> &pchr = player->getObject();
        if(pchr->isTerminated()) {
            continue;
        }

        if (egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Hard && pchr->latch.b[LATCHBUTTON_RESPAWN] && _currentModule->isRespawnValid())
        {
            if ( !pchr->isAlive() && 0 == local_stats.revivetimer )
            {
                pchr->respawn();
                _currentModule->getTeamList()[pchr->team].setLeader(pchr);
                SET_BIT( pchr->ai.alert, ALERTIF_CLEANEDUP );

                // cost some experience for doing this...  never lose a level
                pchr->experience *= EXPKEEP;
                if (egoboo_config_t::get().game_difficulty.getValue() > Ego::GameDifficulty::Easy) pchr->money *= EXPKEEP;
            }

            // remove all latches other than latchbutton_respawn
            pchr->latch.b[LATCHBUTTON_RESPAWN] = 0;
        }
    }
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
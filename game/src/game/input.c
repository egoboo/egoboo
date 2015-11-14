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

/// @file game/input.c
/// @brief Keyboard, mouse, and joystick handling code.
/// @details

#include "game/input.h"

#include "game/network.h"
#include "game/graphic.h"
#include "game/egoboo.h"

//--------------------------------------------------------------------------------------------

// Raw input devices
mouse_data_t mous;
keyboard_data_t keyb;
joystick_data_t joy_lst[MAX_JOYSTICK];

//--------------------------------------------------------------------------------------------

void InputSystem::init_keyboard() {
    keyb.init();
    keyb.on = true;
}

void InputSystem::init_mouse() {
    mous.init();
    mous.on = true;
}

void InputSystem::init_joysticks() {
    for (size_t i = 0; i < MAX_JOYSTICK; ++i) {
        joy_lst[i].init();
		if (i < SDL_NumJoysticks()) {
			joy_lst[i].sdl_ptr = SDL_JoystickOpen(i);
			joy_lst[i].on = (NULL != joy_lst[i].sdl_ptr);
		}
    }
}

void InputSystem::init_devices() {
    for (size_t i = 0; i < MAX_LOCAL_PLAYERS; ++i) {
        InputDevices.lst[i].clear();
    }
    InputDevices.count = 0;
}

void InputSystem::initialize() {
    init_keyboard();
    init_mouse();
    init_joysticks();
    init_devices();
}

void InputSystem::uninitialize() {
}

void InputSystem::read_mouse()
{
    int x, y, b;

    b = SDL_GetRelativeMouseState( &x, &y );

    //Move mouse to the center of the screen since SDL does not detect motion outside the window
    //if (!egoboo_config_t::get().debug_developerMode_enable.getValue())
    //    SDL_WarpMouseInWindow(sdl_scr.window, GFX_WIDTH >> 1, GFX_HEIGHT >> 1);

    mous.x = -x; // mous.x and mous.y are the wrong type to use in above call
    mous.y = -y;
    mous.button[0] = ( b & SDL_BUTTON( 1 ) ) ? 1 : 0;
    mous.button[1] = ( b & SDL_BUTTON( 3 ) ) ? 1 : 0;
    mous.button[2] = ( b & SDL_BUTTON( 2 ) ) ? 1 : 0; // Middle is 2 on SDL
    mous.button[3] = ( b & SDL_BUTTON( 4 ) ) ? 1 : 0;

    // Mouse mask
    mous.b = ( mous.button[3] << 3 ) | ( mous.button[2] << 2 ) | ( mous.button[1] << 1 ) | ( mous.button[0] << 0 );
}

void InputSystem::read_keyboard()
{
    keyb.state_ptr = SDL_GetKeyboardState( &( keyb.state_size ) );
}

void InputSystem::read_joystick( int which )
{
    int dead_zone = 0x8000 / 10;
    int i, button_count, x, y;
    joystick_data_t * pjoy;

    if ( !joy_lst[which].on ) return;

    pjoy = joy_lst + which;

    // get the raw values
    x = SDL_JoystickGetAxis( pjoy->sdl_ptr, 0 );
    y = SDL_JoystickGetAxis( pjoy->sdl_ptr, 1 );

    // make a dead zone
    if ( x > dead_zone ) x -= dead_zone;
    else if ( x < -dead_zone ) x += dead_zone;
    else x = 0;

    if ( y > dead_zone ) y -= dead_zone;
    else if ( y < -dead_zone ) y += dead_zone;
    else y = 0;

    // store the values
    pjoy->x = x / ( float )( 0x8000 - dead_zone );
    pjoy->y = y / ( float )( 0x8000 - dead_zone );

    // get buttons
    button_count = SDL_JoystickNumButtons( pjoy->sdl_ptr );
    button_count = std::min( JOYBUTTON, button_count );
    for ( i = 0; i < button_count; i++ )
    {
        pjoy->button[i] = SDL_JoystickGetButton( pjoy->sdl_ptr, i );
    }

    // buttonmask mask
    pjoy->b = 0;
    for ( i = 0; i < button_count; i++ )
    {
        SET_BIT( pjoy->b, pjoy->button[i] << i );
    }

    return;
}

void InputSystem::read_joysticks()
{
    int cnt;

    SDL_JoystickUpdate();
    for ( cnt = 0; cnt < MAX_JOYSTICK; cnt++ )
    {
        InputSystem::read_joystick( cnt );
    }
}

//--------------------------------------------------------------------------------------------
// implementation of specialized input_device_commands
//--------------------------------------------------------------------------------------------
BIT_FIELD input_device_get_buttonmask( input_device_t *pdevice )
{
    // assume the worst
    BIT_FIELD buttonmask = EMPTY_BIT_FIELD;

    // make sure the idevice is valid
    if ( NULL == pdevice ) return EMPTY_BIT_FIELD;

    // scan for devices that use buttons
    if ( INPUT_DEVICE_MOUSE == pdevice->device_type )
    {
        buttonmask = mous.b;
    }
    else if ( IS_VALID_JOYSTICK( pdevice->device_type ) )
    {
        int ijoy = pdevice->device_type - INPUT_DEVICE_JOY;

        buttonmask = joy_lst[ijoy].b;
    }

    return buttonmask;
}

//--------------------------------------------------------------------------------------------
bool input_device_is_enabled(input_device_t *self)
{
    // assume the worst
    bool retval = false;

    // make sure the idevice is valid
    if (!self) return false;

    if ( INPUT_DEVICE_KEYBOARD == self->device_type )
    {
        retval = TO_C_BOOL( keyb.on );
    }
    else if ( INPUT_DEVICE_MOUSE == self->device_type )
    {
        retval = TO_C_BOOL( mous.on );
    }
    else if ( IS_VALID_JOYSTICK( self->device_type ) )
    {
        int ijoy = self->device_type - INPUT_DEVICE_JOY;

        retval = TO_C_BOOL( joy_lst[ijoy].on );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool input_device_control_active( input_device_t *pdevice, CONTROL_BUTTON icontrol )
{
    /// @author ZZ
    /// @details This function returns true if the given icontrol is pressed...

	// make sure the idevice is valid
    if ( NULL == pdevice ) return false;
    const control_t &pcontrol = pdevice->keyMap[icontrol];

    // if no control information was loaded, it can't be pressed
    if ( !pcontrol.loaded ) return false;

    // test for bits
    if ( pcontrol.tag_bits.any() )
    {
        BIT_FIELD bmask = input_device_get_buttonmask( pdevice );

        if ( !HAS_ALL_BITS( bmask, pcontrol.tag_bits.to_ulong() ) )
        {
            return false;
        }
    }

    // how many tags does this control have?
    for(uint32_t keycode : pcontrol.mappedKeys)
    {
        if (!keyb.is_key_down(keycode))
        {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mouse_data_t::mouse_data_t()
	: on(true), sense(12), x(0), y(0), b(0),
	  button{ 0, 0, 0, 0 } {
}

void mouse_data_t::init() {
    on = true;
    sense = 12;
    x = y = 0.0f;
    b = 0;

    for (size_t i = 0; i < 4; ++i) {
        button[i] = 0;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
keyboard_data_t::keyboard_data_t()
	: on(false), chat_mode(false), chat_done(false), state_size(0), state_ptr(nullptr) {
}

void keyboard_data_t::init() {
    on         = false;
    chat_mode  = false;
    chat_done  = false;
    state_size = 0;
    state_ptr  = NULL;
}

bool keyboard_data_t::is_key_down(int key) const {
    int k = SDL_GetScancodeFromKey(key);
    return !chat_mode && state_ptr && k < state_size && state_ptr[k];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
joystick_data_t::joystick_data_t()
	: on(false), x(0.0f), y(0.0f), b(0), sdl_ptr(nullptr), button() {

}
void joystick_data_t::init() {
    on = false;
    x = y = 0.0f;
    b = 0;
    sdl_ptr = NULL;

    for (size_t i = 0; i < JOYBUTTON; ++i) {
        button[i] = 0;
    }
}

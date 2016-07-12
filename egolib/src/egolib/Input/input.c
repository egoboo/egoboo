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

/// @file egolib/Input/input.c
/// @brief Keyboard, mouse, and joystick handling code.

#include "egolib/Input/input.h"
#include "egolib/Input/input_device.h"

//--------------------------------------------------------------------------------------------

constexpr int InputSystem::MaximumNumberOfJoysticks;

void InputSystem::init_joysticks() {
    for (int i = 0, n = std::max(SDL_NumJoysticks(),InputSystem::MaximumNumberOfJoysticks); i < n; ++i) {
        joysticks[i] = std::make_unique<joystick_data_t>(i);
        // Mark the joystick as connected.
        joysticks[i]->setConnected(true);
    }
}

InputSystem::InputSystem() : mouse(), keyboard() {
    mouse.setConnected(true);
    mouse.open();
    keyboard.setConnected(true);
    keyboard.open();
    init_joysticks();
}

InputSystem::~InputSystem() {
}

Vector2f mouse_data_t::readMouseOffset() {
    int x, y;
    SDL_GetRelativeMouseState(&x, &y);
    return Vector2f(float(x), float(y));
}

Point2f mouse_data_t::readMousePosition() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return Point2f(float(x), float(y));
}

void mouse_data_t::open() {

}

void mouse_data_t::close() {

}

bool mouse_data_t::getOpen() const {
    return getConnected();
}

void mouse_data_t::update()
{
    // If the mouse is not enabled or not open, do not update the mouse.
    if (!this->enabled || !this->getOpen()) return;

    // Read the mouse offset.
    // For some reason this offset is negated.
    this->offset = -readMouseOffset();
    // Read the mouse position.
    this->position = readMousePosition();
    // Read the mouse button state.
    int buttonState_backend = SDL_GetRelativeMouseState(nullptr, nullptr);
    this->button[0] = (b & SDL_BUTTON(1)) ? 1 : 0;
    this->button[1] = (b & SDL_BUTTON(3)) ? 1 : 0;
    this->button[2] = (b & SDL_BUTTON(2)) ? 1 : 0; // Middle is 2 on SDL
    this->button[3] = (b & SDL_BUTTON(4)) ? 1 : 0;

    // Mouse mask
    this->b = ( this->button[3] << 3 ) 
            | ( this->button[2] << 2 ) 
            | ( this->button[1] << 1 ) 
            | ( this->button[0] << 0 );
}

void keyboard_data_t::update()
{
    // If the keyboard is not enabled or not open, do not update the keyboard.
    if (!this->enabled || !this->getOpen()) return;

    // (1) Get the keyboard state.
    this->state_ptr = SDL_GetKeyboardState( &( this->state_size ) );

    // (2) Get and translate the modifier keys state.
    auto backendModifierKeys = SDL_GetModState();
    this->modifierKeys = Ego::ModifierKeys();
    // NUMLOCK.
    if (SDL_Keymod::KMOD_NUM == (backendModifierKeys & SDL_Keymod::KMOD_NUM)) {
        modifierKeys |= Ego::ModifierKeys::Num;
    }
    // LGUI and RGUI
    if (SDL_Keymod::KMOD_LGUI == (backendModifierKeys & SDL_Keymod::KMOD_LGUI)) {
        modifierKeys |= Ego::ModifierKeys::LeftGui;
    }
    if (SDL_Keymod::KMOD_RGUI == (backendModifierKeys & SDL_Keymod::KMOD_RGUI)) {
        modifierKeys |= Ego::ModifierKeys::RightGui;
    }
    // LSHIFT and RSHIFT
    if (SDL_Keymod::KMOD_LSHIFT == (backendModifierKeys & SDL_Keymod::KMOD_LSHIFT)) {
        modifierKeys |= Ego::ModifierKeys::LeftShift;
    }
    if (SDL_Keymod::KMOD_RSHIFT == (backendModifierKeys & SDL_Keymod::KMOD_RSHIFT)) {
        modifierKeys |= Ego::ModifierKeys::RightShift;
    }
    // CAPS
    if (SDL_Keymod::KMOD_CAPS == (backendModifierKeys & SDL_Keymod::KMOD_CAPS)) {
        modifierKeys |= Ego::ModifierKeys::Caps;
    }
    // LCTRL and RCTRL
    if (SDL_Keymod::KMOD_LCTRL == (backendModifierKeys & SDL_Keymod::KMOD_LCTRL)) {
        modifierKeys |= Ego::ModifierKeys::LeftControl;
    }
    if (SDL_Keymod::KMOD_RCTRL == (backendModifierKeys & SDL_Keymod::KMOD_RCTRL)) {
        modifierKeys |= Ego::ModifierKeys::RightControl;
    }
    // LALT and RALT
    if (SDL_Keymod::KMOD_LALT == (backendModifierKeys & SDL_Keymod::KMOD_LALT)) {
        modifierKeys |= Ego::ModifierKeys::LeftAlt;
    }
    if (SDL_Keymod::KMOD_RALT == (backendModifierKeys & SDL_Keymod::KMOD_RALT)) {
        modifierKeys |= Ego::ModifierKeys::RightAlt;
    }
}

void joystick_data_t::update()
{
    static const int dead_zone = 0x8000 / 10; // 32768 / 10 = 327

    // If the joystick is not enabled or not open, do not update the joystick.
    if (!this->enabled || !this->getOpen()) return;

    // Get the raw values.
    int x = SDL_JoystickGetAxis( this->sdl_ptr, 0 );
    int y = SDL_JoystickGetAxis( this->sdl_ptr, 1 );

    // Make a dead zone.
    if ( x > dead_zone ) x -= dead_zone;
    else if ( x < -dead_zone ) x += dead_zone;
    else x = 0;

    if ( y > dead_zone ) y -= dead_zone;
    else if ( y < -dead_zone ) y += dead_zone;
    else y = 0;

    // Store the values.
    this->x = x / ( float )( 0x8000 - dead_zone );
    this->y = y / ( float )( 0x8000 - dead_zone );

    // Get buttons.
    int button_count;
    button_count = SDL_JoystickNumButtons( this->sdl_ptr );
    button_count = std::min( JOYBUTTON, button_count );
    for (int i = 0; i < button_count; i++ )
    {
        this->button[i] = SDL_JoystickGetButton(this->sdl_ptr, i );
    }

    // Compute the button mask.
    this->b = 0;
    for (int i = 0; i < button_count; i++ )
    {
        SET_BIT(this->b, this->button[i] << i );
    }
}

void InputSystem::update()
{
    SDL_Event event;
    int result;
    SDL_PumpEvents();
    // Check if joysticks were plugged out.
    result = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_JOYDEVICEREMOVED, SDL_JOYDEVICEREMOVED);
    while (result == 1) {
        int instanceId = event.jdevice.which;
        for (auto& joystick : joysticks) {
            // If the joystick has the proper instance ID ...
            if (joystick->sdl_ptr != nullptr) {
                if (instanceId == SDL_JoystickInstanceID(joystick->sdl_ptr)) {
                    // Close it if it is opened.
                    if (joystick->getOpen()) {
                        joystick->close();
                    }
                    // Mark it as disconnected.
                    joystick->setConnected(false);
                }
            }
        }
        result = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_JOYDEVICEREMOVED, SDL_JOYDEVICEREMOVED);
    }
    if (result < 0) {
        /* @todo Emit a warning. */
    }
    // Check if joysticks were plugged in.
    result = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_JOYDEVICEADDED, SDL_JOYDEVICEADDED);
    while (result == 1) {
        int deviceIndex = event.jdevice.which;
        if (0 <= deviceIndex && deviceIndex < joysticks.size()) {
            // Mark the device as connected.
            joysticks[deviceIndex]->setConnected(true);
            // Open the device.
            joysticks[deviceIndex]->open();
        }
        result = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_JOYDEVICEADDED, SDL_JOYDEVICEADDED);
    }
    if (result < 0) {
        /* @todo Emit a warning. */
    }
    SDL_JoystickUpdate();
    for (auto& joystick : joysticks) {
        joystick->update();
    }
    mouse.update();
    keyboard.update();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mouse_data_t::mouse_data_t()
    : Device(Device::Kind::Mouse), sense(12), offset{0,0}, b(0),
	  button{ 0, 0, 0, 0 } {
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
keyboard_data_t::keyboard_data_t()
	: Device(Device::Kind::Keyboard), chat_mode(false), chat_done(false), state_size(0), state_ptr(nullptr) {
}

void keyboard_data_t::open() {

}

void keyboard_data_t::close() {

}

bool keyboard_data_t::getOpen() const {
    return getConnected();
}

bool keyboard_data_t::isKeyDown(int key) const {
    int k = SDL_GetScancodeFromKey(key);
    return !chat_mode && state_ptr && k < state_size && state_ptr[k];
}

bool keyboard_data_t::isKeyUp(int key) const {
    return !isKeyDown(key);
}

Ego::ModifierKeys keyboard_data_t::getModifierKeys() const {
    return modifierKeys;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
joystick_data_t::joystick_data_t(int sdl_device_index)
	: Device(Device::Kind::Joystick), sdl_device_index(sdl_device_index), x(0.0f), y(0.0f), b(0), sdl_ptr(nullptr), button() {
}

void joystick_data_t::open() {
    if (!getConnected()) {
        return;
    }
    if (!getOpen()) {
        sdl_ptr = SDL_JoystickOpen(sdl_device_index);
    }
}

void joystick_data_t::close() {
    if (!getConnected()) {
        return;
    }
    if (getOpen()) {
        SDL_JoystickClose(sdl_ptr);
        sdl_ptr = nullptr;
    }
}

bool joystick_data_t::getOpen() const {
    return nullptr != sdl_ptr;
}

joystick_data_t::~joystick_data_t() {
    if (nullptr != sdl_ptr) {
        SDL_JoystickClose(sdl_ptr);
        sdl_ptr = nullptr;
    }
}

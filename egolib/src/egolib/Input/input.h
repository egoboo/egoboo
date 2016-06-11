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

/// @file egolib/Input/input.h
/// @details Keyboard, mouse, and joystick handling code.

#pragma once

#include "egolib/Math/_Include.hpp"
#include "egolib/Core/Singleton.hpp"
#include "egolib/Input/Device.hpp"
#include "egolib/Input/ModifierKeys.hpp"

//--------------------------------------------------------------------------------------------

/// The internal representation of a state of a mouse.
struct mouse_data_t : public Device
{
    /**
     * @brief The sensitivity threshold.
     * @invariant non-negative
     * @default 12
     */
    float sense;

private:
    /**
     * @brief The mouse offset (difference between the current position and the last position).
     */
    Vector2f offset;
    /**
     * @brief The mouse position.
     */
    Point2f position;

public:
    uint8_t   button[4];       ///< Mouse button states
    uint32_t  b;               ///< Button masks

protected:
    /// Construct this mouse device.
	mouse_data_t();
    // Befriend with input system.
    friend struct InputSystem;
    void open() override;
    void close() override;
    bool getOpen() const override;
    void update() override;

public:
    /// @brief Get the mouse position.
    /// @return the mouse position
    Point2f getPosition() const { return position; }
    /// @brief Get the mouse offset (difference between the current position and the last position).
    /// @return the mouse offset
    Vector2f getOffset() const { return offset; }

private:
    /// @brief Read the mouse offset (difference between current position and last position).
    /// @return the mouse offset
    static Vector2f readMouseOffset();
    /// @brief Read the mouse position.
    /// @return the mouse position
    static Point2f readMousePosition();
};

//--------------------------------------------------------------------------------------------
// KEYBOARD

/// The internal representation of a state of a keyboard.
struct keyboard_data_t : public Device
{
    bool chat_mode;                 ///< Input text from keyboard?
    bool chat_done;                 ///< Input text from keyboard finished?

    /// The keyboard state.
    int state_size;
    const Uint8 *state_ptr;
private:
    /// The modifier keys state.
    Ego::ModifierKeys modifierKeys;

protected:
    /// Construct this keyboard device.
	keyboard_data_t();
    // Befriend with input system.
    friend struct InputSystem;
    void open() override;
    void close() override;
    void update() override;

public:
    /// @brief Get if the specified key is down.
    /// @param key the key
    /// @return @a true if the specified key is down, @a false otherwise
	bool isKeyDown(int key) const;
    /// @brief Get if the specified key is up.
    /// @param key the key
    /// @return @a true if the specified key is up, @a false otherwise
    bool isKeyUp(int key) const;
    /// @brief Get the modifier keys state.
    /// @return the modifier key state
    Ego::ModifierKeys getModifierKeys() const;

public:
    bool getOpen() const override;
};

//--------------------------------------------------------------------------------------------
// JOYSTICK
#define JOYBUTTON           32                      ///< Maximum number of joystick buttons

/// The internal representation of a state of a joystick.
struct joystick_data_t : public Device
{
    float   x;
    float   y;
    Uint8   button[JOYBUTTON];
    Uint32  b;                 ///< Button masks

public:
    /// The SDL device index.
    int sdl_device_index;
    /// The SDL device pointer.
    SDL_Joystick *sdl_ptr;

public:
	joystick_data_t(int sdl_device_index);

public:
    ~joystick_data_t();

protected:
    friend struct InputSystem;
    void open() override;
    void close() override;
    void update() override;

public:
    bool getOpen() const override;
};

//--------------------------------------------------------------------------------------------

struct InputSystem : public Ego::Core::Singleton<InputSystem> {
    /**
     * @brief The hard-coded maximum number of joysticks.
     */
    static constexpr int MaximumNumberOfJoysticks = 16;
    static_assert(MaximumNumberOfJoysticks > 0, "maximum number of joysticks must be positive");
    
    /// @details initialize the input system.
    void update();
protected:
	/// @details Initialize the joysticks.
	void init_joysticks();

public:
    mouse_data_t mouse;
    keyboard_data_t keyboard;
    std::array<std::unique_ptr<joystick_data_t>, MaximumNumberOfJoysticks> joysticks;

public:
    InputSystem();
    ~InputSystem();
};

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

    // TODO: Use Vector2f.
    using Vector2i = Ego::Math::Discrete::Vector2<int>;

    /**
     * @brief Offset of the mouse.
     * @todo Type should be Vector2f.
     */
    Vector2i offset;

    uint8_t   button[4];       ///< Mouse button states
    uint32_t  b;               ///< Button masks

	mouse_data_t();

public:
    friend struct InputSystem;
    void open() override;
    void close() override;
    bool getOpen() const override;
    void update() override;

private:
    /// @todo Return type should be Vector2f.
    static Vector2f readMouseOffset();
    /// @todo Return type should be Vector2f.
    static Point2f readMousePosition();
};

//--------------------------------------------------------------------------------------------
// KEYBOARD

/// The internal representation of a state of a keyboard.
struct keyboard_data_t : public Device
{
    bool chat_mode;                 ///< Input text from keyboard?
    bool chat_done;                 ///< Input text from keyboard finished?

    int state_size;
    const Uint8 *state_ptr;

	keyboard_data_t();
	bool isKeyDown(int key) const;
    bool isKeyUp(int key) const;

protected:
    friend struct InputSystem;
    void open() override;
    void close() override;
    void update() override;

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

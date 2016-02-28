#pragma once

#include "IdLib/IdLib.hpp"
#include "egolib/platform.h"

namespace Ego
{
namespace Input
{

class InputDevice
{
public:
    static std::array<InputDevice, 4> DeviceList;

    InputDevice(const std::string &name);

    enum class InputButton
    {
        MOVE_UP,        //< Move towards the top of the screen
        MOVE_RIGHT,     //< Move to the right of the screen
        MOVE_DOWN,      //< Move towards the bottom of the screen
        MOVE_LEFT,      //< Move to the left of the screen
        JUMP,           //< Makes the character jump

        USE_LEFT,       //< Activate item in left hand (or unarmed attack)
        GRAB_LEFT,      //< Grab/Drop item in left hand
        PACK_LEFT,      //< Put left hand item in inventory

        USE_RIGHT,      //< Activate item in left hand (or unarmed attack)
        GRAB_RIGHT,     //< Grab/Drop item in left hand
        PACK_RIGHT,     //< Put left hand item in inventory

        STEALTH,        //< Activates stealth mode

        CAMERA_LEFT,    //< Rotate camera counter-clockwise
        CAMERA_RIGHT,   //< Rotate camera clockwise
        CAMERA_IN,      //< Zoom camera in
        CAMERA_OUT,     //< Zoom camera out

        COUNT           //< Always last
    };

    bool isButtonPressed(const InputButton button) const;

    void setInputMapping(const InputButton button, const SDL_Scancode key);

    std::string getMappedInputName(const InputButton button) const;

private:
    std::string _name;
    std::array<SDL_Scancode, static_cast<size_t>(InputButton::COUNT)> _keyMap;
};

} //Input
} //Ego

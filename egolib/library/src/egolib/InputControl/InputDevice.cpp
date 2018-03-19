#include "InputDevice.hpp"
#include "InputSystem.hpp"

namespace Ego
{
namespace Input
{

std::array<InputDevice, MAX_PLAYER> InputDevice::DeviceList = 
{
    InputDevice("Player 1"), 
    InputDevice("Player 2"), 
    InputDevice("Player 3"), 
    InputDevice("Player 4")
};

InputDevice::InputDevice(const std::string &name) :
    _name(name),
    _keyMap(),
    _type(InputDeviceType::UNKNOWN)
{
    _keyMap.fill(SDLK_UNKNOWN);

    //TODO: only support keyboard for now
    _type = InputDevice::InputDeviceType::KEYBOARD;
}

bool InputDevice::isButtonPressed(const InputButton button) const
{
    if(button == InputButton::COUNT) {
        return false;
    }

    const SDL_Scancode code = SDL_GetScancodeFromKey(_keyMap[static_cast<size_t>(button)]);

    return 1 == SDL_GetKeyboardState(nullptr)[code];
}

void InputDevice::setInputMapping(const InputButton button, const SDL_Keycode key)
{
    if(button != InputButton::COUNT) {
        _keyMap[static_cast<size_t>(button)] = key;
    }
}

std::string InputDevice::getMappedInputName(const InputButton button) const
{
    SDL_Keycode key = SDLK_UNKNOWN;

    if(button != InputButton::COUNT) {
        key = _keyMap[static_cast<size_t>(button)];
    }

    if(key == SDLK_UNKNOWN) {
        return "SDLK_UNKNOWN";
    }

    return SDL_GetKeyName(key);
}

InputDevice::InputDeviceType InputDevice::getDeviceType() const
{
    return _type;
}

Vector2f InputDevice::getInputMovement() const
{
    Vector2f result = idlib::zero<Vector2f>();

    switch(_type)
    {
        // Keyboard routines
        case Ego::Input::InputDevice::InputDeviceType::KEYBOARD:
        {
            if(isButtonPressed(Ego::Input::InputDevice::InputButton::MOVE_RIGHT)) {
                result.x()++;
            }   
            if(isButtonPressed(Ego::Input::InputDevice::InputButton::MOVE_LEFT)) {
                result.x()--;
            }
            if(isButtonPressed(Ego::Input::InputDevice::InputButton::MOVE_DOWN)) {
                result.y()++;
            }   
            if(isButtonPressed(Ego::Input::InputDevice::InputButton::MOVE_UP)) {
                result.y()--;
            }
        }
        break;

        // Mouse routines
        case Ego::Input::InputDevice::InputDeviceType::MOUSE:
        {
            const float dist = idlib::euclidean_norm(InputSystem::get().getMouseMovement());
            if (dist > 0)
            {
                float scale = InputSystem::MOUSE_SENSITIVITY / dist;
                if (dist < InputSystem::MOUSE_SENSITIVITY)
                {
                    scale = dist / InputSystem::MOUSE_SENSITIVITY;
                }

                if (InputSystem::MOUSE_SENSITIVITY > 0.0f)
                {
                    scale /= InputSystem::MOUSE_SENSITIVITY;
                }

                result = InputSystem::get().getMouseMovement() * scale;
            }
        }
        break;

        case Ego::Input::InputDevice::InputDeviceType::JOYSTICK:
        {
            //TODO: Not implemented
            /*
            //Figure out which joystick we are using
            joystick_data_t *joystick;
            joystick = InputSystem::get().joysticks[pdevice->device_type - MAX_JOYSTICK].get();

            if ( fast_camera_turn || !input_device_t::control_active( pdevice, CONTROL_CAMERA ) )
            {
                joy_pos[XX] = joystick->x;
                joy_pos[YY] = joystick->y;

                float dist = joy_pos.length_2();
                if ( dist > 1.0f )
                {
                    scale = 1.0f / std::sqrt( dist );
                    joy_pos *= scale;
                }

                if ( fast_camera_turn && !input_device_t::control_active( pdevice, CONTROL_CAMERA ) )  joy_pos[XX] = 0;

                movementInput.x() = ( joy_pos[XX] * fcos + joy_pos[YY] * fsin );
                movementInput.y() = ( -joy_pos[XX] * fsin + joy_pos[YY] * fcos );
            }
            */
        }
        break;

        case Ego::Input::InputDevice::InputDeviceType::UNKNOWN:
            //Should not happen... throw exception?
        break;
    }

    return result;
}

} //Input
} //Ego

#include "InputDevice.hpp"

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
}

bool InputDevice::isButtonPressed(const InputButton button) const
{
    if(button == InputButton::COUNT) {
        return false;
    }

    const SDL_Scancode code = SDL_GetScancodeFromKey(_keyMap[static_cast<size_t>(button)]);

    return SDL_GetKeyboardState(nullptr)[code];
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

    return SDL_GetKeyName(key);
}

InputDevice::InputDeviceType InputDevice::getDeviceType() const
{
    return _type;
}

} //Input
} //Ego

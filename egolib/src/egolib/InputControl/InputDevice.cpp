#include "InputDevice.hpp"

namespace Ego
{
namespace Input
{

std::array<InputDevice, 4> InputDevice::DeviceList = 
{
    InputDevice("Player 1"), 
    InputDevice("Player 2"), 
    InputDevice("Player 3"), 
    InputDevice("Player 4")
};

InputDevice::InputDevice(const std::string &name) :
    _name(name),
    _keyMap()
{
    _keyMap.fill(SDL_SCANCODE_UNKNOWN);
}

bool InputDevice::isButtonPressed(const InputButton button) const
{
    if(button == InputButton::COUNT) {
        return false;
    }

    const SDL_Scancode key = _keyMap[static_cast<size_t>(button)];

    return SDL_GetKeyboardState(nullptr)[key];
}

void InputDevice::setInputMapping(const InputButton button, const SDL_Scancode key)
{
    if(button != InputButton::COUNT) {
        _keyMap[static_cast<size_t>(button)] = key;
    }
}

std::string InputDevice::getMappedInputName(const InputButton button) const
{
    SDL_Scancode key = SDL_SCANCODE_UNKNOWN;

    if(button != InputButton::COUNT) {
        key = _keyMap[static_cast<size_t>(button)];
    }

    return SDL_GetKeyName(SDL_GetKeyFromScancode(key));
}

} //Input
} //Ego

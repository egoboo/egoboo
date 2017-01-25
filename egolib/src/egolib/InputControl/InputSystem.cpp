#include "InputSystem.hpp"

namespace Ego {
namespace Input {

InputSystem::InputSystem() :
    mouseMovement(0.0f, 0.0f),
    mouseButtonDown(),
    modifierKeys()
{
    mouseButtonDown.fill(false);
}

InputSystem::~InputSystem()
{}

void InputSystem::update()
{
    // Update mouse movement.
    int xBackend, yBackend;
    uint32_t mouseButtonDownBackend = SDL_GetRelativeMouseState(&xBackend, &yBackend);
    mouseMovement.x() = -xBackend;
    mouseMovement.y() = -yBackend;

    // Update mouse buttons.
    mouseButtonDown[MouseButton::LEFT] = mouseButtonDownBackend & SDL_BUTTON(SDL_BUTTON_LEFT);
    mouseButtonDown[MouseButton::MIDDLE] = mouseButtonDownBackend & SDL_BUTTON(SDL_BUTTON_MIDDLE);
    mouseButtonDown[MouseButton::RIGHT] = mouseButtonDownBackend & SDL_BUTTON(SDL_BUTTON_RIGHT);
    mouseButtonDown[MouseButton::X1] = mouseButtonDownBackend & SDL_BUTTON(SDL_BUTTON_X1);
    mouseButtonDown[MouseButton::X2] = mouseButtonDownBackend & SDL_BUTTON(SDL_BUTTON_X2);

    // (2) Get and translate the modifier keys state.
    auto backendModifierKeys = SDL_GetModState();
    modifierKeys = ModifierKeys();
    // NUMLOCK.
    if (SDL_Keymod::KMOD_NUM == (backendModifierKeys & SDL_Keymod::KMOD_NUM))
    {
        modifierKeys |= ModifierKeys::Num;
    }
    // LGUI and RGUI
    if (SDL_Keymod::KMOD_LGUI == (backendModifierKeys & SDL_Keymod::KMOD_LGUI))
    {
        modifierKeys |= ModifierKeys::LeftGui;
    }
    if (SDL_Keymod::KMOD_RGUI == (backendModifierKeys & SDL_Keymod::KMOD_RGUI))
    {
        modifierKeys |= ModifierKeys::RightGui;
    }
    // LSHIFT and RSHIFT
    if (SDL_Keymod::KMOD_LSHIFT == (backendModifierKeys & SDL_Keymod::KMOD_LSHIFT))
    {
        modifierKeys |= ModifierKeys::LeftShift;
    }
    if (SDL_Keymod::KMOD_RSHIFT == (backendModifierKeys & SDL_Keymod::KMOD_RSHIFT))
    {
        modifierKeys |= ModifierKeys::RightShift;
    }
    // CAPS
    if (SDL_Keymod::KMOD_CAPS == (backendModifierKeys & SDL_Keymod::KMOD_CAPS))
    {
        modifierKeys |= ModifierKeys::Caps;
    }
    // LCTRL and RCTRL
    if (SDL_Keymod::KMOD_LCTRL == (backendModifierKeys & SDL_Keymod::KMOD_LCTRL))
    {
        modifierKeys |= ModifierKeys::LeftControl;
    }
    if (SDL_Keymod::KMOD_RCTRL == (backendModifierKeys & SDL_Keymod::KMOD_RCTRL))
    {
        modifierKeys |= ModifierKeys::RightControl;
    }
    // LALT and RALT
    if (SDL_Keymod::KMOD_LALT == (backendModifierKeys & SDL_Keymod::KMOD_LALT))
    {
        modifierKeys |= ModifierKeys::LeftAlt;
    }
    if (SDL_Keymod::KMOD_RALT == (backendModifierKeys & SDL_Keymod::KMOD_RALT))
    {
        modifierKeys |= ModifierKeys::RightAlt;
    }
}

const Vector2f& InputSystem::getMouseMovement() const
{
    return mouseMovement;
}

bool InputSystem::isMouseButtonDown(const MouseButton button) const
{
    if (button >= mouseButtonDown.size())
    {
        return false;
    }
    return mouseButtonDown[button];
}

bool InputSystem::isKeyDown(const SDL_Keycode key) const
{
    return SDL_GetKeyboardState(nullptr)[SDL_GetScancodeFromKey(key)];
}

ModifierKeys InputSystem::getModifierKeys() const
{
    return modifierKeys;
}

} // namespace Input
} // namespace Ego


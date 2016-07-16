#include "InputSystem.hpp"

namespace Ego
{
namespace Input
{


InputSystem::InputSystem() :
	_mouseMovement(0.0f, 0.0f),
	_mouseButtonDown(),
	_modifierKeys()
{
	_mouseButtonDown.fill(false);
}

InputSystem::~InputSystem()
{

}

void InputSystem::update()
{
	//Update mouse movement
	int x, y;
    uint32_t b = SDL_GetRelativeMouseState(&x, &y);
    _mouseMovement.x() = -x;
    _mouseMovement.y() = -y;

    //Update mouse buttons
    _mouseButtonDown[MouseButton::LEFT] = b & SDL_BUTTON(SDL_BUTTON_LEFT);
    _mouseButtonDown[MouseButton::MIDDLE] = b & SDL_BUTTON(SDL_BUTTON_MIDDLE);
    _mouseButtonDown[MouseButton::RIGHT] = b & SDL_BUTTON(SDL_BUTTON_RIGHT); 
    _mouseButtonDown[MouseButton::X1] = b & SDL_BUTTON(SDL_BUTTON_X1);
    _mouseButtonDown[MouseButton::X2] = b & SDL_BUTTON(SDL_BUTTON_X2); 

    // (2) Get and translate the modifier keys state.
    auto backendModifierKeys = SDL_GetModState();
    _modifierKeys = Ego::ModifierKeys();
    // NUMLOCK.
    if (SDL_Keymod::KMOD_NUM == (backendModifierKeys & SDL_Keymod::KMOD_NUM)) {
        _modifierKeys |= Ego::ModifierKeys::Num;
    }
    // LGUI and RGUI
    if (SDL_Keymod::KMOD_LGUI == (backendModifierKeys & SDL_Keymod::KMOD_LGUI)) {
        _modifierKeys |= Ego::ModifierKeys::LeftGui;
    }
    if (SDL_Keymod::KMOD_RGUI == (backendModifierKeys & SDL_Keymod::KMOD_RGUI)) {
        _modifierKeys |= Ego::ModifierKeys::RightGui;
    }
    // LSHIFT and RSHIFT
    if (SDL_Keymod::KMOD_LSHIFT == (backendModifierKeys & SDL_Keymod::KMOD_LSHIFT)) {
        _modifierKeys |= Ego::ModifierKeys::LeftShift;
    }
    if (SDL_Keymod::KMOD_RSHIFT == (backendModifierKeys & SDL_Keymod::KMOD_RSHIFT)) {
        _modifierKeys |= Ego::ModifierKeys::RightShift;
    }
    // CAPS
    if (SDL_Keymod::KMOD_CAPS == (backendModifierKeys & SDL_Keymod::KMOD_CAPS)) {
        _modifierKeys |= Ego::ModifierKeys::Caps;
    }
    // LCTRL and RCTRL
    if (SDL_Keymod::KMOD_LCTRL == (backendModifierKeys & SDL_Keymod::KMOD_LCTRL)) {
        _modifierKeys |= Ego::ModifierKeys::LeftControl;
    }
    if (SDL_Keymod::KMOD_RCTRL == (backendModifierKeys & SDL_Keymod::KMOD_RCTRL)) {
        _modifierKeys |= Ego::ModifierKeys::RightControl;
    }
    // LALT and RALT
    if (SDL_Keymod::KMOD_LALT == (backendModifierKeys & SDL_Keymod::KMOD_LALT)) {
        _modifierKeys |= Ego::ModifierKeys::LeftAlt;
    }
    if (SDL_Keymod::KMOD_RALT == (backendModifierKeys & SDL_Keymod::KMOD_RALT)) {
        _modifierKeys |= Ego::ModifierKeys::RightAlt;
    }
}

const Vector2f& InputSystem::getMouseMovement() const
{
	return _mouseMovement;
}

bool InputSystem::isMouseButtonDown(const MouseButton button) const
{
	if(button >= _mouseButtonDown.size()) {
		return false;
	}
	return _mouseButtonDown[button];
}

bool InputSystem::isKeyDown(const SDL_Keycode key) const
{
	return SDL_GetKeyboardState(nullptr)[SDL_GetScancodeFromKey(key)];
}

} //Input
} //Ego

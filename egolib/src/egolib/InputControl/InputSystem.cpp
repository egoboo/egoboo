#include "InputSystem.hpp"

namespace Ego
{
namespace Input
{


InputSystem::InputSystem() :
	_mouseMovement(0.0f, 0.0f),
	_mouseButtonDown()
{
	_mouseButtonDown.fill(false);
}

void InputSystem::initialize()
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

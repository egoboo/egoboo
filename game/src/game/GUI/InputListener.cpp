#include "game/GUI/InputListener.hpp"

InputListener::~InputListener()
{
    //dtor
}

bool InputListener::notifyMouseMoved(const Ego::Events::MouseMovedEventArgs& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyKeyDown(const int keyCode)
{
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseClicked(const Ego::Events::MouseClickedEventArgs& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseScrolled(const int amount)
{
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseReleased(const Ego::Events::MouseReleasedEventArgs& e)
{
    // Default: Event is not handled.
    return false;
}

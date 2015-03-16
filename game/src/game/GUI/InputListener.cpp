#include "game/GUI/InputListener.hpp"

InputListener::~InputListener()
{
    //dtor
}

bool InputListener::notifyMouseMoved(const int x, const int y)
{
    //default no implementation
    return false;
}

bool InputListener::notifyKeyDown(const int keyCode)
{
    //default no implementation
    return false;
}

bool InputListener::notifyMouseClicked(const int button, const int x, const int y)
{
    //default no implementation
    return false;
}

bool InputListener::notifyMouseScrolled(const int amount)
{
    //default no implementation
    return false;
}

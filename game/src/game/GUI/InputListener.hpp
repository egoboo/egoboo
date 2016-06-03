#pragma once

#include "egolib/egolib.h"

/**
 * This interface class allows the object to listen to any SDL events occuring. If any of the functions
 * return true, then it means the event will be consumed (and no other listeners will receive it)
 */
class InputListener {
public:
    virtual ~InputListener();
    /**
     * @brief Invoked if a mouse moved event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseMoved(const Ego::Events::MouseMovedEventArgs& e);
    virtual bool notifyKeyDown(const int keyCode);
    /**
     * @brief Invoked if a mouse clicked event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseClicked(const Ego::Events::MouseClickedEventArgs& e);
    virtual bool notifyMouseScrolled(const int amount);
    virtual bool notifyMouseReleased(const Ego::Events::MouseReleasedEventArgs& e);
};

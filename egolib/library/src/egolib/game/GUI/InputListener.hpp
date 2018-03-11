#pragma once

#include "egolib/integrations/events.hpp"

namespace Ego {
namespace GUI {

/**
 * This interface class allows the object to listen to any SDL events occuring. If any of the functions
 * return true, then it means the event will be consumed (and no other listeners will receive it)
 */
class InputListener {
public:
    virtual ~InputListener();

public:
    /**
     * @brief Invoked if a mouse moved event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMousePointerMoved(const Events::MousePointerMovedEvent& e);

public:
    /**
     * @brief Invoked if a mouse wheel turned event occurred.
     * @param e the event arguments
     * @retrn @a true if th event was processed, @a false otherwise
     */
    virtual bool notifyMouseWheelTurned(const Events::MouseWheelTurnedEvent& e);

public:
    /**
     * @brief Invoked if a mouse button pressed event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseButtonPressed(const Events::MouseButtonPressedEvent& e);

    /**
     * @brief Invoked if a mouse button released event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseButtonReleased(const Events::MouseButtonReleasedEvent& e);

    /**
     * @brief Invoked if a mouse button clicked event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseButtonClicked(const Events::MouseButtonClickedEvent& e);

public:
    /**
     * @brief Invoked if a keyboard key pressed event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEvent& e);

    /**
     * @brief Invoked if a keyboard key released event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyKeyboardKeyReleased(const Events::KeyboardKeyReleasedEvent& e);

    /**
     * @brief Invoked if a keyboard key typed event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyKeyboardKeyTyped(const Events::KeyboardKeyTypedEvent& e);
  
};

} // namespace GUI
} // namespace Ego

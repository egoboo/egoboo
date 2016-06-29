#include "game/GUI/InputListener.hpp"

namespace Ego {
namespace GUI {

InputListener::~InputListener() {
    //dtor
}

bool InputListener::notifyMouseMoved(const Events::MouseMovedEventArgs& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEventArgs& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseButtonClicked(const Events::MouseButtonClickedEventArgs& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseScrolled(const int amount) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseButtonReleased(const Events::MouseButtonReleasedEventArgs& e) {
    // Default: Event is not handled.
    return false;
}

} // namespace GUI
} // namespace Ego

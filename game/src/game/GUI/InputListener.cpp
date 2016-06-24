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

bool InputListener::notifyKeyDown(const int keyCode) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseClicked(const Events::MouseClickedEventArgs& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseScrolled(const int amount) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseReleased(const Events::MouseReleasedEventArgs& e) {
    // Default: Event is not handled.
    return false;
}

} // namespace GUI
} // namespace Ego

#include "game/GUI/OptionsButton.hpp"

namespace Ego {
namespace GUI {

OptionsButton::OptionsButton(const std::string &label) :
    _label(label) {

}

void OptionsButton::setPosition(float x, float y) {
    _label.setPosition(x, y);
    Component::setPosition(x + 200, y - getHeight() / 2);
}

void OptionsButton::draw() {
    _label.draw();
    Button::draw();
}

} // namespace GUI
} // namespace Ego

#include "egolib/game/GUI/OptionsButton.hpp"

namespace Ego {
namespace GUI {

OptionsButton::OptionsButton(const std::string &label) :
    _label(label) {

}

void OptionsButton::setPosition(const Point2f& position) {
    _label.setPosition(position);
    Component::setPosition(position + Vector2f(200, - getHeight() / 2));
}

void OptionsButton::draw(DrawingContext& drawingContext) {
    _label.draw(drawingContext);
    Button::draw(drawingContext);
}

} // namespace GUI
} // namespace Ego

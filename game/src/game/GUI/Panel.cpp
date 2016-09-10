#include "game/GUI/Panel.hpp"
#include "game/GUI/JoinBounds.hpp"

namespace Ego {
namespace GUI {

void Panel::draw(DrawingContext& drawingContext) {
    drawContainer(drawingContext);
    drawAll(drawingContext);
}

void Panel::drawContainer(DrawingContext& drawingContext) {}

Panel::Panel() : Container() {
    setSize(Vector2f(320, 240));
}

Vector2f Panel::getDesiredSize() {
    JoinBounds joinBounds;
    return joinBounds(*this).getSize();
}

} // namespace GUI
} // namespace Ego

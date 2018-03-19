#include "egolib/game/GUI/Panel.hpp"
#include "egolib/game/GUI/JoinBounds.hpp"

namespace Ego::GUI {

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
    return joinBounds(*this).get_size();
}

} // namespace Ego::GUI

#include "game/GUI/TabPanel.hpp"
#include "game/GUI/JoinBounds.hpp"

namespace Ego {
namespace GUI {

void Tab::draw(DrawingContext& drawingContext) {
    drawContainer(drawingContext);
    drawAll(drawingContext);
}

void Tab::drawContainer(DrawingContext& drawingContext) {}

Tab::Tab(const std::string& title) : Container(),
    _title(title) {
    setSize(Vector2f(320, 240));
}

const std::string& Tab::getTitle() const {
    return _title;
}

void Tab::setTitle(const std::string& title) {
    _title = title;
}

Vector2f Tab::getDesiredSize() {
    JoinBounds joinBounds;
    return joinBounds(*this).get_size();
}

} // namespace GUI
} // namespace Ego
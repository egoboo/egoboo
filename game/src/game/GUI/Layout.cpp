#include "game/GUI/Layout.hpp"
#include "game/GUI/Component.hpp"

namespace Ego {
namespace GUI {

LayoutColumns::LayoutColumns() : LayoutColumns(Point2f(0.0f, 0.0f), 1.0f, 0.0f, 0.0f) {}

LayoutColumns::LayoutColumns(const Point2f& leftTop, float bottom, float horizontalGap, float verticalGap)
    : leftTop(leftTop), bottom(bottom), horizontalGap(horizontalGap), verticalGap(verticalGap) {}

void LayoutColumns::operator()(const std::vector<std::shared_ptr<Component>>& components) const {
    auto it = components.cbegin();
    // If there are no components, there is nothing to do.
    if (it == components.cend()) return;
    // Compute maximum size of all components.
    float maximumWidth = (*it)->getWidth(),
        maximumHeight = (*it)->getHeight();
    it++;
    while (it != components.cend()) {
        maximumWidth = std::max(maximumWidth, (*it)->getWidth());
        maximumHeight = std::max(maximumHeight, (*it)->getHeight());
        it++;
    }

    float horizontalIncrement = maximumWidth + horizontalGap;
    float verticalIncrement = maximumHeight + verticalGap;

    Point2f currentLeftTop = leftTop;
    for (auto& component : components) {
        // If the component would not fit into the given vertical space ...
        if (currentLeftTop.y() + maximumHeight > bottom) {
            // ... move it into a new column:
            // Increment current left by maximum width plus horizontal gap.
            // Set current top to top.
            currentLeftTop = Point2f(leftTop.x() + horizontalIncrement, leftTop.y());
        }
        component->setPosition(currentLeftTop);
        // Increment current top by maximum height plus vertical gap.
        currentLeftTop += Vector2f(0.0f, verticalIncrement);
    }
}

} // namespace GUI
} // namespace Ego

namespace Ego {
namespace GUI {

LayoutRows::LayoutRows() : LayoutRows(Point2f(0.0f, 0.0f), 1.0f, 0.0f, 0.0f)
{}

LayoutRows::LayoutRows(const Point2f& leftTop, float right, float horizontalGap, float verticalGap)
    : leftTop(leftTop), right(right), horizontalGap(horizontalGap), verticalGap(verticalGap)
{}

void LayoutRows::operator()(const std::vector<std::shared_ptr<Component>>& components) const {
    auto it = components.cbegin();
    // If there are no components, there is nothing to do.
    if (it == components.cend()) return;
    // Compute maximum size of all components.
    float maximumWidth = (*it)->getWidth(),
        maximumHeight = (*it)->getHeight();
    it++;
    while (it != components.cend()) {
        maximumWidth = std::max(maximumWidth, (*it)->getWidth());
        maximumHeight = std::max(maximumHeight, (*it)->getHeight());
        it++;
    }

    float horizontalIncrement = maximumWidth + horizontalGap;
    float verticalIncrement = maximumHeight + verticalGap;

    Point2f currentLeftTop = leftTop;
    for (auto& component : components) {
        // If the component would not fit into the given horizontal space ...
        if (currentLeftTop.x() + maximumWidth > right) {
            // ... move it into a new row:
            // Increment current top by maximum height plus vertical gap (vertical increment).
            // Set current left to left.
            currentLeftTop = Point2f(leftTop.x(), currentLeftTop.y() + verticalIncrement);
        }
        component->setPosition(currentLeftTop);
        // Increment current left by maximum width plus horizontal gap.
        currentLeftTop += Vector2f(horizontalIncrement, 0.0f);
    }
}

} // namespace GUI
} // namespace Ego

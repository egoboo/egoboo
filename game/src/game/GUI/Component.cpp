#include "game/GUI/Component.hpp"
#include "game/GUI/ComponentContainer.hpp"

namespace Ego {
namespace GUI {

Component::Component() :
    _destroyed(false),
    _enabled(true),
    _visible(true),
    _parent(nullptr),
    _bounds(Point2f(0.0f, 0.0f), Point2f(32.0f, 32.0f)) {}

bool Component::isEnabled() const {
    return _enabled && !_destroyed && _visible;
}

void Component::setEnabled(const bool enabled) {
    _enabled = enabled;
}

Point2f Component::getPosition() const {
    return _bounds.getMin();
}

Vector2f Component::getSize() const {
    return _bounds.getSize();
}

float Component::getX() const {
    return _bounds.getMin().x();
}

float Component::getY() const {
    return _bounds.getMin().y();
}

float Component::getWidth() const {
    return _bounds.getSize().x();
}

float Component::getHeight() const {
    return _bounds.getSize().y();
}

void Component::setWidth(float width) {
    _bounds = Rectangle2f(_bounds.getMin(), _bounds.getMin() + Vector2f(width, _bounds.getSize().y()));
}

void Component::setHeight(float height) {
    _bounds = Rectangle2f(_bounds.getMin(), _bounds.getMin() + Vector2f(_bounds.getSize().x(), height));
}

void Component::setSize(float width, float height) {
    setWidth(width);
    setHeight(height);
}

void Component::setX(float x) {
    Translate<Rectangle2f> t;
    // Translate such that the left side is at x
    _bounds = t(_bounds, Vector2f(-_bounds.getMin().x() + x, 0.0f));
}

void Component::setY(float y) {
    Translate<Rectangle2f> t;
    // Translate such that the top side is at y.
    _bounds = t(_bounds, Vector2f(0.0f, -_bounds.getMin().y() + y));
}

void Component::setPosition(float x, float y) {
    Translate<Rectangle2f> t;
    // Translate such that the top/left corner is at (x,y).
    _bounds = t(_bounds, Vector2f(-_bounds.getMin().x() + x, -_bounds.getMin().y() + y));
}

void Component::setCenterPosition(float x, float y, const bool onlyHorizontal) {
    setPosition(x - getWidth() / 2.0f, onlyHorizontal ? y : y - getHeight() / 2.0f);
}

const Rectangle2f& Component::getBounds() const {
    return _bounds;
}

bool Component::contains(const Point2f& point) const {
    Contains<Rectangle2f, Point2f> functor;
    return functor(_bounds, point);
}

ComponentContainer* Component::getParent() const {
    return _parent;
}

void Component::destroy() {
    _destroyed = true;
    if (_parent) {
        getParent()->notifyDestruction();
    }
}

bool Component::isDestroyed() const {
    return _destroyed;
}

void Component::setVisible(const bool visible) {
    _visible = visible;
}

bool Component::isVisible() const {
    return _visible && !_destroyed;
}

void Component::bringToFront() {
    if (!_parent) return;
    _parent->bringComponentToFront(shared_from_this());
}

} // namespace GUI
} // namespace Ego

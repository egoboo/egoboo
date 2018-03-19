#include "egolib/game/GUI/Component.hpp"
#include "egolib/game/GUI/Container.hpp"

namespace Ego::GUI {

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
    return _bounds.get_min();
}

Vector2f Component::getSize() const {
    return _bounds.get_size();
}

float Component::getX() const {
    return getPosition().x();
}

float Component::getY() const {
    return getPosition().y();
}

float Component::getWidth() const {
    return getSize().x();
}

float Component::getHeight() const {
    return getSize().y();
}

void Component::setWidth(float width) {
    setSize(Vector2f(width, getSize().y()));
}

void Component::setHeight(float height) {
    setSize(Vector2f(getSize().x(), height));
}

void Component::setSize(const Vector2f& size) {
    _bounds = Rectangle2f(_bounds.get_min(), _bounds.get_min() + size);
}

void Component::setX(float x) {
    setPosition(Point2f(x, getPosition().y()));
}

void Component::setY(float y) {
    setPosition(Point2f(getPosition().x(), y));
}

void Component::setPosition(const Point2f& position) {
    // Translate such that the top/left corner is at (x,y).
    _bounds = idlib::translate(_bounds, Vector2f(-_bounds.get_min().x() + position.x(), -_bounds.get_min().y() + position.y()));
}

void Component::setCenterPosition(const Point2f& position, const bool onlyHorizontal) {
    auto t = Vector2f(-getWidth() / 2.0f, onlyHorizontal ? 0.0f : -getHeight() / 2.0f);
    setPosition(position + t);
}

const Rectangle2f& Component::getBounds() const {
    return _bounds;
}

bool Component::contains(const Point2f& point) const {
    return idlib::is_enclosing(_bounds, point);
}

void Component::setParent(Container *parent) {
    this->_parent = parent;
}

Container *Component::getParent() const {
    return _parent;
}

Rectangle2f Component::getDerivedBounds() const {
    auto bounds = getBounds();
    if (nullptr != this->_parent) {
        return idlib::translate(bounds, idlib::semantic_cast<Vector2f>(this->_parent->getDerivedPosition()));
    } else {
        return bounds;
    }
}

Point2f Component::getDerivedPosition() const {
    if (nullptr != this->_parent) {
        return getPosition() + idlib::semantic_cast<Vector2f>(this->_parent->getDerivedPosition());
    } else {
        return getPosition();
    }
}

void Component::destroy() {
    _destroyed = true;
    if (_parent) {
        getParent()->removeComponent(shared_from_this());
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

} // namespace Ego::GUI

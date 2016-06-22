#include "game/GUI/GUIComponent.hpp"
#include "game/GUI/ComponentContainer.hpp"

GUIComponent::GUIComponent() :
    _destroyed(false),
    _enabled(true),
    _visible(true),
    _parent(nullptr),
    _bounds(Point2f(0.0f, 0.0f), Point2f(32.0f, 32.0f))
{
}

bool GUIComponent::isEnabled() const
{
    return _enabled && !_destroyed && _visible;
}

void GUIComponent::setEnabled(const bool enabled)
{
    _enabled = enabled;
}

Point2f GUIComponent::getPosition() const
{
    return _bounds.getMin();
}

Vector2f GUIComponent::getSize() const
{
    return _bounds.getSize();
}

float GUIComponent::getX() const
{
    return _bounds.getMin().x();
}

float GUIComponent::getY() const
{
    return _bounds.getMin().y();
}

float GUIComponent::getWidth() const
{
    return _bounds.getSize().x();
}

float GUIComponent::getHeight() const
{
    return _bounds.getSize().y();
}

void GUIComponent::setWidth(float width)
{
    _bounds = Rectangle2f(_bounds.getMin(), _bounds.getMin() + Vector2f(width, _bounds.getSize().y()));
}

void GUIComponent::setHeight(float height)
{
    _bounds = Rectangle2f(_bounds.getMin(), _bounds.getMin() + Vector2f(_bounds.getSize().x(), height));
}

void GUIComponent::setSize(float width, float height)
{
    setWidth(width);
    setHeight(height);
}

void GUIComponent::setX(float x)
{
    Ego::Translate<Rectangle2f> t;
    // Translate such that the left side is at x
    _bounds = t(_bounds, Vector2f(-_bounds.getMin().x() + x, 0.0f));
}

void GUIComponent::setY(float y)
{
    Ego::Translate<Rectangle2f> t;
    // Translate such that the top side is at y.
    _bounds = t(_bounds, Vector2f(0.0f, -_bounds.getMin().y() + y));
}

void GUIComponent::setPosition(float x, float y)
{
    Ego::Translate<Rectangle2f> t;
    // Translate such that the top/left corner is at (x,y).
    _bounds = t(_bounds, Vector2f(-_bounds.getMin().x() + x, -_bounds.getMin().y() + y));
}

void GUIComponent::setCenterPosition(float x, float y, const bool onlyHorizontal)
{
    setPosition(x - getWidth() / 2.0f, onlyHorizontal ? y : y - getHeight() / 2.0f);
}

const Rectangle2f& GUIComponent::getBounds() const
{
    return _bounds;
}

bool GUIComponent::contains(const Point2f& point) const
{
    Ego::Contains<Rectangle2f, Point2f> functor;
    return functor(_bounds, point);
}

ComponentContainer* GUIComponent::getParent() const
{
    return _parent;
}

void GUIComponent::destroy()
{
    _destroyed = true;
    if(_parent) {
        getParent()->notifyDestruction();        
    }
}

bool GUIComponent::isDestroyed() const
{
    return _destroyed;
}

void GUIComponent::setVisible(const bool visible)
{
    _visible = visible;
}

bool GUIComponent::isVisible() const
{
    return _visible && !_destroyed;
}

void GUIComponent::bringToFront()
{
    if(!_parent) return;
    _parent->bringComponentToFront(shared_from_this());
}

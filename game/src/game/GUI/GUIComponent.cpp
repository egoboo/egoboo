#include "game/GUI/GUIComponent.hpp"
#include "game/GUI/ComponentContainer.hpp"

GUIComponent::GUIComponent() :
    _destroyed(false),
    _enabled(true),
    _visible(true),
    _parent(nullptr)
{
    _bounds.x = 0; _bounds.y = 0; _bounds.w = 32; _bounds.h = 32;
}

bool GUIComponent::isEnabled() const
{
    return _enabled && !_destroyed && _visible;
}

void GUIComponent::setEnabled(const bool enabled)
{
    _enabled = enabled;
}

int GUIComponent::getX() const
{
    return _bounds.x;
}

int GUIComponent::getY() const
{
    return _bounds.y;
}

int GUIComponent::getWidth() const
{
    return _bounds.w;
}

int GUIComponent::getHeight() const
{
    return _bounds.h;
}

void GUIComponent::setWidth(const int width)
{
    _bounds.w = width;
}

void GUIComponent::setHeight(const int height)
{
    _bounds.h = height;
}

void GUIComponent::setSize(const int width, const int height)
{
    setWidth(width);
    setHeight(height);
}

void GUIComponent::setX(const int x)
{
    _bounds.x = x;
}

void GUIComponent::setY(const int y)
{
    _bounds.y = y;
}

void GUIComponent::setPosition(const int x, const int y)
{
    setX(x);
    setY(y);
}

void GUIComponent::setCenterPosition(const int x, const int y, const bool onlyHorizontal)
{
    setPosition(x - getWidth() / 2, onlyHorizontal ? y : y - getHeight()/2);
}

const SDL_Rect& GUIComponent::getBounds() const
{
    return _bounds;
}

bool GUIComponent::contains(const Point2f& point) const
{
    return point.x() >= _bounds.x && point.y() >= _bounds.y
        && point.x() < _bounds.x + _bounds.w && point.y() < _bounds.y+_bounds.h;
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

#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/UIManager.hpp"
#include "game/GUI/InputListener.hpp"
#include "game/graphic.h"

namespace Ego {
namespace GUI {

// Forward declarations.
class Container;

class Component : public InputListener, public std::enable_shared_from_this<Component>, public Id::NonCopyable {
public:
    Component();

    virtual void draw() = 0;

    /// @brief Get if this component is enabled.
    /// @return @a true if this component is enabled, @a false otherwise
    virtual bool isEnabled() const;
    /// @brief Enable/disable this component.
    /// @param @a true enables this component, @a false disables it
    virtual void setEnabled(const bool enabled);

    /// @brief Get if this component is visible.
    /// @return @a true if this component is visible, @a false otherwise
    virtual bool isVisible() const;
    /// @brief Show/hide this component.
    /// @param visible @a true shows this component, @a false hides it
    void setVisible(const bool visible);


    /// @brief Get the bounds of this component.
    /// @return the bounds of this component
    const Rectangle2f& getBounds() const;
    /// @brief Set the size of this component.
    /// @param size the size of this component
    virtual void setSize(const Vector2f& size);
    /// @brief Set the position of this component
    /// @param position the position of this component
    virtual void setPosition(const Point2f& position);

    Point2f getPosition() const;
    Vector2f getSize() const;
    float getX() const;
    float getY() const;
    float getWidth() const;
    float getHeight() const;

    void setCenterPosition(const Point2f& position, const bool onlyHorizontal = false);
    virtual void setWidth(float width);
    virtual void setHeight(float height);

    virtual void setX(float x);
    virtual void setY(float y);


    /**
     * @brief Get if this component contains a point.
     * @param point the point
     * @return @a true if this component contains the specified point, @a false otherwise
     */
    bool contains(const Point2f& point) const;

    void setParent(Container *parent);
    Container *getParent() const;

    //Type cast operator
    operator std::shared_ptr<Component>() { return shared_from_this(); }

    void destroy();
    bool isDestroyed() const;

    /**
    * @return
    *   Number of GUI components currently contained within this container
    **/
    size_t getComponentCount() const;

    /**
    * @brief
    *   Requests the ComponentContainer parent of this GUI component to send this component
    *   to the back of the rendering queue, making it drawn last on the top of all other
    *   components inside this container. It will also be notified of any input events first.
    **/
    void bringToFront();

private:
    bool _destroyed;
    Rectangle2f _bounds;
    bool _enabled;
    bool _visible;
    Container *_parent;
};

} // namespace GUI
} // namespace Ego

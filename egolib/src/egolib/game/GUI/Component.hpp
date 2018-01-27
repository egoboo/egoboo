#pragma once

#include "egolib/game/GUI/DrawingContext.hpp"
#include "egolib/game/Core/GameEngine.hpp"
#include "egolib/game/GUI/UIManager.hpp"
#include "egolib/game/GUI/InputListener.hpp"
#include "egolib/game/graphic.h"

namespace Ego {
namespace GUI {

// Forward declarations.
class Container;

class Component : public InputListener, public std::enable_shared_from_this<Component>, private id::non_copyable {
public:
    Component();

    /// @param useDerived if @a true derived coordinates are used to draw the component, otherwise absolute coordinates
    virtual void draw(DrawingContext& drawingContext) = 0;

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

    /// @brief Get the derived bounds of this component.
    /// @return the derived bounds of this component
    Rectangle2f getDerivedBounds() const;
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

    /// @brief Set the parent container of this component.
    /// @param a pointer to the parent container or a null pointer
    void setParent(Container *parent);
    
    /// @brief Get the parent of this component.
    /// @return a pointer to the parent container of this component or a null pointer
    Container *getParent() const;

    /// @brief Get the derived position of this node.
    /// @remark The derived position takes into account the position of this node as well as the position of its parent container (if any).
    Point2f getDerivedPosition() const;

    /// @brief Type cast operator.
    operator std::shared_ptr<Component>() { return shared_from_this(); }

    void destroy();
    bool isDestroyed() const;

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

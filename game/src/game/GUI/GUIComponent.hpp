#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/UIManager.hpp"
#include "game/GUI/InputListener.hpp"
#include "game/graphic.h"

//Forward declarations
class ComponentContainer;

class GUIComponent : public InputListener, public std::enable_shared_from_this<GUIComponent>, public Id::NonCopyable
{
    public:
        GUIComponent();

        virtual void draw() = 0;

        virtual bool isEnabled() const;
        virtual void setEnabled(const bool enabled);

        void setVisible(const bool visible);

        virtual bool isVisible() const;

        const Rectangle2f& getBounds() const;
        Point2f getPosition() const;
        Vector2f getSize() const;
        float getX() const;
        float getY() const;
        float getWidth() const;
        float getHeight() const;

        void setCenterPosition(float x, float y, const bool onlyHorizontal = false);
        virtual void setWidth(float width);
        virtual void setHeight(float height);
        virtual void setSize(float width, float height);
        virtual void setX(float x);
        virtual void setY(float y);
        virtual void setPosition(float x, float y);

        /**
         * @brief Get if this component contains a point.
         * @param point the point
         * @return @a true if this component contains the specified point, @a false otherwise 
         */
        bool contains(const Point2f& point) const;

        ComponentContainer* getParent() const;

        //Type cast operator
        operator std::shared_ptr<GUIComponent>() {return shared_from_this();}

        void destroy();
        bool isDestroyed() const;

        /**
        * @brief
        *   Requests the ComponentContainer parent of this GUIComponent to send this component
        *   to the back of the rendering queue, making it drawn last on the top of all other
        *   components inside this container. It will also be notified of any input events first.
        **/
        void bringToFront();

    private:
        bool _destroyed;
        Rectangle2f _bounds;
        bool _enabled;
        bool _visible;
        ComponentContainer* _parent;

        friend class ComponentContainer;
};

#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/UIManager.hpp"
#include "game/GUI/InputListener.hpp"
#include "game/graphic.h"

//Forward declarations
class ComponentContainer;

class GUIComponent : public InputListener, public std::enable_shared_from_this<GUIComponent>, public Ego::Core::NonCopyable
{
    public:
        GUIComponent();

        virtual void draw() = 0;

        virtual bool isEnabled() const;
        virtual void setEnabled(const bool enabled);

        void setVisible(const bool visible);

        bool isVisible() const;

        const SDL_Rect& getBounds() const;
        int getX() const;
        int getY() const;
        int getWidth() const;
        int getHeight() const;

        void setCenterPosition(const int x, const int y, const bool onlyHorizontal = false);
        virtual void setWidth(const int width);
        virtual void setHeight(const int height);
        void setSize(const int width, const int height);
        virtual void setX(const int x);
        virtual void setY(const int y);
        virtual void setPosition(const int x, const int y);

        bool contains(const int x, const int y) const;

        ComponentContainer* getParent() const;

        //Type cast operator
        operator std::shared_ptr<GUIComponent>() {return shared_from_this();}

        void destroy();
        bool isDestroyed() const;

    private:
        bool _destroyed;
        SDL_Rect _bounds;
        bool _enabled;
        bool _visible;
        ComponentContainer* _parent;

        friend class ComponentContainer;
};

#pragma once

#include "game/GUI/InputListener.hpp"

//Forward declarations
class GUIComponent;

class ComponentContainer : public InputListener
{
public:
    ComponentContainer();

    virtual void addComponent(std::shared_ptr<GUIComponent> component);
    virtual void removeComponent(std::shared_ptr<GUIComponent> component);
    virtual void clearComponents();
    size_t getComponentCount() const;

    void drawAll();

    virtual bool notifyMouseMoved(const int x, const int y) override;
    virtual bool notifyKeyDown(const int keyCode) override;
    virtual bool notifyMouseClicked(const int button, const int x, const int y) override;
    virtual bool notifyMouseScrolled(const int amount) override;

    void notifyDestruction();

protected:
    virtual void drawContainer() = 0;

private:
    void cleanDestroyedComponents();

protected:
    std::list<std::shared_ptr<GUIComponent>> _componentList;
    std::mutex _componentListMutex;
    bool _componentDestroyed;
};

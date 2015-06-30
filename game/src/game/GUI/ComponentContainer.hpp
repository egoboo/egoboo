#pragma once

#include "game/GUI/InputListener.hpp"

//Forward declarations
class GUIComponent;

class ComponentContainer : public InputListener, Id::NonCopyable
{
public:
    ComponentContainer();

    virtual void addComponent(std::shared_ptr<GUIComponent> component);
    virtual void removeComponent(std::shared_ptr<GUIComponent> component);

    /**
    * @brief
    *   Clears and removes all GUIComponents from this container. Components are not
    *   destroyed (i.e they can still exist in another container). This method is thread-safe.
    **/
    virtual void clearComponents();

    /**
    * @return
    *   Number of GUIComponents currently contained within this container
    **/
    size_t getComponentCount() const;

    /**
    * @brief
    *   Renders all GUIComponents contained inside this container
    **/
    void drawAll();

    virtual bool notifyMouseMoved(const int x, const int y) override;
    virtual bool notifyKeyDown(const int keyCode) override;
    virtual bool notifyMouseClicked(const int button, const int x, const int y) override;
    virtual bool notifyMouseScrolled(const int amount) override;

    /**
    * @brief
    *   Notifies this container that one or more of its components have been destroyed.
    *   On the next event it will remove any destroyed components from the container.
    **/
    void notifyDestruction();

    /**
    * For each iterators
    **/
    inline std::vector<std::shared_ptr<GUIComponent>>::iterator begin() 
    {
        return _componentList.begin();
    }

    inline std::vector<std::shared_ptr<GUIComponent>>::iterator end() 
    {
        return _componentList.end();
    }

protected:
    virtual void drawContainer() = 0;

private:
    /**
    * @brief
    *   Called at each event to remove any components marked for destruction
    **/
    void cleanDestroyedComponents();

protected:
    std::vector<std::shared_ptr<GUIComponent>> _componentList;
    std::recursive_mutex _componentListMutex;
    bool _componentDestroyed;
};

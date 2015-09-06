#pragma once

#include "game/GUI/InputListener.hpp"

//Forward declarations
class GUIComponent;

class ComponentContainer : public InputListener, public Id::NonCopyable
{
public:
    class ComponentIterator : public Id::NonCopyable
    {
        public:

            inline std::vector<std::shared_ptr<GUIComponent>>::const_iterator cbegin() const 
            {
                return _container._componentList.cbegin();
            }

            inline std::vector<std::shared_ptr<GUIComponent>>::const_iterator cend() const 
            {
                return _container._componentList.cend();
            }

            inline std::vector<std::shared_ptr<GUIComponent>>::iterator begin()
            {
                return _container._componentList.begin();
            }

            inline std::vector<std::shared_ptr<GUIComponent>>::iterator end()
            {
                return _container._componentList.end();
            }   

            inline std::vector<std::shared_ptr<GUIComponent>>::const_reverse_iterator crbegin() const 
            {
                return _container._componentList.crbegin();
            }

            inline std::vector<std::shared_ptr<GUIComponent>>::const_reverse_iterator crend() const 
            {
                return _container._componentList.crend();
            }

            inline std::vector<std::shared_ptr<GUIComponent>>::reverse_iterator rbegin()
            {
                return _container._componentList.rbegin();
            }

            inline std::vector<std::shared_ptr<GUIComponent>>::reverse_iterator rend()
            {
                return _container._componentList.rend();
            }   

            ~ComponentIterator()
            {
                //Free the ComponentContainer lock
                _container.unlock();
            }

            // Copy constructor
            ComponentIterator(const ComponentIterator &other) : NonCopyable(),
                _container(other._container)
            {
                _container.lock();
            }

                
        private:
            ComponentIterator(ComponentContainer &container) :
                _container(container)
            {
                // Ensure the ComponentContainer is locked as long as we are in existance.
                _container.lock();
            }

            ComponentContainer &_container;

            friend class ComponentContainer;
    };

public:
    ComponentContainer();
    ~ComponentContainer();

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
    virtual bool notifyMouseReleased(const int button, const int x, const int y) override;
    virtual bool notifyMouseScrolled(const int amount) override;

    /**
    * @brief
    *   Notifies this container that one or more of its components have been destroyed.
    *   On the next event it will remove any destroyed components from the container.
    **/
    void notifyDestruction();

    /**
    * @brief
    *   Bring a GUI component to the front of this container, so that it is drawn on top of all
    *   others and consumes input events first
    **/ 
    void bringComponentToFront(std::shared_ptr<GUIComponent> component);

    /**
    * @brief
    *   Returns a thread-safe iterator to this container
    **/
    inline ComponentIterator iterator() { return ComponentIterator(*this); }

protected:
    virtual void drawContainer() = 0;

    void setComponentList(const std::vector<std::shared_ptr<GUIComponent>> &list);

private:
    /**
    * @brief
    *   Locks this container to ensure no destroyed components will be removed until unlock() has been called.
    *   A container can be locked multiple times and only once all locks have been released will it clean destroyed
    *   components.
    **/
    void lock();

    /**
    * @brief
    *   Releases one lock from the container. If all locks are released, then any GUIComponents marked for destruction
    *   will be removed from the container.
    **/
    void unlock();

private:
    std::vector<std::shared_ptr<GUIComponent>> _componentList;
    bool _componentDestroyed;
    size_t _semaphoreLock;          //Ref counter does not need to be atomic because of mutex locks
    std::mutex _containerMutex;
};

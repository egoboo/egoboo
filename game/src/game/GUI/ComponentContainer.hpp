#pragma once

#include "game/GUI/InputListener.hpp"

namespace Ego {
namespace GUI {

// Forward declarations.
class Component;

} // namespace GUI
} // namespace Ego


namespace Ego {
namespace GUI {

class ComponentContainer : public InputListener, public Id::NonCopyable {
public:
    class ComponentIterator : public Id::NonCopyable {
    public:

        inline std::vector<std::shared_ptr<Component>>::const_iterator cbegin() const {
            return _container._componentList.cbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::const_iterator cend() const {
            return _container._componentList.cend();
        }

        inline std::vector<std::shared_ptr<Component>>::iterator begin() {
            return _container._componentList.begin();
        }

        inline std::vector<std::shared_ptr<Component>>::iterator end() {
            return _container._componentList.end();
        }

        inline std::vector<std::shared_ptr<Component>>::const_reverse_iterator crbegin() const {
            return _container._componentList.crbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::const_reverse_iterator crend() const {
            return _container._componentList.crend();
        }

        inline std::vector<std::shared_ptr<Component>>::reverse_iterator rbegin() {
            return _container._componentList.rbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::reverse_iterator rend() {
            return _container._componentList.rend();
        }

        ~ComponentIterator() {
            //Free the ComponentContainer lock
            _container.unlock();
        }

        // Copy constructor
        ComponentIterator(const ComponentIterator &other) : NonCopyable(),
            _container(other._container) {
            _container.lock();
        }


    private:
        ComponentIterator(ComponentContainer &container) :
            _container(container) {
            // Ensure the ComponentContainer is locked as long as we are in existance.
            _container.lock();
        }

        ComponentContainer &_container;

        friend class ComponentContainer;
    };

public:
    ComponentContainer();
    ~ComponentContainer();

    virtual void addComponent(std::shared_ptr<Component> component);
    virtual void removeComponent(std::shared_ptr<Component> component);

    /**
     * @brief
     *  Clears and removes all GUI components from this container. GUI Components are not
     *  destroyed (i.e they can still exist in another container). This method is thread-safe.
     */
    virtual void clearComponents();

    /**
    * @return
    *   Number of GUI components currently contained within this container
    **/
    size_t getComponentCount() const;

    /**
    * @brief
    *   Renders all GUI components contained inside this container
    **/
    void drawAll();

    virtual bool notifyMouseMoved(const Events::MouseMovedEventArgs& e) override;
    virtual bool notifyKeyDown(const int keyCode) override;
    virtual bool notifyMouseClicked(const Events::MouseClickedEventArgs& e) override;
    virtual bool notifyMouseReleased(const Events::MouseReleasedEventArgs& e) override;
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
    void bringComponentToFront(std::shared_ptr<Component> component);

    /**
    * @brief
    *   Returns a thread-safe iterator to this container
    **/
    inline ComponentIterator iterator() { return ComponentIterator(*this); }

protected:
    virtual void drawContainer() = 0;

    void setComponentList(const std::vector<std::shared_ptr<Component>> &list);

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
    *   Releases one lock from the container. If all locks are released, then any GUI components marked for destruction
    *   will be removed from the container.
    **/
    void unlock();

private:
    std::vector<std::shared_ptr<Component>> _componentList;
    bool _componentDestroyed;
    size_t _semaphoreLock;          //Ref counter does not need to be atomic because of mutex locks
    std::mutex _containerMutex;
};

} // namespace GUI
} // namespace Ego

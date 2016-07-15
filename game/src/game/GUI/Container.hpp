#pragma once

#include "game/GUI/Component.hpp"

namespace Ego {
namespace GUI {
/// A container is a component which may contain other components.
class Container : public Component {
public:
    class ContainerIterator : public Id::NonCopyable {
    public:

        inline std::vector<std::shared_ptr<Component>>::const_iterator cbegin() const {
            return container.components.cbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::const_iterator cend() const {
            return container.components.cend();
        }

        inline std::vector<std::shared_ptr<Component>>::iterator begin() {
            return container.components.begin();
        }

        inline std::vector<std::shared_ptr<Component>>::iterator end() {
            return container.components.end();
        }

        inline std::vector<std::shared_ptr<Component>>::const_reverse_iterator crbegin() const {
            return container.components.crbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::const_reverse_iterator crend() const {
            return container.components.crend();
        }

        inline std::vector<std::shared_ptr<Component>>::reverse_iterator rbegin() {
            return container.components.rbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::reverse_iterator rend() {
            return container.components.rend();
        }

        ~ContainerIterator() {
            // Free the ComponentContainer lock
            container.unlock();
        }

        // Copy constructor
        ContainerIterator(const ContainerIterator &other) : NonCopyable(),
            container(other.container) {
            container.lock();
        }


    private:
        ContainerIterator(Container &container) :
            container(container) {
            // Ensure the ComponentContainer is locked as long as we are in existance.
            container.lock();
        }

        Container& container;

        friend class Container;
    };
    /// Construct this container.
    Container();
    /// Destruct this container.
    ~Container();
    /// Add a component to this container.
    /// @param component the component
    /// @throw Id::InvalidArgumentException @a component is a null pointer
    virtual void addComponent(const std::shared_ptr<Component>& component);
    /// Remove a component from this container.
    /// @param component the component
    /// @throw Id::InvalidArgumentException @a component is a null pointer
    virtual void removeComponent(const std::shared_ptr<Component>& component);
public:
    /// @brief
    /// Locks this container to ensure no destroyed components will be removed until unlock() has been called.
    /// A container can be locked multiple times and only once all locks have been released will it clean destroyed components.
    void lock();
    /// @brief
    /// Releases one lock from the container.
    /// If all locks are released, then any GUI components marked for destruction will be removed from the container.
    void unlock();

public:
    /// @copydoc InputListener::notifyMouseMoved
    virtual bool notifyMouseMoved(const Events::MouseMovedEventArgs& e) override;
    /// @copydoc InputListener::notifyKeyboardKeyPressed
    virtual bool notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEventArgs& ee) override;
    /// @copydoc InputListener::notifyMouseButtonReleased
    virtual bool notifyMouseButtonReleased(const Events::MouseButtonReleasedEventArgs& e) override;
    /// @copydoc InputListener::notifyMouseButtonPressed
    virtual bool notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e) override;
    /// @copydoc InputListener::notifyMouseWheelTurned
    virtual bool notifyMouseWheelTurned(const Events::MouseWheelTurnedEventArgs& e) override;

    /// @brief Notifies this container that one or more of its components have been destroyed.
    /// The next time the container is unlocked, it will remove all destroyed components.
    void notifyDestruction();

    /// @brief Bring a GUI component to the front of this container, so that it is drawn on top of all others and consumes input events first.
    /// @param component the component
    void bringComponentToFront(std::shared_ptr<Component> component);

    /**
    * @brief
    *  Clears and removes all GUI components from this container. GUI Components are not
    *  destroyed (i.e they can still exist in another container). This method is thread-safe.
    */
    virtual void clearComponents();
    void setComponentList(const std::vector<std::shared_ptr<Component>> &list);


    /// @brief Return a thread-safe iterator to this container.
    /// @return the iterator
    inline ContainerIterator iterator() { return ContainerIterator(*this); }

    /**
    * @return
    *   Number of GUI components currently contained within this container
    **/
    size_t getComponentCount() const;

public:
    /// @brief Renders all GUI components contained inside this container
    void drawAll();

protected:


    virtual void drawContainer() = 0;

private:
    /// @brief The component list.
    std::vector<std::shared_ptr<Component>> components;
    /// @brief The mutex.
    std::mutex mutex;
    /// @brief If the container has destroyed components.
    bool componentDestroyed;
    /// @brief The semaphore.
    /// Semaphore does not need to be atomic because of mutex locks.
    size_t semaphore;
};
} // namespace GUI
} // namespace Ego
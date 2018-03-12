#pragma once

#include "egolib/game/GUI/Component.hpp"

namespace Ego::GUI {

/// A container is a component which may contain other components.
class Container : public Component {
public:
    class ContainerIterator : private idlib::non_copyable {
    public:

        inline std::vector<std::shared_ptr<Component>>::const_iterator cbegin() const {
            return _components.cbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::const_iterator cend() const {
            return _components.cend();
        }

        inline std::vector<std::shared_ptr<Component>>::iterator begin() {
            return _components.begin();
        }

        inline std::vector<std::shared_ptr<Component>>::iterator end() {
            return _components.end();
        }

        inline std::vector<std::shared_ptr<Component>>::const_reverse_iterator crbegin() const {
            return _components.crbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::const_reverse_iterator crend() const {
            return _components.crend();
        }

        inline std::vector<std::shared_ptr<Component>>::reverse_iterator rbegin() {
            return _components.rbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::reverse_iterator rend() {
            return _components.rend();
        }

        ~ContainerIterator() {
        }

        // Copy constructor
        ContainerIterator(const ContainerIterator &other) : non_copyable(),
            _components(other._components) {
        }


    private:
        ContainerIterator(const std::vector<std::shared_ptr<Component>> &components) :
            _components(components) 
        {
            //ctor
        }

    private:
        //Copy of the vector member in the Container (for thread safety)
        std::vector<std::shared_ptr<Component>> _components;

        friend class Container;
    };
    /// Construct this container.
    Container();
    /// Destruct this container.
    ~Container();
    /// Add a component to this container.
    /// @param component the component
    /// @throw idlib::invalid_argument_error @a component is a null pointer
    virtual void addComponent(const std::shared_ptr<Component>& component);
    /// Remove a component from this container.
    /// @param component the component
    /// @throw idlib::invalid_argument_error @a component is a null pointer
    virtual void removeComponent(const std::shared_ptr<Component>& component);

public:
    /// @copydoc InputListener::notifyMousePointerMoved
    virtual bool notifyMousePointerMoved(const Events::MousePointerMovedEvent& e) override;
    /// @copydoc InputListener::notifyKeyboardKeyPressed
    virtual bool notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEvent& ee) override;
    /// @copydoc InputListener::notifyMouseButtonReleased
    virtual bool notifyMouseButtonReleased(const Events::MouseButtonReleasedEvent& e) override;
    /// @copydoc InputListener::notifyMouseButtonPressed
    virtual bool notifyMouseButtonPressed(const Events::MouseButtonPressedEvent& e) override;
    /// @copydoc InputListener::notifyMouseWheelTurned
    virtual bool notifyMouseWheelTurned(const Events::MouseWheelTurnedEvent& e) override;

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
    inline ContainerIterator iterator() 
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return ContainerIterator(_components); 
    }

    /**
    * @return
    *   Number of GUI components currently contained within this container
    **/
    size_t getComponentCount() const;

public:
    /// @brief Renders all GUI components contained inside this container
    virtual void drawAll(DrawingContext& drawingContext);

protected:
    virtual void drawContainer(DrawingContext& drawingContext) = 0;

private:
    /// @brief The component list.
    std::vector<std::shared_ptr<Component>> _components;
    /// @brief The mutex.
    std::mutex _mutex;
};

} // namespace Ego::GUI

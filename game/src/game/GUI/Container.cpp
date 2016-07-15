#include "game/GUI/Container.hpp"

namespace Ego {
namespace GUI {

Container::Container()
    : components(),
      componentDestroyed(false),
      semaphore(0),
      mutex()
{}

Container::~Container()
{}

void Container::lock() {
    std::lock_guard<std::mutex> lock(mutex);
    semaphore++;
}

void Container::unlock() {
    std::lock_guard<std::mutex> lock(mutex);
    if (semaphore == 0) {
        throw new std::logic_error("container calling unlock() without lock()");
    }

    // Release one lock.
    semaphore--;

    // If all locks are released, remove all destroyed components.
    if (semaphore == 0 && componentDestroyed) {
        components.erase(std::remove_if(components.begin(), components.end(),
                                            [](std::shared_ptr<Component> component) {return component->isDestroyed(); }),
                             components.end());
        componentDestroyed = false;
    }
}

void Container::addComponent(const std::shared_ptr<Component>& component) {
    if (!component) {
        throw Id::InvalidArgumentException(__FILE__, __LINE__, "nullptr == component");
    }
    std::lock_guard<std::mutex> lock(mutex);
    components.push_back(component);
    component->setParent(this);
}

void Container::removeComponent(const std::shared_ptr<Component>& component) {
    if (!component) {
        throw Id::InvalidArgumentException(__FILE__, __LINE__, "nullptr == component");
    }
    std::lock_guard<std::mutex> lock(mutex);
    components.erase(std::remove(components.begin(), components.end(), component), components.end());
    component->setParent(nullptr);
}

void Container::notifyDestruction() {
    // Deferred destruction.
    componentDestroyed = true;
}


void Container::bringComponentToFront(std::shared_ptr<Component> component) {
    removeComponent(component);
    addComponent(component);
}

void Container::clearComponents() {
    std::lock_guard<std::mutex> lock(mutex);
    components.clear();
}

void Container::setComponentList(const std::vector<std::shared_ptr<Component>> &list) {
    clearComponents();
    for (const auto& component : list) addComponent(component);
}

size_t Container::getComponentCount() const {
    return components.size();
}

void Container::drawAll() {
    // Render the container itself.
    drawContainer();

    // Draw reach GUI component.
    _gameEngine->getUIManager()->beginRenderUI();
    for (const std::shared_ptr<Component> component : iterator()) {
        if (!component->isVisible()) continue;  // Ignore hidden/destroyed components.
        component->draw();
    }
    _gameEngine->getUIManager()->endRenderUI();
}

bool Container::notifyMouseMoved(const Events::MouseMovedEventArgs& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseMoved(e)) return true;
    }
    return false;
}

bool Container::notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEventArgs& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyKeyboardKeyPressed(e)) return true;
    }
    return false;
}

bool Container::notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first
    auto it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseButtonPressed(e)) return true;
    }
    return false;
}

bool Container::notifyMouseButtonReleased(const Events::MouseButtonReleasedEventArgs& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseButtonReleased(e)) return true;
    }
    return false;
}

bool Container::notifyMouseWheelTurned(const Events::MouseWheelTurnedEventArgs& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseWheelTurned(e)) return true;
    }
    return false;
}


} // namespace GUI
} // namespace Ego
#include "game/GUI/Container.hpp"

namespace Ego {
namespace GUI {

Container::Container()
    : _components(),
      _mutex()
{}

Container::~Container()
{}

void Container::addComponent(const std::shared_ptr<Component>& component) {
    if (!component) {
        throw Id::InvalidArgumentException(__FILE__, __LINE__, "nullptr == component");
    }
    std::lock_guard<std::mutex> lock(_mutex);
    _components.push_back(component);
    component->setParent(this);
}

void Container::removeComponent(const std::shared_ptr<Component>& component) {
    if (!component) {
        throw Id::InvalidArgumentException(__FILE__, __LINE__, "nullptr == component");
    }
    std::lock_guard<std::mutex> lock(_mutex);
    _components.erase(std::remove(_components.begin(), _components.end(), component), _components.end());
    component->setParent(nullptr);
}

void Container::bringComponentToFront(std::shared_ptr<Component> component) {
    removeComponent(component);
    addComponent(component);
}

void Container::clearComponents() {
    std::lock_guard<std::mutex> lock(_mutex);
    _components.clear();
}

void Container::setComponentList(const std::vector<std::shared_ptr<Component>> &list) {
    clearComponents();
    for (const auto& component : list) addComponent(component);
}

size_t Container::getComponentCount() const {
    return _components.size();
}

void Container::drawAll(DrawingContext& drawingContext) {
    // Render the container itself.
    drawContainer(drawingContext);

    // Draw reach GUI component.
    _gameEngine->getUIManager()->beginRenderUI();
    for (const std::shared_ptr<Component> component : iterator()) {
        if (!component->isVisible()) continue;  // Ignore hidden/destroyed components.
        component->draw(drawingContext);
    }
    _gameEngine->getUIManager()->endRenderUI();
}

bool Container::notifyMouseMoved(const Events::MouseMovedEventArgs& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    auto newEventArgs = Events::MouseMovedEventArgs(e.getPosition() - Point2f::toVector(getPosition()));
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseMoved(newEventArgs)) return true;
    }
    return false;
}

bool Container::notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEventArgs& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    auto newEventArgs = Events::KeyboardKeyPressedEventArgs(e.getKey());
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyKeyboardKeyPressed(newEventArgs)) return true;
    }
    return false;
}

bool Container::notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first
    auto newEventArgs = Events::MouseButtonPressedEventArgs(e.getPosition() - Point2f::toVector(getPosition()), e.getButton());
    auto it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseButtonPressed(newEventArgs)) return true;
    }
    return false;
}

bool Container::notifyMouseButtonReleased(const Events::MouseButtonReleasedEventArgs& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    auto newEventArgs = Events::MouseButtonReleasedEventArgs(e.getPosition() - Point2f::toVector(getPosition()), e.getButton());
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseButtonReleased(newEventArgs)) return true;
    }
    return false;
}

bool Container::notifyMouseWheelTurned(const Events::MouseWheelTurnedEventArgs& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    auto newEventArgs = Events::MouseWheelTurnedEventArgs(e.getDelta());
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseWheelTurned(newEventArgs)) return true;
    }
    return false;
}

} // namespace GUI
} // namespace Ego
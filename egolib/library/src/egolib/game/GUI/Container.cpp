#include "egolib/game/GUI/Container.hpp"
#include "egolib/game/Core/GameEngine.hpp"

namespace Ego::GUI {

Container::Container()
    : _components(),
      _mutex()
{}

Container::~Container()
{}

void Container::addComponent(const std::shared_ptr<Component>& component) {
    if (!component) {
        throw idlib::argument_null_error(__FILE__, __LINE__, "component");
    }
    std::lock_guard<std::mutex> lock(_mutex);
    _components.push_back(component);
    component->setParent(this);
}

void Container::removeComponent(const std::shared_ptr<Component>& component) {
    if (!component) {
        throw idlib::argument_null_error(__FILE__, __LINE__, "component");
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

bool Container::notifyMousePointerMoved(const Events::MousePointerMovedEvent& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    auto newEventArgs = Events::MousePointerMovedEvent(e.get_position() - idlib::semantic_cast<Vector2f>(getPosition()));
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMousePointerMoved(newEventArgs)) return true;
    }
    return false;
}

bool Container::notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEvent& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    auto newEventArgs = Events::KeyboardKeyPressedEvent(e.get_key());
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyKeyboardKeyPressed(newEventArgs)) return true;
    }
    return false;
}

bool Container::notifyMouseButtonPressed(const Events::MouseButtonPressedEvent& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first
    auto newEventArgs = Events::MouseButtonPressedEvent(e.get_position() - idlib::semantic_cast<Vector2f>(getPosition()), e.get_button());
    auto it = iterator();
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseButtonPressed(newEventArgs)) return true;
    }
    return false;
}

bool Container::notifyMouseButtonReleased(const Events::MouseButtonReleasedEvent& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    auto newEventArgs = Events::MouseButtonReleasedEvent(e.get_position() - idlib::semantic_cast<Vector2f>(getPosition()), e.get_button());
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseButtonReleased(newEventArgs)) return true;
    }
    return false;
}

bool Container::notifyMouseWheelTurned(const Events::MouseWheelTurnedEvent& e) {
    // Iterate over GUI components in reverse order so GUI components added last (i.e on top) consume events first.
    auto it = iterator();
    auto newEventArgs = Events::MouseWheelTurnedEvent(e.get_delta());
    for (auto i = it.rbegin(); i != it.rend(); ++i) {
        std::shared_ptr<Component> component = *i;
        if (!component->isEnabled()) continue;
        if (component->notifyMouseWheelTurned(newEventArgs)) return true;
    }
    return false;
}

} // namespace Ego:.GUI

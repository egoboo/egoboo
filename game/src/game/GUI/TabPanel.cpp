#include "game/GUI/TabPanel.hpp"

namespace Ego {
namespace GUI {

Tab::Tab(const std::string& title) :
    _title(title), _components() {}

void Tab::setEnabled(bool enabled) {
    for (auto& component : _components) {
        component->setEnabled(enabled);
    }
}

void Tab::setVisible(bool visible) {
    for (auto& component : _components) {
        component->setVisible(visible);
    }
}

void Tab::addComponent(const std::shared_ptr<Component>& component) {
    _components.push_back(component);
}

const std::string& Tab::getTitle() const {
    return _title;
}

void Tab::setTitle(const std::string& title) {
    _title = title;
}

} // namespace GUI
} // namespace Ego
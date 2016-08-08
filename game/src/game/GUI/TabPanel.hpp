#pragma once

#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include "game/GUI/Container.hpp"

namespace Ego {
namespace GUI {

class Tab {
public:
    Tab(const std::string& title);
    void setEnabled(bool enabled);
    void setVisible(bool visible);
    const std::string& getTitle() const;
    void setTitle(const std::string& title);
    void addComponent(const std::shared_ptr<Component>& component);
    std::string _title;
    std::vector<std::shared_ptr<Component>> _components;
};

} // namespace GUI
} // namespace Ego
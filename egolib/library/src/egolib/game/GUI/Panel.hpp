#pragma once

#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include "egolib/game/GUI/Container.hpp"

namespace Ego {
namespace GUI {

/// A panel.
class Panel : public Container {
public:
    /// Construct this panel.
    Panel();
    void draw(DrawingContext& drawingContext) override;
    void drawContainer(DrawingContext& drawingContext) override;
    Vector2f getDesiredSize();
};

} // namespace GUI
} // namespace Ego

#pragma once

#include "egolib/game/GUI/Container.hpp"

namespace Ego::GUI {

/// A panel.
class Panel : public Container {
public:
    /// Construct this panel.
    Panel();
    void draw(DrawingContext& drawingContext) override;
    void drawContainer(DrawingContext& drawingContext) override;
    Vector2f getDesiredSize();
};

} // namespace Ego::GUI

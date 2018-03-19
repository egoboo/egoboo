#pragma once

#include "egolib/game/GUI/Container.hpp"

namespace Ego::GUI {

class Tab : public Container {
public:
    Tab(const std::string& title);
    const std::string& getTitle() const;
    void setTitle(const std::string& title);
    void draw(DrawingContext& drawingContext) override;
    void drawContainer(DrawingContext& drawingContext) override;
    std::string _title;
    Vector2f getDesiredSize();
};

} // namespace Ego::GUI
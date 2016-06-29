#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Button.hpp"

namespace Ego {
namespace GUI {

class OptionsButton : public Button {
public:
    OptionsButton(const std::string &label);

    virtual void draw() override;

    void setPosition(float x, float y) override;

private:
    Label _label;
};

} // namespace GUI
} // namespace Ego

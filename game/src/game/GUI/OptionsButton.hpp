#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Button.hpp"

class OptionsButton : public Button
{
public:
    OptionsButton(const std::string &label);

    virtual void draw() override;

    void setPosition(const int x, const int y) override;

private:
    Label _label;
};

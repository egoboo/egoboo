#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/GUIComponent.hpp"

class Label : public GUIComponent
{
public:
    Label(const std::string &text);

    virtual void draw() override;
       
    void setText(const std::string &LabelText);

private:
    std::string _text;
};

#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/GUIComponent.hpp"

class Label : public GUIComponent
{
public:
    Label(const std::string &text);

    virtual void draw() override;
       
    void setText(const std::string &LabelText);

    void setFont(const std::shared_ptr<Ego::Font> &font);
    
private:
    std::string _text;
    std::shared_ptr<Ego::Font> _font;
};

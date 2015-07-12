#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/GUIComponent.hpp"

class Label : public GUIComponent
{
public:
    /**
    * @brief
    *   Default construct an empty label with no text
    **/
    Label(const std::string &text = "", const UIManager::UIFontType font = UIManager::FONT_DEFAULT);

    virtual void draw() override;
       
    void setText(const std::string &LabelText);

    void setFont(const std::shared_ptr<Ego::Font> &font);

    /**
    * @brief
    *   changes the colour of the text when rendering this label
    * @remark
    *   the default color is white
    **/
    void setColor(const Ego::Math::Colour4f& color);

    /**
    * @brief
    *   changes the alpha (transparency) of this label
    * @param a
    *   a value between 0 and 1 (else it will throw an exception)
    * @remark
    *   the default color is 1.0f (no transparency)
    **/
    void setAlpha(const float a);

    /**
    * @return
    *   get the current colour of this Label
    **/
    const Ego::Math::Colour4f& getColour() const;
    
private:
    std::string _text;
    std::shared_ptr<Ego::Font> _font;
    Ego::Math::Colour4f _color;
};

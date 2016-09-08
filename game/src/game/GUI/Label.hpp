#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/Component.hpp"

namespace Ego {
namespace GUI {

/// @brief A label is text enclosed in a rectangle.
class Label : public Component {
public:
    /// @brief Default construct this label with default values.
    /// The default values are the empty string and the default font.
    Label();
    /// @brief Construct this label with the specified values.
    /// @param text the text of this label
    /// @param font the font of this label. Default is UIManager::FONT_DEFAULT.
    Label(const std::string& text, const UIManager::UIFontType font = UIManager::FONT_DEFAULT);

    virtual void draw(DrawingContext& drawingContext) override;

    /**@{*/

    /// @brief Get the text of this label.
    /// @return the text of this label
    const std::string& getText() const;

    /// @brief Set the text of this label.
    /// @param text the text of this label
    /// @default The default text of this label is the empty string.
    void setText(const std::string& text);

    /**@}*/

    /**@{*/

    /// @brief Get the font of this label.
    /// @return the font of this label
    const std::shared_ptr<Font>& getFont() const;

    /// @brief Set the font of this label.
    /// @param textFont the font of this label
    void setFont(const std::shared_ptr<Font>& font);

    /**@}*/

    /**@{*/

    /// @brief Get the colour of this label.
    /// @return the colour of this label
    const Math::Colour4f& getColour() const;

    /// @brief Set the colour of this label.
    /// @param colour the colour
    /// @default The default colour is opaque white.
    void setColour(const Math::Colour4f& colour);

    /**@}*/

    /**@{*/

    /// @brief Get the alpha (transparency) of this label.
    /// @return the default alpha (transparency) of this label
    float getAlpha() const;

    /// @brief Set the alpha (transparency) of tis label.
    /// @param a the alpha (transparency) of this label
    /// @default The default alpha (transparency) is 1.0f.
    void setAlpha(float a);

    /**@}*/

private:
    std::string _text;
    std::shared_ptr<Font> _font;
    Math::Colour4f _colour;
private:
    std::shared_ptr<Font::LaidTextRenderer> _textRenderer;
};

} // namespace GUI
} // namespace Ego

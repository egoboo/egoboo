#include "game/GUI/Label.hpp"

namespace Ego {
namespace GUI {

Label::Label() : Label(std::string()) {
}

Label::Label(const std::string &text, const UIManager::UIFontType font) :
    _text(text),
    _font(_gameEngine->getUIManager()->getFont(font)),
    _textRenderer(),
    _colour(Math::Colour4f::white()) {
    if (!text.empty()) {
        setText(text);
    }
}

void Label::draw() {
    // Draw text.
    if (_textRenderer)
        _textRenderer->render(getX(), getY(), _colour);
}

const std::string& Label::getText() const {
    return _text;
}

void Label::setText(const std::string& text) {
    _text = text;

    // Recalculate our size.
    int textWidth, textHeight;
    _textRenderer = _font->layoutTextBox(_text, 0, 0, _font->getLineSpacing(), &textWidth, &textHeight);
    setSize(Vector2f(textWidth, textHeight));
}

const std::shared_ptr<Font>& Label::getFont() const {
    return _font;
}

void Label::setFont(const std::shared_ptr<Font>& font) {
    _font = font;

    // Recalculate our size.
    int textWidth, textHeight;
    _textRenderer = _font->layoutTextBox(_text, 0, 0, _font->getLineSpacing(), &textWidth, &textHeight);
    setSize(Vector2f(textWidth, textHeight));
}

const Math::Colour4f& Label::getColour() const {
    return _colour;
}

void Label::setColour(const Math::Colour4f& colour) {
    _colour = colour;
}

void Label::setAlpha(float a) {
    _colour.setAlpha(a);
}

float Label::getAlpha() const {
    return _colour.getAlpha();
}

} // namespace GUI
} // namespace Ego
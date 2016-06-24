#include "game/GUI/Label.hpp"

namespace Ego {
namespace GUI {

Label::Label(const std::string &text, const UIManager::UIFontType font) :
    _text(text),
    _font(_gameEngine->getUIManager()->getFont(font)),
    _textRenderer(),
    _color(Ego::Math::Colour4f::white()) {
    if (!text.empty()) {
        setText(text);
    }
}

void Label::draw() {
    //Draw text
    if (_textRenderer)
        _textRenderer->render(getX(), getY(), _color);
}

void Label::setText(const std::string &text) {
    _text = text;

    //Recalculate our size
    int textWidth, textHeight;
    _textRenderer = _font->layoutTextBox(_text, 0, 0, _font->getLineSpacing(), &textWidth, &textHeight);
    setSize(textWidth, textHeight);
}

void Label::setFont(const std::shared_ptr<Ego::Font> &font) {
    _font = font;

    //Recalculate our size
    int textWidth, textHeight;
    _textRenderer = _font->layoutTextBox(_text, 0, 0, _font->getLineSpacing(), &textWidth, &textHeight);
    setSize(textWidth, textHeight);
}

void Label::setColor(const Ego::Math::Colour4f& color) {
    _color = color;
}

void Label::setAlpha(const float a) {
    _color.setAlpha(a);
}

const Ego::Math::Colour4f& Label::getColour() const {
    return _color;
}

} // namespace GUI
} // namespace Ego
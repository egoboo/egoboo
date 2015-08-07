//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file game/GameStates/DebugModuleLoadingState.cpp
/// @details Debugging state where one can debug font layout and rendering
/// @author Johan Jansen, penguinflyer5234

#include "game/GameStates/DebugFontRenderingState.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/ScrollableList.hpp"

#include "game/renderer_2d.h"

class DebugFontRenderingState::DebugLabel : public GUIComponent
{
public:
    DebugLabel(const std::string &text) :
    _text(text),
    _font(_gameEngine->getUIManager()->getFont(UIManager::FONT_DEFAULT)),
    _textRenderer(),
    _maxColor(0, 0, .7, 1),
    _textColor(0, 1, 0, .4),
    _maxWidth(0),
    _maxHeight(0),
    _textWidth(0),
    _textHeight(0)
    {
        redraw();
    }
    
    void draw() override {
        ego_frect_t src, tx;
        
        src.xmin = getX();
        src.ymin = getY();
        src.xmax = src.xmin + _maxWidth;
        src.ymax = src.ymin + _maxHeight;
        draw_quad_2d(nullptr, src, tx, true, _maxColor);
        
        src.xmax = src.xmin + _textWidth;
        src.ymax = src.ymin + _textHeight;
        draw_quad_2d(nullptr, src, tx, true, _textColor);
        
        _textRenderer->render(getX(), getY());
    }
    
    void setFont(const std::shared_ptr<Ego::Font> &font) {
        _font = font;
        redraw();
    }
    
    void setMaxWidth(int maxWidth) {
        _maxWidth = maxWidth;
        redraw();
    }
    
    void setMaxHeight(int maxHeight) {
        _maxHeight = maxHeight;
        redraw();
    }
    
private:
    void redraw() {
        int textWidth, textHeight;
        _textRenderer = _font->layoutTextBox(_text, _maxWidth, _maxHeight, 0, &textWidth, &textHeight);
        _textWidth = textWidth;
        _textHeight = textHeight;
        if (_maxWidth > 0)
            textWidth = _maxWidth;
        if (_maxHeight > 0)
            textHeight = _maxHeight;
        setSize(textWidth, textHeight);
    }
    
    std::string _text;
    std::shared_ptr<Ego::Font> _font;
    std::shared_ptr<Ego::Font::LaidTextRenderer> _textRenderer;
    Ego::Math::Colour4f _maxColor, _textColor;
    int _maxWidth, _maxHeight;
    int _textWidth, _textHeight;
};

DebugFontRenderingState::DebugFontRenderingState()
{
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();
    using UIFontType = UIManager::UIFontType;
    
    int x = 125;
    int width = (SCREEN_WIDTH - 110) / 4 - 20;
    
    std::stringstream debugTextStream;
    std::vector<char> ascii{10};
    for (char i = 32; i < 127; i++) ascii.emplace_back(i);
    
    for (int i = 0; i < 2560; i++)
        debugTextStream << Random::getRandomElement(ascii);
    
    std::string debugText = debugTextStream.str();
    _textLabel = std::make_shared<DebugLabel>(debugText);
    _textLabel->setPosition(5, 5);
    addComponent(_textLabel);
    
    auto button = std::make_shared<Button>("Back");
    button->setOnClickFunction([this] { endState(); } );
    button->setWidth(100);
    button->setPosition(5, SCREEN_HEIGHT - button->getHeight() - 5);
    addComponent(button);
    
    button = std::make_shared<Button>("Default");
    button->setOnClickFunction([this] { setFont(UIFontType::FONT_DEFAULT); });
    button->setWidth(width);
    button->setPosition(x, SCREEN_HEIGHT - button->getHeight() - 5);
    x += width + 20;
    addComponent(button);
    
    button = std::make_shared<Button>("Floating Text");
    button->setOnClickFunction([this] { setFont(UIFontType::FONT_FLOATING_TEXT); });
    button->setWidth(width);
    button->setPosition(x, SCREEN_HEIGHT - button->getHeight() - 5);
    x += width + 20;
    addComponent(button);
    
    button = std::make_shared<Button>("Debug");
    button->setOnClickFunction([this] { setFont(UIFontType::FONT_DEBUG); });
    button->setWidth(width);
    button->setPosition(x, SCREEN_HEIGHT - button->getHeight() - 5);
    x += width + 20;
    addComponent(button);
    
    button = std::make_shared<Button>("Game");
    button->setOnClickFunction([this] { setFont(UIFontType::FONT_GAME); });
    button->setWidth(width);
    button->setPosition(x, SCREEN_HEIGHT - button->getHeight() - 5);
    x += width + 20;
    addComponent(button);
    
    x = 5;
    _missingSpace = button->getHeight() * 2 + 30;
    int y = SCREEN_HEIGHT - _missingSpace + 20;
    
    _maxWidth = SCREEN_WIDTH - 10;
    _maxHeight = SCREEN_HEIGHT - _missingSpace;
    
    _textLabel->setMaxWidth(_maxWidth);
    _textLabel->setMaxHeight(_maxHeight);
    
    auto label = std::make_shared<Label>("Width");
    label->setPosition(x, y);
    addComponent(label);
    x += label->getWidth() + 15;
    
    button = std::make_shared<Button>("<<");
    button->setPosition(x, y);
    button->setWidth(35);
    button->setOnClickFunction([this] { setMaxWidth(0); });
    addComponent(button);
    x += 40;
    
    button = std::make_shared<Button>("<");
    button->setPosition(x, y);
    button->setWidth(35);
    button->setOnClickFunction([this] { setMaxWidth(_maxWidth - 1); });
    addComponent(button);
    x += 40;
    
    _widthButton = std::make_shared<Button>(std::to_string(_maxWidth));
    _widthButton->setPosition(x, y);
    _widthButton->setWidth(75);
    addComponent(_widthButton);
    x += 80;
    
    button = std::make_shared<Button>(">");
    button->setPosition(x, y);
    button->setWidth(35);
    button->setOnClickFunction([this] { setMaxWidth(_maxWidth + 1); });
    addComponent(button);
    x += 40;
    
    button = std::make_shared<Button>(">>");
    button->setPosition(x, y);
    button->setWidth(35);
    button->setOnClickFunction([this] { setMaxWidth(std::numeric_limits<int>::max()); });
    addComponent(button);
    x += 55;
    
    label = std::make_shared<Label>("Height");
    label->setPosition(x, y);
    addComponent(label);
    x += label->getWidth() + 15;
    
    button = std::make_shared<Button>("<<");
    button->setPosition(x, y);
    button->setWidth(35);
    button->setOnClickFunction([this] { setMaxHeight(0); });
    addComponent(button);
    x += 40;
    
    button = std::make_shared<Button>("<");
    button->setPosition(x, y);
    button->setWidth(35);
    button->setOnClickFunction([this] { setMaxHeight(_maxHeight - 1); });
    addComponent(button);
    x += 40;
    
    _heightButton = std::make_shared<Button>(std::to_string(_maxHeight));
    _heightButton->setPosition(x, y);
    _heightButton->setWidth(75);
    addComponent(_heightButton);
    x += 80;
    
    button = std::make_shared<Button>(">");
    button->setPosition(x, y);
    button->setWidth(35);
    button->setOnClickFunction([this] { setMaxHeight(_maxHeight + 1); });
    addComponent(button);
    x += 40;
    
    button = std::make_shared<Button>(">>");
    button->setPosition(x, y);
    button->setWidth(35);
    button->setOnClickFunction([this] { setMaxHeight(std::numeric_limits<int>::max()); });
    addComponent(button);
    x += 55;
}

void DebugFontRenderingState::setFont(UIManager::UIFontType font) {
    _textLabel->setFont(_gameEngine->getUIManager()->getFont(font));
}

void DebugFontRenderingState::setMaxHeight(int maxHeight) {
    _maxHeight = Ego::Math::constrain(maxHeight, 0, _gameEngine->getUIManager()->getScreenHeight() - _missingSpace);
    _textLabel->setMaxHeight(_maxHeight);
    _heightButton->setText(std::to_string(_maxHeight));
}

void DebugFontRenderingState::setMaxWidth(int maxWidth) {
    _maxWidth = Ego::Math::constrain(maxWidth, 0, _gameEngine->getUIManager()->getScreenWidth() - 10);
    _textLabel->setMaxWidth(_maxWidth);
    _widthButton->setText(std::to_string(_maxWidth));
}

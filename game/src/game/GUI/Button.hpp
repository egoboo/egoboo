#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/GUIComponent.hpp"

class Button : public GUIComponent
{
    public:
        Button(int hotkey = SDLK_UNKNOWN);
        Button(const std::string &buttonText, int hotkey = SDLK_UNKNOWN);

        virtual void draw() override;
        void setOnClickFunction(const std::function<void()> onClick);
        void setText(const std::string &buttonText);

        bool notifyMouseMoved(const Ego::Events::MouseMovedEventArgs& e) override;
        bool notifyMouseClicked(const Ego::Events::MouseClickedEventArgs& e) override;
        bool notifyKeyDown(const int keyCode) override;

        void doClick();

        void beginSlidyButtonEffect(float offset);

        bool isEnabled() const override;

        void setEnabled(const bool enabled) override;

    protected:
        void updateSlidyButtonEffect();

    protected:
        bool _mouseOver;
        std::shared_ptr<Ego::Font::LaidTextRenderer> _buttonTextRenderer;
        int _buttonTextWidth;
        int _buttonTextHeight;

        static const Ego::Math::Colour4f DEFAULT_BUTTON_COLOUR;
        static const Ego::Math::Colour4f HOVER_BUTTON_COLOUR;
        static const Ego::Math::Colour4f DISABLED_BUTTON_COLOUR;

    private:
        std::string _buttonText;
        std::function<void()> _onClickFunction;
        int _hotkey;
        float _slidyButtonTargetX;
        float _slidyButtonCurrentX;
};

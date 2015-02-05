#pragma once

#include "game/core/GameEngine.hpp"
#include "game/gui/GUIComponent.hpp"

class Button : public GUIComponent
{
    public:
        Button(int hotkey = SDLK_UNKNOWN);
        Button(const std::string &buttonText, int hotkey = SDLK_UNKNOWN);

        virtual void draw() override;
        void setOnClickFunction(const std::function<void()> onClick);
        void setText(const std::string &buttonText);

        bool notifyMouseMoved(const int x, const int y) override;
        bool notifyMouseClicked(const int button, const int x, const int y) override;
        bool notifyKeyDown(const int keyCode) override;

        void doClick();

        void beginSlidyButtonEffect(float offset);

        bool isEnabled() const override;

        void setEnabled(bool enabled) override;

        //Disable copying class
        Button(const Button& copy) = delete;
        Button& operator=(const Button&) = delete;

    protected:
        bool _mouseOver;

        static const GLXvector4f DEFAULT_BUTTON_COLOUR;
        static const GLXvector4f HOVER_BUTTON_COLOUR;
        static const GLXvector4f DISABLED_BUTTON_COLOUR;

    private:
        std::string _buttonText;
        std::function<void()> _onClickFunction;
        int _hotkey;
        float _slidyButtonTargetX;
};

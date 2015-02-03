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
        void setClickSound(const std::string &soundID);
        void setText(const std::string &buttonText);

        bool notifyMouseMoved(const int x, const int y) override;
        bool notifyMouseClicked(const int button, const int x, const int y) override;
        bool notifyKeyDown(const int keyCode) override;

        void doClick();

        void beginSlidyButtonEffect();

        //Disable copying class
        Button(const Button& copy) = delete;
        Button& operator=(const Button&) = delete;

    private:
        oglx_texture_t _buttonSprite;
        std::string _buttonText;
        std::string _clickSoundID;
        bool _mouseOver;
        std::function<void()> _onClickFunction;
        int _hotkey;
        float _slidyButtonTargetX;
};

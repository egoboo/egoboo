#pragma once

#include "game/core/GameEngine.hpp"
#include "game/gui/GUIComponent.hpp"

class Label : public GUIComponent
{
    public:
        Label(const std::string &text);

        virtual void draw() override;
       
        void setText(const std::string &LabelText);

        //Disable copying class
        Label(const Label& copy) = delete;
        Label& operator=(const Label&) = delete;

    private:
        std::string _text;
};

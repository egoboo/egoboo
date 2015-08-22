#pragma once

#include "game/GUI/GUIComponent.hpp"

namespace GUI
{

class ProgressBar : public GUIComponent
{
    public:
        ProgressBar();
        virtual void draw() override;

        void setValue(float value);
        void setMaxValue(float value);
        void setTickWidth(float value);

    private:
        float _currentValue;
        float _maxValue;
        float _tickWidth;
};

} //namespace GUI
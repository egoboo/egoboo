#pragma once

#include "egolib/game/GUI/Component.hpp"

namespace Ego::GUI {

class ProgressBar : public Component {
public:
    ProgressBar();
    virtual void draw(DrawingContext& drawingContext) override;

    void setValue(float value);
    void setMaxValue(float value);
    void setTickWidth(float value);

private:
    float _currentValue;
    float _maxValue;
    float _tickWidth;
};

} // namespace Ego::GUI

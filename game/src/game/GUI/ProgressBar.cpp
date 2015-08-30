#include "ProgressBar.hpp"

namespace GUI
{

ProgressBar::ProgressBar() :
    _currentValue(0.0f),
    _maxValue(100.0f),
    _tickWidth(0.0f)
{

}

void ProgressBar::draw()
{
    struct Vertex
    {
        float x, y;
    };

    auto &renderer = Ego::Renderer::get();
    const auto &vb = _gameEngine->getUIManager()->_vertexBuffer;
	renderer.getTextureUnit().setActivated(nullptr);

    // Draw the bar background
    renderer.setColour(Ego::Math::Colour4f(Ego::Math::Colour3f::grey(), 0.5f));

    Vertex *v = static_cast<Vertex *>(vb->lock());
    v->x = getX(); v->y = getY(); v++;
    v->x = getX(); v->y = getY() + getHeight(); v++;
    v->x = getX() + getWidth(); v->y = getY() + getHeight(); v++;
    v->x = getX() + getWidth(); v->y = getY();
    vb->unlock();
    renderer.render(*vb, Ego::PrimitiveType::Quadriliterals, 0, 4);

    //Draw progress
    const int BAR_EDGE = 2;
    renderer.setColour(Ego::Math::Colour4f(Ego::Math::Colour3f::purple(), 0.8f));

    const float progressWidth = (getWidth()-BAR_EDGE*2) * (_currentValue/_maxValue);

    v = static_cast<Vertex *>(vb->lock());
    v->x = getX()+BAR_EDGE; v->y = getY()+BAR_EDGE; v++;
    v->x = getX()+BAR_EDGE; v->y = getY()+BAR_EDGE + getHeight()-BAR_EDGE*2; v++;
    v->x = getX()+BAR_EDGE + progressWidth; v->y = getY()+BAR_EDGE + getHeight()-BAR_EDGE*2; v++;
    v->x = getX()+BAR_EDGE + progressWidth; v->y = getY()+BAR_EDGE;
    vb->unlock();
    renderer.render(*vb, Ego::PrimitiveType::Quadriliterals, 0, 4);

    //Draw ticks if needed
    if(_tickWidth > 0.0f) {
        const int numberOfTicks = _maxValue / _tickWidth;
        const float actualTickWidth = static_cast<float>(getWidth()) / numberOfTicks;

        renderer.setColour(Ego::Math::Colour4f::black());

        for(int i = 1; i < numberOfTicks; ++i) {
            v = static_cast<Vertex *>(vb->lock());
            v->x = getX()+BAR_EDGE + actualTickWidth*i; v->y = getY(); v++;
            v->x = getX()+BAR_EDGE + actualTickWidth*i; v->y = getY() + getHeight(); v++;
            v->x = getX()+BAR_EDGE + actualTickWidth*i + BAR_EDGE; v->y = getY() + getHeight(); v++;
            v->x = getX()+BAR_EDGE + actualTickWidth*i + BAR_EDGE; v->y = getY();
            vb->unlock();
            renderer.render(*vb, Ego::PrimitiveType::Quadriliterals, 0, 4);
        }
    }
}

void ProgressBar::setValue(float value)
{
    if(value > _maxValue) return;
    _currentValue = value;
}

void ProgressBar::setMaxValue(float value)
{
    if(value <= 0.0f) return;
    if(_maxValue < _currentValue) _currentValue = value;
    _maxValue = value;
}

void ProgressBar::setTickWidth(float value)
{
    _tickWidth = value;
}

} //namespace GUI
#include "egolib/Graphics/Viewport.hpp"

namespace Ego {
namespace Graphics {

/// Utility function to set a value.
template <typename T>
void SetProperty(T& storage, T value, idlib::signal<void(T, T)>& signal)
{
    if (storage != value)
    {
        auto oldValue = storage;
        storage = value;
        signal(oldValue, value);
    }
}


Viewport::Viewport() :
    m_leftPixels(0), m_topPixels(0),
    m_widthPixels(800), m_heightPixels(600),
    m_clearColour(0.0f, 0.0f, 0.0f, 0.0f),
    m_clearDepth(1.0f)
{}

Viewport::~Viewport()
{}


float Viewport::getLeftPixels() const
{
    return m_leftPixels;
}

void Viewport::setLeftPixels(float leftPixels)
{
    SetProperty(m_leftPixels, leftPixels, LeftPixelsChanged);
}


float Viewport::getTopPixels() const
{
    return m_topPixels;
}

void Viewport::setTopPixels(float topPixels)
{
    SetProperty(m_topPixels, topPixels, TopPixelsChanged);
}


float Viewport::getWidthPixels() const
{
    return m_widthPixels;
}

void Viewport::setWidthPixels(float widthPixels)
{
    SetProperty(m_widthPixels, widthPixels, WidthPixelsChanged);
}


float Viewport::getHeightPixels() const
{
    return m_heightPixels;
}

void Viewport::setHeightPixels(float heightPixels)
{
    SetProperty(m_heightPixels, heightPixels, HeightPixelsChanged);
}


const Ego::Colour4f& Viewport::getClearColour() const
{
    return m_clearColour;
}

void Viewport::setClearColour(const Ego::Colour4f& clearColour)
{
    SetProperty(m_clearColour, clearColour, ClearColourChanged);
}


float Viewport::getClearDepth() const
{
    return m_clearDepth;
}

void Viewport::setClearDepth(float clearDepth)
{
    SetProperty(m_clearDepth, clearDepth, ClearDepthChanged);
}

} // namespace Graphics
} // namespace Ego

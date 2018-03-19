#include "egolib/Graphics/Viewport.hpp"

namespace Ego::Graphics {

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
    m_absolute_rectangle({ 0,0 }, { 800, 600 })
{}

Viewport::~Viewport()
{}

idlib::rectangle_2s Viewport::absolute_rectangle() const
{ return m_absolute_rectangle; }

void Viewport::absolute_rectangle(const idlib::rectangle_2s& absolute_rectangle)
{ SetProperty(m_absolute_rectangle, absolute_rectangle, absolute_rectangle_changed); }

} // namespace Ego::Graphics

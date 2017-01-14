#include "egolib/Graphics/SDL/DisplayMode.hpp"

namespace Ego {
namespace SDL {

DisplayMode::DisplayMode(GraphicsSystemNew *graphicsSystem, SDL_DisplayMode& displayMode) :
    graphicsSystem(graphicsSystem), displayMode(displayMode)
{
    if (!graphicsSystem)
    {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == graphicsSystem");
    }
}

bool DisplayMode::compare(const DisplayMode& other) const
{
    return displayMode.format == other.displayMode.format
        && displayMode.w == other.displayMode.w
        && displayMode.h == other.displayMode.h
        && displayMode.refresh_rate == other.displayMode.refresh_rate;
}

bool DisplayMode::compare(const Ego::DisplayMode& other) const
{
    auto pother = dynamic_cast<const DisplayMode *>(&other);
    if (!pother) return false;
    else return compare(*pother);
}

void *DisplayMode::get() const
{
    return (void *)&displayMode;
}

int DisplayMode::getHorizontalResolution() const
{
    return displayMode.w;
}

int DisplayMode::getVerticalResolution() const
{
    return displayMode.h;
}

int DisplayMode::getRefreshRate() const
{
    return displayMode.refresh_rate;
}

} // namespace SDL
} // namespace Ego

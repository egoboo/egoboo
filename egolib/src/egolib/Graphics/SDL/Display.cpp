#include "egolib/Graphics/SDL/Display.hpp"
#include "egolib/Graphics/SDL/DisplayMode.hpp"

namespace Ego {
namespace SDL {

Display::Display(GraphicsSystemNew *graphicsSystem, int displayIndex) :
    graphicsSystem(graphicsSystem), displayIndex(displayIndex)
{
    if (!graphicsSystem)
    {
        throw id::runtime_error(__FILE__, __LINE__, "nullptr == graphicsSystem");
    }
    for (int i = 0, n = SDL_GetNumDisplayModes(displayIndex); i < n; ++i)
    {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(displayIndex, i, &mode);
        displayModes.push_back(std::make_shared<DisplayMode>(graphicsSystem, mode));
    }
    if (-1 == SDL_GetDisplayBounds(displayIndex, &displayBounds))
    {
        throw id::environment_error(__FILE__, __LINE__, "[SDL]", "unable to get display bounds");
    }
}

bool Display::compare(const Display& other) const
{
    return displayIndex == other.displayIndex;
}

bool Display::compare(const Ego::Display& other) const
{
    auto pother = dynamic_cast<const Display *>(&other);
    if (!pother) return false;
    else return compare(*pother);
}

bool Display::isPrimaryDisplay() const
{
    return 0 == displayBounds.x
        && 0 == displayBounds.y;
}

int Display::getDisplayIndex() const
{
    return displayIndex;
}

void Display::update()
{
    displayModes.clear();
    for (int i = 0, n = SDL_GetNumDisplayModes(displayIndex); i < n; ++i)
    {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(displayIndex, i, &mode);
        displayModes.push_back(std::make_shared<DisplayMode>(graphicsSystem, mode));
    }
    if (-1 == SDL_GetDisplayBounds(displayIndex, &displayBounds))
    {
        throw id::environment_error(__FILE__, __LINE__, "[SDL]", "unable to get display bounds");
    }
}

} // namespace SDL
} // namespace Ego

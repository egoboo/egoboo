#include "egolib/Graphics/SDL/DisplayMode.hpp"

namespace Ego {
namespace SDL {

DisplayMode::DisplayMode(SDL_DisplayMode& displayMode)
    : _displayMode(displayMode) {
    /* Intentionally empty. */
}

bool DisplayMode::compare(const DisplayMode& other) const {
    return _displayMode.format == other._displayMode.format
        && _displayMode.w == other._displayMode.w
        && _displayMode.h == other._displayMode.h
        && _displayMode.refresh_rate == other._displayMode.refresh_rate;
}

bool DisplayMode::compare(const Ego::DisplayMode& other) const {
    auto pother = dynamic_cast<const DisplayMode *>(&other);
    if (!pother) return false;
    else return compare(*pother);
}

void *DisplayMode::get() const {
    return (void *)&_displayMode;
}

int DisplayMode::getHorizontalResolution() const {
    return _displayMode.w;
}

int DisplayMode::getVerticalResolution() const {
    return _displayMode.h;
}

int DisplayMode::getRefreshRate() const {
    return _displayMode.refresh_rate;
}

} // namespace SDL
} // namespace Ego

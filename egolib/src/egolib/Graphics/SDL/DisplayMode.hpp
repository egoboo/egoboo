#pragma once

#include "egolib/Graphics/DisplayMode.hpp"

namespace Ego {
namespace SDL {

/// @brief Display mode implementation for SDL.
struct DisplayMode : Ego::DisplayMode
{
private:
    SDL_DisplayMode displayMode;

public:
    DisplayMode(SDL_DisplayMode& displayMode);

protected:
    bool compare(const DisplayMode& other) const;
    bool compare(const Ego::DisplayMode& other) const override;
    void *get() const override;

public:
    int getHorizontalResolution() const override;
    int getVerticalResolution() const override;
    int getRefreshRate() const override;
};

} // namespace SDL
} // namespace Ego

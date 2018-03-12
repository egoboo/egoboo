#pragma once

#include "egolib/Graphics/DisplayMode.hpp"

namespace Ego::SDL {

// Forward declaration.
class GraphicsSystemNew;

/// @brief Display mode implementation for SDL.
class DisplayMode : public Ego::DisplayMode
{
private:
    /// @brief The SDL display mode.
    SDL_DisplayMode displayMode;

    /// @brief A pointer to the SDL graphics system.
    GraphicsSystemNew *graphicsSystem;

public:
    /// @brief Construct this SDL display mode.
    /// @param graphicsSystem a pointer to the SDL graphics system
    /// @param displayMode the SDL display mode
    DisplayMode(GraphicsSystemNew *graphicsSystem, SDL_DisplayMode& displayMode);

protected:
    bool compare(const DisplayMode& other) const;
    bool compare(const Ego::DisplayMode& other) const override;
    void *get() const override;

public:
    int getHorizontalResolution() const override;
    int getVerticalResolution() const override;
    int getRefreshRate() const override;
};

} // namespace Ego::SDL

#pragma once

#include "egolib/Graphics/Display.hpp"

namespace Ego {
namespace SDL {

// Forward declaration.
struct GraphicsSystemNew;

/// @brief Display implementation for SDL.
struct Display : Ego::Display
{
private:
    /// @brief The display index of this display.
    int displayIndex;

    /// @brief The display bounds.
    SDL_Rect displayBounds;

    /// @brief A pointer to the SDL graphics system.
    GraphicsSystemNew *graphicsSystem;

public:
    /// @brief Construct this SDL display.
    /// @param graphicsSystem a pointer to the SDL graphics system
    /// @param displayIndex the display index of this display
    Display(GraphicsSystemNew *graphicsSystem, int displayIndex);

    /** @copydoc Ego::Display::getDisplayIndex() */
    int getDisplayIndex() const override;

    /** @copydoc Ego::Display::update */
    void update() override;

    /** @copydoc Ego::Display::isPrimaryDisplay */
    bool isPrimaryDisplay() const override;

protected:
    bool compare(const Display& other) const;
    bool compare(const Ego::Display& other) const override;
};

} // namespace SDL
} // namespace Ego

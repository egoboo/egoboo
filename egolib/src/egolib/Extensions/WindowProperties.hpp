#pragma once

#include "egolib/Log/_Include.hpp"

namespace Ego {

/// Properties of a window.
struct WindowProperties {
    /// A fullscreen window.
    /// @remark If this is @a true, then the SDL_WINDOW_FULLSCREEN flag is set.
    /// @warning fulldesktop and fullscreen are mutually exclusive.
    unsigned fullscreen : 1;

    /// A fulldesktop window.
    /// @remark A fulldesktop window always covers the entire desktop or is minimized.
    /// @remark If this is @a true, then the SDL_WINDOW_FULLSCREEN_DESKTOP flag is set.
    /// @warning fulldesktop and fullscreen are mutually exclusive.
    unsigned fulldesktop : 1;

    /// Support for OpenGL
    /// If this is @a true, then the SDL_WINDOW_OPENGL flag is set.
    unsigned opengl : 1;

    /// Support for resizing.
    /// If this is @a true, then the SDL_WINDOW_RESIZABLE flag is set.
    unsigned resizable : 1;

    /// No window caption or edge frame
    /// If this is @a true, then the SDL_WINDOW_BORDERLESS flag is set.
    unsigned borderless : 1;

    /// Does the window support high-DPI modes (in Apple terminology 'Retina').
    /// If this is @a true, then the SDL_WINDOW_ALLOW_HIGHDPI is set.
    unsigned highdpi : 1;

public:
    /**
     * @brief Construct these window properties with "reasonable" default values.
     */
    WindowProperties();
    /**
     * @brief Copy-construct these window properties.
     * @param other the other window properties
     */
    WindowProperties(const WindowProperties& other);
    /**
     * @brief Assign this window flags.
     * @param other the other window flags
     * @return this window flags
     */
    const WindowProperties& operator=(const WindowProperties& other);
   
public:
    /// @brief Encode these window properties to an SDL value.
    uint32_t upload() const;
    /// @brief Encode an SDL value into these window properties.
    void download(uint32_t source);

}; // struct WindowProperties

} // namespace Ego

Log::Entry& operator<<(Log::Entry& target, const Ego::WindowProperties& source);

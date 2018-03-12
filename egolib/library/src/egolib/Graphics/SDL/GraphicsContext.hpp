#pragma once

#include "egolib/Graphics/GraphicsContext.hpp"

namespace Ego::SDL {

// Forward declaration.
class GraphicsWindow;

/// @brief An SDL/OpenGL context.
class GraphicsContext : public Ego::GraphicsContext
{
private:
    GraphicsWindow *window;
    SDL_GLContext context;

public:
    /// @brief Construct this graphics context with the specified window and the specified graphics context properties.
    /// @param window a pointer to the window
    /// @throw idlib::runtime_error @a window is a null pointer
    /// @throw idlib::runtime_error context creation failed
    GraphicsContext(GraphicsWindow *window);

    /// @brief Destruct this graphics context.
    virtual ~GraphicsContext();

}; // class GraphicsContext

} // namespace Ego::SDL

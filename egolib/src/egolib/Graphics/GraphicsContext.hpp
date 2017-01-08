#pragma once

#include "egolib/Extensions/ContextProperties.hpp"

namespace Ego {

struct GraphicsWindow;

struct GraphicsContext
{
private:
    GraphicsWindow *window;
    SDL_GLContext context;
public:
    /// @brief Construct this graphics context with the specified window and the specified graphics context properties.
    /// @param window a pointer to the window
    /// @param contextProperties the context properties
    /// @throw Id::RuntimeErrorException @a window is a null pointer
    /// @throw Id::RuntimeErrorException context creation failed
    GraphicsContext(GraphicsWindow *window, const ContextProperties& contextProperties);

    /// @brief Destruct this graphics context.
    virtual ~GraphicsContext();
};

} // namespace Ego

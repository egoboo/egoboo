#pragma once

#include "egolib/platform.h"

namespace Ego {

// Forward declaration.
class GraphicsWindow;

/// @brief An graphics context.
class GraphicsContext
{
protected:
    /// @brief Construct this graphics context with the specified window and the specified graphics context properties.
    /// @param window a pointer to the window
    /// @throw idlib::runtime_error @a window is a null pointer
    /// @throw idlib::runtime_error context creation failed
    GraphicsContext(GraphicsWindow *window);

public:
    /// @brief Destruct this graphics context.
    virtual ~GraphicsContext();

}; // class GraphicsContext

} // namespace Ego

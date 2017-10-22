#pragma once

#include "egolib/platform.h"
#include "egolib/Core/Singleton.hpp"
#include "egolib/Graphics/GraphicsSystemNew.hpp"
#include "egolib/Extensions/ogl_extensions.h"

namespace Ego {

// Forward declaration.
class GraphicsWindow;
class GraphicsContext;
class WindowProperties;

struct GraphicsSystem : public Core::Singleton<GraphicsSystem>
{
    /// @brief A pointer to the (single) SDL window if it exists, a null pointer otherwise.
    GraphicsWindow *window;
    
    /// @brief A pointer to the (single) SDL/OpenGL context if it exists, a null pointer otherwise.
    GraphicsContext *context;
    
    /// @brief Initialize the graphics system.
    /// @remark This method is a no-op if the graphics system is initialized.
    GraphicsSystem();

    /// @brief Uninitialize the graphics system.
    /// @remark This method is a no-op if the graphics system is uninitialized.
    virtual ~GraphicsSystem();
    
    /// @brief Create a graphics context.
    /// @param window a pointer to a window
    /// @param contextProperties the context properties
    /// @return a pointer to the graphics context on success, a null pointer on failure
    GraphicsContext *createContext(GraphicsWindow *window);
    
    /// @hrief Create a graphics window.
    /// @return a pointer to the graphics window on success, a null pointer on failure
    GraphicsWindow *createWindow();
};

} // namespace Ego

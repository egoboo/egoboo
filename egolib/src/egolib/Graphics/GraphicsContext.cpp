#include "egolib/Graphics/GraphicsContext.hpp"
#include "egolib/Graphics/GraphicsWindow.hpp"

namespace Ego {

GraphicsContext::GraphicsContext(GraphicsWindow *window, const ContextProperties& contextProperties)
    : window(window)
{
    if (!window)
    {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "window is null");
    }
    contextProperties.upload();
    context = SDL_GL_CreateContext(window->get());
    if (!context)
    {
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__,
                                         "unable to create SDL/OpenGL context: ",
                                         SDL_GetError(), Log::EndOfEntry);
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "unable to create SDL/OpenGL context");
    }

}

GraphicsContext::~GraphicsContext()
{
    if (context)
    {
        SDL_GL_DeleteContext(context);
        context = nullptr;
    }
    window = nullptr;
}

} // namespace Ego

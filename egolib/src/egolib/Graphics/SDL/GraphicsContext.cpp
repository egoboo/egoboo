#include "egolib/Graphics/SDL/GraphicsContext.hpp"

#include "egolib/Graphics/SDL/GraphicsWindow.hpp"
#include "egolib/egoboo_setup.h"
#define GLEW_STATIC
#include <GL/glew.h>

namespace Ego {
namespace SDL {
	
GraphicsContext::GraphicsContext(GraphicsWindow *window) :
	Ego::GraphicsContext(window), window(window)
{
    if (!window)
    {
        throw idlib::null_error(__FILE__, __LINE__, "window");
    }
    context = SDL_GL_CreateContext(window->get());
    if (!context)
    {
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__,
                                         "unable to create SDL/OpenGL context: ",
                                         SDL_GetError(), Log::EndOfEntry);
        throw idlib::runtime_error(__FILE__, __LINE__, "unable to create SDL/OpenGL context");
    }

    if (GLEW_OK != glewInit())
    {
        SDL_GL_DeleteContext(context);
        context = nullptr;
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__,
            "unable to create SDL/OpenGL context: ",
            SDL_GetError(), Log::EndOfEntry);
        throw idlib::runtime_error(__FILE__, __LINE__, "unable to create SDL/OpenGL context");
    }
    auto& config = egoboo_config_t::get();

    // (1) Multisample antialiasing.
    // The GL_MULTISAMPLE/GL_MULTISAMPLE_ARB superseed GL_POINT_SMOOTH, GL_LINE_SMOOTH, and GL_POLYGON_SMOOTH.
    // https://www.khronos.org/opengl/wiki/Multisampling
    if (config.graphic_antialiasing.getValue() > 0)
    {
        glEnable(GL_MULTISAMPLE);
    }
    else
    {
        glDisable(GL_MULTISAMPLE);
    }
    // (2) Dithering.
    if (config.graphic_dithering_enable.getValue())
    {
        glEnable(GL_DITHER);
    }
    else
    {
        glDisable(GL_DITHER);
    }
    // (3) Polygon fill mode.
    // Fill front and back.
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);
    // (4) Lighting.
    // Disable lighting.
    glDisable(GL_LIGHTING);

    // (5) Perspective correction.
    if (config.graphic_perspectiveCorrection_enable.getValue())
    {
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    }
    else
    {
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    }
    // (6) Gouraud shading.
    if (config.graphic_gouraudShading_enable.getValue())
    {
        glShadeModel(GL_SMOOTH);
    }
    else
    {
        glShadeModel(GL_FLAT);
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
	
} // namespace SDL
} // namespace Ego

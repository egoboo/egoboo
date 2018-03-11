#include "egolib/Graphics/GraphicsSystem.hpp"

#include "egolib/Graphics/GraphicsContext.hpp"
#include "egolib/Graphics/GraphicsWindow.hpp"
#include "egolib/Graphics/SDL/Utilities.hpp"
#include "egolib/egoboo_setup.h"

namespace Ego {

GraphicsContext *GraphicsSystem::createContext(GraphicsWindow *window)
{
    return GraphicsSystemNew::get().createContext(window);
}

GraphicsWindow *GraphicsSystem::createWindow()
{
    return GraphicsSystemNew::get().createWindow();
}

// TODO: This leaks like mad if it fails. use std::shared_ptr.
std::pair<GraphicsWindow *, GraphicsContext *> CreateWindowAndContext()
{
    GraphicsWindow *window = nullptr;
    GraphicsContext *context = nullptr;
    auto& configuration = egoboo_config_t::get();
    auto& graphicsSystem = Ego::GraphicsSystemNew::get();

    window = graphicsSystem.createWindow();
    if (!window)
    {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create SDL window: ", SDL_GetError(), Log::EndOfEntry);
        return std::make_pair<GraphicsWindow *, GraphicsContext *>(nullptr, nullptr);
    }
    else
    {
		window->size({ configuration.graphic_resolution_horizontal.getValue(),
				       configuration.graphic_resolution_vertical.getValue() });
        window->center();
        context = graphicsSystem.createContext(window);
        if (!context)
        {
            Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create OpenGLGL context: ", SDL_GetError(), Log::EndOfEntry);
            delete window;
            window = nullptr;
            return std::make_pair<GraphicsWindow *, GraphicsContext *>(nullptr, nullptr);
        }
    }
    {
        // Setup the cute windows manager icon.
        const std::string fileName = "icon.bmp";
        auto pathName = "mp_data/" + fileName;
        SDL_Surface *theSurface = IMG_Load_RW(vfs_openRWopsRead(pathName.c_str()), 1);
        if (!theSurface)
        {
            Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to load icon ", "`", pathName, "`: ", SDL_GetError(), Log::EndOfEntry);
        }
        else
        {
            window->setIcon(theSurface);
            // ...and the surface containing the icon pixel data is no longer required.
            SDL_FreeSurface(theSurface);

        }
    }
    // Set the window title.
    window->title("SDL 2.x OpenGL Window");
    // Return the result.
    return std::make_pair(window, context);
}


GraphicsSystem::GraphicsSystem() :
    window(nullptr), context(nullptr)
{
    // Initialize the NEW graphics system.
    GraphicsSystemNew::initialize();

    Log::get() << Log::Entry::create(Log::Level::Info, __FILE__, __LINE__, "setting SDL video mode", Log::EndOfEntry);

    // Try create window.
    Requirements requirements;
    auto p = CreateWindowAndContext();
    // Fail: Apply heuristic.
    if (!p.first)
    {
        requirements.reset();
        requirements.requirements.push_front(std::make_shared<AliasingRequirement>());
        while (!p.first && requirements.relax())
        {
            p = CreateWindowAndContext();
        }
    }
    // Fail: Apply heuristic.
    if (!p.first)
    {
        requirements.reset();
        requirements.requirements.push_front(std::make_shared<FullscreenRequirement>());
        while (!p.first && requirements.relax())
        {
            p = CreateWindowAndContext();
        }
    }
    if (!p.first)
    {
        auto e = Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "unable to set any video mode", Log::EndOfEntry);
        Log::get() << e;
        throw idlib::runtime_error(__FILE__, __LINE__, e.getText());
    }
    else
    {
        window = p.first;
        context = p.second;
    }
}

GraphicsSystem::~GraphicsSystem()
{
    delete context;
    context = nullptr;
    delete window;
    window = nullptr;
    // Uninitialize the NEW graphics system.
    GraphicsSystemNew::uninitialize();
}

} // namespace Ego

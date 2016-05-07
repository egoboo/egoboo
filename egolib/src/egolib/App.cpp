#include "egolib/App.hpp"

#include "egolib/egolib.h"

namespace Ego {
void App::initialize() {
    // Initialize the graphics system.
    Ego::GraphicsSystem::initialize();
    // Initialize the image manager.
    ImageManager::initialize();
    // Initialize the renderer.
    Ego::Renderer::initialize();
    // Initialize the texture manager.
    TextureManager::initialize();
    // Initialize the font manager.
    Ego::FontManager::initialize();
}

void App::uninitialize() {
    // Uninitialize the font manager.
    Ego::FontManager::uninitialize();
    // Uninitialize the texture manager.
    TextureManager::uninitialize();
    // Uninitialize the renderer.
    Ego::Renderer::uninitialize();
    // Uninitialize the image manager.
    ImageManager::uninitialize();
    // Uninitialize the graphics system.
    Ego::GraphicsSystem::uninitialize();
}
}
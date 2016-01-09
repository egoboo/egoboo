//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************


#include "egolib/Image/ImageManager.hpp"
#include "egolib/egoboo_setup.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/Image/ImageLoader_SDL.hpp"
#include "egolib/Image/ImageLoader_SDL_image.hpp"
#include "egolib/Graphics/PixelFormat.hpp"

ImageManager *ImageManager::_singleton = nullptr;

void ImageManager::registerImageLoaders(int flags) {
    if (flags & IMG_INIT_JPG) {
        _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".jpg"))));
        _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".jpeg"))));
    }
    if (flags & IMG_INIT_PNG) {
        _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".png"))));
    }
    if (flags & IMG_INIT_TIF) {
        _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".tif"))));
        _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".tiff"))));
    }
    _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".gif"))));
    _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".pcx"))));
    _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".ppm"))));
    _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".xpm"))));
    _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".pnm"))));
    _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".lbm"))));
    // Loading TGA images using the standard method does not work according to SDL_Image documentation.
    // @todo MH: A solution is to provide a subclass of ImageLoader_SDL_image for this special case.
#if 0
    _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".tga"))));
#endif
    _loaders.push_back(move(unique_ptr<ImageLoader_SDL_image>(new ImageLoader_SDL_image(".bmp"))));
}

ImageManager::ImageManager() :
    _loaders(),
    _withSDL_image(egoboo_config_t::get().debug_sdlImage_enable.getValue()) {
    using namespace std;
    if (_withSDL_image) {
        Log::get().info("initializing SDL_image imaging version %d.%d.%d ...", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL);
        int flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
#if SDL_VERSIONNUM(SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL) >= SDL_VERSIONNUM(1, 2, 11)
        // WebP support added in SDL_image 1.2.11
        flags |= IMG_INIT_WEBP;
#endif
        flags = IMG_Init(flags);
        // PNG support is mandatory.
        if (!(flags & IMG_INIT_PNG)) {
            Id::EnvironmentErrorException error(__FILE__, __LINE__, "ImageManager", string("Failed to initialize SDL_Image subsystem (") + SDL_GetError() + ")");
            _withSDL_image = false;
            Log::get().warn(" %s\n", ((std::string)error).c_str());
            throw error;
        }
        try {
            registerImageLoaders(flags);
        } catch (...) {
            IMG_Quit();
            Log::get().warn(" failure");
            _withSDL_image = false;
            std::rethrow_exception(std::current_exception());
        }
    } else {
        Log::get().info("SDL_image imaging disable by %s = \"false\" in `setup.txt` - only support for .bmp files\n",
                        egoboo_config_t::get().debug_sdlImage_enable.getName().c_str());
        Log::get().info("initializing standard SDL imaging ...");
        try {
            // These typed are natively supported with SDL.
            // Place them *after* the SDL_image types, so that if both are present,
            // the other types will be preferred over ".bmp".
            _loaders.push_back(move(unique_ptr<ImageLoader_SDL>(new ImageLoader_SDL(".bmp"))));
        } catch (...) {
            Log::get().warn(" failure\n");
            std::rethrow_exception(std::current_exception());
        }
    }
    Log::get().info(" success\n");
}

ImageManager::~ImageManager() {
    if (_withSDL_image) {
        IMG_Quit();
    }
}

void ImageManager::initialize() {
    if (!_singleton) {
        _singleton = new ImageManager();
    }
}

void ImageManager::uninitialize() {
    if (_singleton) {
        delete _singleton;
        _singleton = nullptr;
    }
}

ImageManager& ImageManager::get() {
    if (!_singleton) {
        throw logic_error("image manager not initialized");
    }
    return *_singleton;
}


std::shared_ptr<SDL_Surface> ImageManager::getDefaultImage() {
    /// Create a surface of 8 x 8 blocks each of 16 x 16 pixels.
    const auto& pixelFormatDescriptor = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>();
    auto surface = createImage(8 * 16, 8 * 16, pixelFormatDescriptor);
    if (!surface) {
        throw std::runtime_error("unable to create error image");
    }
    // Iterate over the blocks from left to right and from top to bottom.
    // Alternating between black and white.
    for (size_t z = 0, y = 0; y < 8; ++y) {
        for (size_t x = 0; x < 8; ++x) {
            SDL_Rect rect;
            rect.x = x * 16;
            rect.y = y * 16;
            rect.w = 16;
            rect.h = 16;
            Uint32 color;
            if (z % 2 != 0) {
                color = SDL_MapRGBA(surface->format, 0, 0, 0, 255);      // black
            } else {
                color = SDL_MapRGBA(surface->format, 255, 255, 255, 255); // white
            }
            SDL_FillRect(surface.get(), &rect, color);
            z++;
        }
    }
    return surface;
}

std::shared_ptr<SDL_Surface> ImageManager::createImage(size_t width, size_t height, const Ego::PixelFormatDescriptor& pixelFormatDescriptor) {
    SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
                                                pixelFormatDescriptor.getColourDepth().getDepth(),
                                                pixelFormatDescriptor.getRedMask(),
                                                pixelFormatDescriptor.getGreenMask(),
                                                pixelFormatDescriptor.getBlueMask(),
                                                pixelFormatDescriptor.getAlphaMask());
    if (!surface)  {
        return nullptr;
    }
    // Note: According to C++ documentation, the deleter is invoked if the std::shared_ptr constructor
    //       raises an exception.
    return std::shared_ptr<SDL_Surface>(surface, [](SDL_Surface *surface) { SDL_FreeSurface(surface); });
}

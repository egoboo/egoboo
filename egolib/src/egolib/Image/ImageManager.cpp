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

namespace Ego {

void ImageManager::registerImageLoaders() {
    using namespace Ego::Internal;
    using namespace Id;
    using String = std::string;
    using Set = std::unordered_set<string>;
    if (egoboo_config_t::get().debug_sdlImage_enable.getValue()) {
        Log::get().info("[image manager]: SDL_image %d.%d.%d\n", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL);
        // JPG support is optional.
        if ((IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG) == IMG_INIT_JPG) {
            loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".jpg", ".jpeg"})));
        }
        // PNG support is mandatory.
        if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == IMG_INIT_PNG) {
            loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".png"})));
        } else {
            Log::Entry e(Log::Level::Warning, __FILE__, __LINE__);
            e << "[image manager]: SDL_image does not support PNG file format: " << SDL_GetError() << Log::EndOfLine;
            Log::get() << e;
            throw EnvironmentErrorException(__FILE__, __LINE__, "font manager", String("SDL_image does not support PNG file format: ") + SDL_GetError());
        }
        // WEBP support is optional and available in SDL_image 1.2.11 or higher.
    #if SDL_VERSIONNUM(SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL) >= SDL_VERSIONNUM(1, 2, 11)
        if ((IMG_Init(IMG_INIT_WEBP) & IMG_INIT_WEBP) == IMG_INIT_WEBP) {
            loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".webp"})));
        }
    #endif
        // TIF support is optional.
        if ((IMG_Init(IMG_INIT_TIF) & IMG_INIT_TIF) == IMG_INIT_TIF) {
            loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".tif", ".tiff"})));
        }
        loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".gif"})));
        loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".pcx"})));
        loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".ppm"})));
        loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".xpm"})));
        loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".pnm"})));
        loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".lbm"})));
        // Loading TGA images using the standard method does not work according to SDL_Image documentation.
        // @todo MH: A solution is to provide a subclass of ImageLoader_SDL_image for this special case.
    #if 0
        loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".tga"})));
    #endif
        loaders.push_back(move(make_unique<ImageLoader_SDL_image>(Set{".bmp"})));
    } else {
        Log::get().info("[image manager]: SDL_image disable by %s = \"false\" in `setup.txt` - using SDL -  only support for .bmp files\n",
                        egoboo_config_t::get().debug_sdlImage_enable.getName().c_str());
    }
    // These loaders are natively supported with SDL.
    // Place them *after* the SDL_image loaders, such that the SDL_image loader loaders are preferred.
    loaders.push_back(move(make_unique<ImageLoader_SDL>(Set{".bmp"})));
}

ImageManager::ImageManager() :
    loaders() {
    try {
        registerImageLoaders();
    } catch (...) {
        if (0 != IMG_Init(0)) {
            IMG_Quit();
        }
    }
}

ImageManager::~ImageManager() {
    if (0 != IMG_Init(0)) {
        IMG_Quit();
    }
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

} // namespace Ego

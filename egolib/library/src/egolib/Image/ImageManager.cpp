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

/// @file egolib/Image/ImageManager.cpp
/// @brief An image manager.
/// @author Michael Heilmann

#include "egolib/Image/ImageManager.hpp"
#include "egolib/egoboo_setup.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/Image/ImageLoader_SDL.hpp"
#include "egolib/Image/ImageLoader_SDL_image.hpp"
#include "egolib/Image/SDL_Image_Extensions.h"
#include "egolib/Graphics/PixelFormat.hpp"
#include "egolib/Image/ImageLoader.hpp"

namespace Ego {

ImageManager::Iterator::Iterator(const ImageManager::Loaders::const_iterator& inner) :
    m_inner(inner)
{}

// CRTP
void ImageManager::Iterator::increment()
{
    ++m_inner;
}

// CRTP
bool ImageManager::Iterator::equal_to(const Iterator& other) const
{
    return m_inner == other.m_inner;
}

ImageManager::Iterator::Iterator() :
    m_inner()
{}

ImageManager::Iterator::Iterator(const Iterator& other) :
    m_inner(other.m_inner)
{}

ImageManager::Iterator& ImageManager::Iterator::operator=(const Iterator& other)
{
    m_inner = other.m_inner;
    return *this;
}

ImageManager::Iterator::reference ImageManager::Iterator::operator*() const
{
    auto x = (*m_inner).get();
    return *x;
}

ImageManager::Iterator::reference ImageManager::Iterator::operator->() const
{
    auto x = (*m_inner).get();
    return *x;
}

void ImageManager::registerImageLoaders()
{
    if (egoboo_config_t::get().debug_sdlImage_enable.getValue())
    {
        Log::get() << Log::Entry::create(Log::Level::Info, __FILE__, __LINE__, "[image manager]: SDL_image ", SDL_IMAGE_MAJOR_VERSION, ".", SDL_IMAGE_MINOR_VERSION, ".", SDL_IMAGE_PATCHLEVEL, Log::EndOfEntry);
        // JPG support is optional.
        if ((IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG) == IMG_INIT_JPG)
        {
            loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".jpg", "jpeg"}));
        }
        // PNG support is mandatory.
        if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == IMG_INIT_PNG)
        {
            loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".png"}));
        }
        else
        {
            auto e = Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "[image manager]: SDL_image does not "
                                        "support PNG file format: ", SDL_GetError(), Log::EndOfLine);
            Log::get() << e;
            throw idlib::environment_error(__FILE__, __LINE__, "font manager", e.getText());
        }
        // WEBP support is optional and available in SDL_image 1.2.11 or higher.
    #if SDL_VERSIONNUM(SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL) >= SDL_VERSIONNUM(1, 2, 11)
        if ((IMG_Init(IMG_INIT_WEBP) & IMG_INIT_WEBP) == IMG_INIT_WEBP)
        {
            loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".webp"}));
        }
    #endif
        // TIF support is optional.
        if ((IMG_Init(IMG_INIT_TIF) & IMG_INIT_TIF) == IMG_INIT_TIF)
        {
            loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".tif", "tiff"}));
        }
        loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".gif"}));
        loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".pcx"}));
        loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".ppm"}));
        loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".xpm"}));
        loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".pnm"}));
        loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".lbm"}));
        // Loading TGA images using the standard method does not work according to SDL_Image documentation.
        // @todo MH: A solution is to provide a subclass of ImageLoader_SDL_image for this special case.
    #if 0
        loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".tga"}));
    #endif
        loaders.push_back(std::make_unique<Internal::ImageLoader_SDL_image>(std::unordered_set<std::string>{".bmp"}));
    }
    else
    {
        Log::get() << Log::Entry::create(Log::Level::Info, __FILE__, __LINE__, "[image manager]: SDL_image disable by ", egoboo_config_t::get().debug_sdlImage_enable.getName(), " = false in `setup.txt` "
                                         " - using SDL -  only support for .bmp files", Log::EndOfEntry);
    }
    // These loaders are natively supported with SDL.
    // Place them *after* the SDL_image loaders, such that the SDL_image loader loaders are preferred.
    loaders.push_back(std::make_unique<Internal::ImageLoader_SDL>(std::unordered_set<std::string>{".bmp"}));
}

ImageManager::ImageManager() :
    loaders()
{
    try
    {
        registerImageLoaders();
    }
    catch (...)
    {
        if (0 != IMG_Init(0))
        {
            IMG_Quit();
        }
    }
}

ImageManager::~ImageManager()
{
    if (0 != IMG_Init(0))
    {
        IMG_Quit();
    }
}

ImageManager::Iterator ImageManager::find(std::unordered_set<std::string> extensions, Iterator start) const
{
    auto it = start;
    while (it != end())
    {
        auto supportedExtensions = (*it).getExtensions();
        auto found = std::find_first_of(extensions.cbegin(), extensions.cend(),
                                        supportedExtensions.cbegin(), supportedExtensions.cend());
        if (found != extensions.end())
        {
            return it;
        }
    }
    return end();
}

ImageManager::Iterator ImageManager::find(std::unordered_set<std::string> extensions)
{
    return find(extensions, begin());
}

ImageManager::Iterator ImageManager::begin() const
{
    return Iterator(loaders.begin());
}

ImageManager::Iterator ImageManager::end() const
{
    return Iterator(loaders.end());
}

std::shared_ptr<SDL_Surface> ImageManager::getDefaultImage()
{
    /// Create a surface of 8 x 8 blocks each of 16 x 16 pixels.
    const auto& pixelFormatDescriptor = pixel_descriptor::get<idlib::pixel_format::R8G8B8A8>();
    auto surface = createImage(8 * 16, 8 * 16, pixelFormatDescriptor);
    if (!surface)
    {
        throw std::runtime_error("unable to create error image");
    }
    // Iterate over the blocks from left to right and from top to bottom.
    // Alternating between black and white.
    for (size_t z = 0, y = 0; y < 8; ++y)
    {
        for (size_t x = 0; x < 8; ++x)
        {
            auto rect = Rectangle2f({x * 16, y * 16}, {x * 16 + 16, y * 16 + 16});
            if (z % 2 != 0)
            {
                idlib::fill(surface.get(), Colour3b::black(), rect); // black
            }
            else
            {
                idlib::fill(surface.get(), Colour3b::white(), rect); // white
            }
            z++;
        }
    }
    return surface;
}

std::shared_ptr<SDL_Surface> ImageManager::createImage(size_t width, size_t height, size_t pitch, const pixel_descriptor& pixel_descriptor, void *pixels)
{
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixels,
                                                    width,
                                                    height,
                                                    pixel_descriptor.get_color_depth().depth(),
                                                    pitch,
                                                    pixel_descriptor.get_red().get_mask(),
                                                    pixel_descriptor.get_green().get_mask(),
                                                    pixel_descriptor.get_blue().get_mask(),
                                                    pixel_descriptor.get_alpha().get_mask());
    if (!surface)
    {
        throw idlib::environment_error(__FILE__, __LINE__, "SDL image", "unable to create surface");
    }
    // Note: According to C++ documentation, the deleter is invoked if the std::shared_ptr constructor
    //       raises an exception.
    return std::shared_ptr<SDL_Surface>(surface, [](SDL_Surface *surface) { SDL_FreeSurface(surface); });
}

std::shared_ptr<SDL_Surface> ImageManager::createImage(size_t width, size_t height, const pixel_descriptor& pixel_descriptor)
{
    SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
                                                pixel_descriptor.get_color_depth().depth(),
                                                pixel_descriptor.get_red().get_mask(),
                                                pixel_descriptor.get_green().get_mask(),
                                                pixel_descriptor.get_blue().get_mask(),
                                                pixel_descriptor.get_alpha().get_mask());
    if (!surface)
    {
        return nullptr;
    }
    // Note: According to C++ documentation, the deleter is invoked if the std::shared_ptr constructor
    //       raises an exception.
    return std::shared_ptr<SDL_Surface>(surface, [](SDL_Surface *surface) { SDL_FreeSurface(surface); });
}

std::shared_ptr<SDL_Surface> ImageManager::createImage(size_t width, size_t height)
{
    return createImage(width, height, pixel_descriptor::get<idlib::pixel_format::R8G8B8A8>());
}

void ImageManager::save_as_bmp(const std::shared_ptr<SDL_Surface>& pixels, const std::string& pathname)
{
    SDL_SaveBMP_RW(pixels.get(), vfs_openRWopsWrite(pathname.c_str()), 1);
}
void ImageManager::save_as_png(const std::shared_ptr<SDL_Surface>& pixels, const std::string& pathname)
{
    IMG_SavePNG_RW(pixels.get(), vfs_openRWopsWrite(pathname.c_str()), 1);
}
} // namespace Ego

std::shared_ptr<SDL_Surface> gfx_loadImage(const std::string& pathname)
{
	SDL_Surface *image = IMG_Load_RW(vfs_openRWopsRead(pathname.c_str()), 1);
	if (!image)
	{
		return nullptr;
	}
	return std::shared_ptr<SDL_Surface>(image, [](SDL_Surface *surface) { SDL_FreeSurface(surface); });
}
